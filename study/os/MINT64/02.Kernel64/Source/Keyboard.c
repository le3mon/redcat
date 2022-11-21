#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"

BOOL kIsOutputBufferFull(void) {
    // 상태 레지스터(0x64)에서 출력 버퍼 상태 비트(비트 0)이 1일 경우
    // 키보드가 전송한 데이터가 존재함
    if(kInPortByte(0x64) & 0x01)
        return TRUE;

    return FALSE;
}

BOOL kIsInputBufferFull(void) {
    // 상태 레지스터에서 입력 버퍼 상태 비트(비트 1)이 1일 경우
    // 아직 입력 버퍼 내의 데이터 존재
    if(kInPortByte(0x64) & 0x02)
        return TRUE;
    
    return FALSE;
}

// ACK 대기, ACK이 아닌 다른 스캔 코드는 변환해서 큐에 삽입
BOOL kWaitForACKAndPutOtherScanCode(void) {
    int i, j;
    BYTE bData;
    BOOL bResult = FALSE;

    for(j = 0; j < 100; j++) {
        for(i = 0; i < 0xFFFF; i++) {
            if(kIsOutputBufferFull() == TRUE)
                break;
        }

        bData = kInPortByte(0x60);
        if(bData == 0xFA) {
            bResult = TRUE;
            break;
        }
        else // ACK이 아니면 아스키 코드로 변환하여 키 큐에 삽입
            kConvertScanCodeAndPutQueue(bData);
    }
    return bResult;
}

BOOL kActivateKeyboard(void) {
    int i, j;
    BOOL bPreviousInterrupt;
    BOOL bResult;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // 컨트롤 레지스터에(0x64) 키보드 활성화 커맨드(0xAE) 전달
    kOutPortByte(0x64, 0xAE);

    for(i = 0; i < 0xFFFF; i++) {
        // 입력 버퍼가 비어있을 경우 키보드가 이를 처리했다는 의미
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    // 입력 버퍼에(0x60) 키보드 활성화(0xF4) 커맨드를 키보드로 전송
    kOutPortByte(0x60, 0xF4);

    bResult = kWaitForACKAndPutOtherScanCode();

    kSetInterruptFlag(bPreviousInterrupt);
    return bResult;
}

BYTE kGetKeyboardScanCode(void) {
    while(kIsOutputBufferFull() == FALSE) {
        ;
    }
    return kInPortByte(0x60);
}


BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn) {
    int i, j;
    BOOL bPreviousInterrupt;
    BOOL bResult;
    BYTE bData;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    // 출력 버퍼(0x60)로 LED 상태 변경 커맨드(0xED) 전송
    kOutPortByte(0x60, 0xED);
    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    bResult = kWaitForACKAndPutOtherScanCode();
    if(bResult == FALSE) {
        kSetInterruptFlag(bPreviousInterrupt);
        return FALSE;
    }
    
    kOutPortByte(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | (bScrollLockOn));
    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    bResult = kWaitForACKAndPutOtherScanCode();
    if(bResult == FALSE) {
        kSetInterruptFlag(bPreviousInterrupt);
        return FALSE;
    }
    
    return bResult;
}

void kEnableA20Gate(void) {
    BYTE bOutputPortData;
    int i;

    // 컨트롤 레지스터에 키보드 컨트롤러의 출력 포트 값을 읽는 커맨드(0xD0) 전송
    kOutPortByte(0x64, 0xD0);

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsOutputBufferFull() == TRUE)
            break;
    }

    // 출력 포트에 수신된 키보드 컨트롤러 출력 포트 값 읽음
    bOutputPortData = kInPortByte(0x60);

    bOutputPortData |= 0x01; // A20 게이트 비트 설정

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    // 커맨드 레지스터(0x64)에 출력 포트 설정 커맨드(0xD1) 전달
    kOutPortByte(0x64, 0xD1);

    // 입력 버퍼에 A20 게이트 비트가 1로 설정된 값 전달
    kOutPortByte(0x60, bOutputPortData);
}

void kReboot(void) {
    int i;

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    // 커맨드 레지스터에 출력 포트 설정 커맨드(0xD1) 전달
    kOutPortByte(0x64, 0xD1);

    // 입력 버퍼에 0을 전달하여 프로세서 리셋
    kOutPortByte(0x60, 0x00);
    while (1) {
        ;
    }
}

// 스캔 코드를 아스키 코드로 변환하는 기능에 관련된 함수들

// 키보드 상태를 관리하는 키보드 매니저
static KEYBOARDMANAGER gs_stKeyboardManager = {0, };

static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
    /* 0 */ { KEY_NONE , KEY_NONE },
    /* 1 */ { KEY_ESC , KEY_ESC },
    /* 2 */ { '1' , '!' },
    /* 3 */ { '2' , '@' },
    /* 4 */ { '3' , '#' },
    /* 5 */ { '4' , '$' },
    /* 6 */ { '5' , '%' },
    /* 7 */ { '6' , '^' },
    /* 8 */ { '7' , '&' },
    /* 9 */ { '8' , '*' },
    /* 10 */ { '9' , '(' },
    /* 11 */ { '0' , ')' },
    /* 12 */ { '-' , '_' },
    /* 13 */ { '=' , '+' },
    /* 14 */ { KEY_BACKSPACE , KEY_BACKSPACE },
    /* 15 */ { KEY_TAB , KEY_TAB },
    /* 16 */ { 'q' , 'Q' },
    /* 17 */ { 'w' , 'W' },
    /* 18 */ { 'e' , 'E' },
    /* 19 */ { 'r' , 'R' },
    /* 20 */ { 't' , 'T' },
    /* 21 */ { 'y' , 'Y' },
    /* 22 */ { 'u' , 'U' },
    /* 23 */ { 'i' , 'I' },
    /* 24 */ { 'o' , 'O' },
    /* 25 */ { 'p' , 'P' },
    /* 26 */ { '[' , '{' },
    /* 27 */ { ']' , '}' },
    /* 28 */ { '\n' , '\n' },
    /* 29 */ { KEY_CTRL , KEY_CTRL },
    /* 30 */ { 'a' , 'A' },
    /* 31 */ { 's' , 'S' },
    /* 32 */ { 'd' , 'D' },
    /* 33 */ { 'f' , 'F' },
    /* 34 */ { 'g' , 'G' },
    /* 35 */ { 'h' , 'H' },
    /* 36 */ { 'j' , 'J' },
    /* 37 */ { 'k' , 'K' },
    /* 38 */ { 'l' , 'L' },
    /* 39 */ { ';' , ':' },
    /* 40 */ { '\'' , '\"' },
    /* 41 */ { '`' , '~' },
    /* 42 */ { KEY_LSHIFT , KEY_LSHIFT },
    /* 43 */ { '\\' , '|' },
    /* 44 */ { 'z' , 'Z' },
    /* 45 */ { 'x' , 'X' },
    /* 46 */ { 'c' , 'C' },
    /* 47 */ { 'v' , 'V' },
    /* 48 */ { 'b' , 'B' },
    /* 49 */ { 'n' , 'N' },
    /* 50 */ { 'm' , 'M' },
    /* 51 */ { ',' , '<' },
    /* 52 */ { '.' , '>' },
    /* 53 */ { '/' , '?' },
    /* 54 */ { KEY_RSHIFT , KEY_RSHIFT },
    /* 55 */ { '*' , '*' },
    /* 56 */ { KEY_LALT , KEY_LALT },
    /* 57 */ { ' ' , ' ' },
    /* 58 */ { KEY_CAPSLOCK , KEY_CAPSLOCK },
    /* 59 */ { KEY_F1 , KEY_F1 },
    /* 60 */ { KEY_F2 , KEY_F2 },
    /* 61 */ { KEY_F3 , KEY_F3 },
    /* 62 */ { KEY_F4 , KEY_F4 },
    /* 63 */ { KEY_F5 , KEY_F5 },
    /* 64 */ { KEY_F6 , KEY_F6 },
    /* 65 */ { KEY_F7 , KEY_F7 },
    /* 66 */ { KEY_F8 , KEY_F8 },
    /* 67 */ { KEY_F9 , KEY_F9 },
    /* 68 */ { KEY_F10 , KEY_F10 },
    /* 69 */ { KEY_NUMLOCK , KEY_NUMLOCK },
    /* 70 */ { KEY_SCROLLLOCK , KEY_SCROLLLOCK },  
    /* 71 */ { KEY_HOME , '7' },
    /* 72 */ { KEY_UP , '8' },
    /* 73 */ { KEY_PAGEUP , '9' },
    /* 74 */ { '-' , '-' },
    /* 75 */ { KEY_LEFT , '4' },
    /* 76 */ { KEY_CENTER , '5' },
    /* 77 */ { KEY_RIGHT , '6' },
    /* 78 */ { '+' , '+' },
    /* 79 */ { KEY_END , '1' },
    /* 80 */ { KEY_DOWN , '2' },
    /* 81 */ { KEY_PAGEDOWN , '3' },
    /* 82 */ { KEY_INS , '0' },
    /* 83 */ { KEY_DEL , '.' },
    /* 84 */ { KEY_NONE , KEY_NONE },
    /* 85 */ { KEY_NONE , KEY_NONE },
    /* 86 */ { KEY_NONE , KEY_NONE },
    /* 87 */ { KEY_F11 , KEY_F11 },
    /* 88 */ { KEY_F12 , KEY_F12 } 
};

BOOL kIsAlphabetScanCode(BYTE bScanCode) {
    if(('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) && 
        (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z')) {
            return TRUE;
        }
    
    return FALSE;
}

BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode) {
    // 숫자 패드나 확장 키 범위를 제외한 범위(스캔 코드 2 ~ 53)에서 영문자가 아니면 숫자나 기호임
    if((2 <= bScanCode) && (bScanCode <= 53) && (kIsAlphabetScanCode(bScanCode) == FALSE))
        return TRUE;
    
    return FALSE;
}

BOOL kIsNumberPadScanCode(BYTE bScanCode) {
    // 숫자 패드는 스캔 코드의 71 ~ 83에 있음
    if((71 <= bScanCode) && (bScanCode <= 83))
        return TRUE;
    
    return FALSE;
}

BOOL kIsUseCombinedCode(BYTE bScanCode) {
    BYTE bDownScanCode;
    BOOL bUseCombinedKey = FALSE;

    bDownScanCode = bScanCode & 0x7F;

    // 알파벳이라면 Shift, Caps Lock의 영향을 받음
    if(kIsAlphabetScanCode(bScanCode) == TRUE) {
        if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn)
            bUseCombinedKey = TRUE;
        else
            bUseCombinedKey = FALSE;
    }

    // 숫자와 기호라면 Shift 키의 영향을 받음
    else if(kIsNumberOrSymbolScanCode(bScanCode) == TRUE) {
        if(gs_stKeyboardManager.bShiftDown == TRUE)
            bUseCombinedKey = TRUE;
        else
            bUseCombinedKey = FALSE;
    }

    // 숫자 패드 키라면 Num Lock 키의 영향을 받음
    // 0xE0만 제외하면 확장 키 코드와 숫자 패드의 코드가 겹치므로
    // 확장 키 코드가 수신되지 않았을 때만 처리, 조합된 코드 사용
    else if((kIsNumberPadScanCode(bScanCode) == TRUE) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE)) {
        if(gs_stKeyboardManager.bNumLockOn == TRUE)
            bUseCombinedKey = TRUE;
        else
            bUseCombinedKey = FALSE;
    }
    return bUseCombinedKey;
}

void UpdateCombinationKeyStatusAndLED(BYTE bScanCode) {
    BOOL bDown;
    BYTE bDownScanCode;
    BOOL bLEDStatusChanged = FALSE;

    if(bScanCode & 0x80) {
        bDown = FALSE;
        bDownScanCode = bScanCode & 0x7F;
    }
    else {
        bDown = TRUE;
        bDownScanCode = bScanCode;
    }

    // 조합 키 검색
    // Shift 키의 스캔 코드(42 or 54)이면 Shift 키의 상태 갱신
    if((bDownScanCode == 42) || (bDownScanCode == 54))
        gs_stKeyboardManager.bShiftDown = bDown;

    // Caps Lock(58)이면 상태 갱신 및 LED 상태 변경
    else if((bDownScanCode == 58) && (bDown == TRUE)) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    //  Num Lock(69) 위와 동일
    else if((bDownScanCode == 69) && (bDown == TRUE)) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    // Scroll Lock(70) 위와 동일
    else if((bDownScanCode == 70) && (bDown == TRUE)) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    if(bLEDStatusChanged == TRUE)
        kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
}

BOOL kConvertScanCodeToASCIITable(BYTE bScanCode, BYTE *pbASCIICode, BOOL *pbFlags) {
    BOOL bUseCombinedKey;

    // 이전에 Pause 키가 수신되었다면, Pause의 남은 스캔 코드 무시
    if(gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    // Pause 키는 특별히 처리
    if(bScanCode == 0xE1) {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    }

    // 확장 키 코드가 들어왔을 때, 실제 키 값은 다음에 들어오므로 플래그 설정만 하고 종료
    else if(bScanCode == 0xE0) {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }

    //  조합된 키를 반환해야 하는가?
    bUseCombinedKey = kIsUseCombinedCode(bScanCode);

    if(bUseCombinedKey == TRUE)
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
    else
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
    
    // 확장 키 유무 설정
    if(gs_stKeyboardManager.bExtendedCodeIn == TRUE) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    }
    else
        *pbFlags = 0;
    
    if((bScanCode & 0x80) == 0)
        *pbFlags |= KEY_FLAGS_DOWN;
    
    // 조합 키 눌림이나 떨어짐 상태 갱신
    UpdateCombinationKeyStatusAndLED(bScanCode);
    return TRUE;
}

BOOL kInitializeKeyboard(void) {
    // 큐 초기화
    kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));
    
    return kActivateKeyboard(); // 키보드 활성화
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
    KEYDATA stData;
    BOOL bResult = FALSE;
    BOOL bPreviousInterrupt;

    stData.bScanCode = bScanCode;

    if(kConvertScanCodeToASCIITable(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE) {
        bPreviousInterrupt = kSetInterruptFlag(FALSE);
        bResult = kPutQueue(&gs_stKeyQueue, &stData);
        kSetInterruptFlag(bPreviousInterrupt);
    }

    return bResult;
}

BOOL kGetKeyFromKeyQueue(KEYDATA *pstData) {
    BOOL bResult;
    BOOL bPreviousInterrupt;

    if(kIsQueueEmpty(&gs_stKeyQueue) == TRUE)
        return FALSE;
    
    bPreviousInterrupt = kSetInterruptFlag(FALSE);
    bResult = kGetQueue(&gs_stKeyQueue, pstData);
    kSetInterruptFlag(bPreviousInterrupt);
    
    return bResult;
}