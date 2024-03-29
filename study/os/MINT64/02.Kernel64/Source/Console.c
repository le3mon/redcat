#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"

CONSOLEMANAGER gs_stConsoleManager = {0, };

// 그래픽 모드로 시작했을 때 사용하는 화면 버퍼 영역
static CHARACTER gs_vstScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];

// 그래픽 모드로 시작할 때 GUI 콘솔 셸 윈도우로 전달된 키 이벤트를 콘솔 셸 태스크로 전달하는 큐 버퍼
static KEYDATA gs_vstKeyQueueBuffer[CONSOLE_GUIKETQUEUE_MAXCOUNT];

// 콘솔 초기화
void kInitializeConsole(int iX, int iY) {
    // 자료구조를 모두 0으로 초기화
    kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));

    // 화면 버퍼 초기화
    kMemSet(&gs_vstScreenBuffer, 0, sizeof(gs_vstScreenBuffer));

    if(kIsGraphicMode() == FALSE) {
        // 그래픽 모드 X -> 비디오 메모리를 화면 버퍼로 설정
        gs_stConsoleManager.pstScreenBuffer = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    }
    else {
        // 그래픽 모드 시작 시 그래픽 모드용 화면 버퍼를 설정
        gs_stConsoleManager.pstScreenBuffer = gs_vstScreenBuffer;

        // 그래픽 모드에서 사용할 키 큐와 뮤텍스 초기화
        kInitializeQueue(&(gs_stConsoleManager.stKeyQueueForGUI), gs_vstKeyQueueBuffer,
            CONSOLE_GUIKETQUEUE_MAXCOUNT, sizeof(KEYDATA));
        kInitializeMutex(&(gs_stConsoleManager.stLock));
    }

    // 커서 위치 설정
    kSetCursor(iX, iY);
}

// 커서 위치 설정
// 문자를 출력할 위치도 설정
void kSetCursor(int iX, int iY) {
    int iLinearValue;
    int iOldX, iOldY;
    int i;

    // 커서 위치 계산
    iLinearValue = iY * CONSOLE_WIDTH + iX;

    // 텍스트 모드로 시작 시 CRT 컨트롤러로 커서 위치 전송
    if(kIsGraphicMode() == FALSE) {
        // 상위 커서 위치 레지스터 선택
        kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
        kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

        // 하위 커서 위치 레지스터 선택
        kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
        kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);
    }
    // 그래픽 모드 시작 시 화면 버퍼에 출력한 커서 위치 옮김
    else {
        // 이전 커서가 있던 위치가 그대로 커서로 남아 있으면 커서를 지움
        for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
            // 커서 있으면 제거
            if((gs_stConsoleManager.pstScreenBuffer[i].bCharactor == '_') &&
                (gs_stConsoleManager.pstScreenBuffer[i].bAttribute == 0x00)) {
                gs_stConsoleManager.pstScreenBuffer[i].bCharactor = ' ';
                gs_stConsoleManager.pstScreenBuffer[i].bAttribute =
                    CONSOLE_DEFAULTTEXTCOLOR;
                break;
            }
        }

        // 새로운 위치에 커서 출력
        gs_stConsoleManager.pstScreenBuffer[iLinearValue].bCharactor = '_';
        gs_stConsoleManager.pstScreenBuffer[iLinearValue].bAttribute = 0x00;
    }

    // 문자를 출력할 위치 업데이트
    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

// 현재 커서의 위치 반환
void kGetCursor(int *piX, int *piY) {
    // 저장된 위치를 콘솔 화면의 너비로 나눈 나머지로 X 좌표를 구할 수 있음
    // 화면 너비로 나누면 Y 좌표 구할 수 있음
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

// printf 함수 내부 구현
void kPrintf(const char *pcFormatString, ...) {
    va_list ap;
    char vcBuffer[1024];
    int iNextPrintOffset;

    // 가변 인자 리스트를 사용해서 vsprintf()로 처리
    va_start(ap, pcFormatString);
    kVSPrintf(vcBuffer, pcFormatString, ap);
    va_end(ap);

    // 포맷 문자열을 화면에 출력
    iNextPrintOffset = kConsolePrintString(vcBuffer);

    kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

// \n, \t 같은 문자가 포함된 문자열을 출력한 후 화면상의 다음 출력할 위치를 반환
int kConsolePrintString(const char *pcBuffer) {
    CHARACTER *pstScreen;
    int i, j;
    int iLength;
    int iPrintOffset;

    // 화면 버퍼 설정
    pstScreen = gs_stConsoleManager.pstScreenBuffer;

    // 문자열을 출력할 위치를 저장
    iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;
    
    // 문자열의 길이만큼 화면에 출력
    iLength = kStrLen(pcBuffer);
    for(i = 0; i < iLength; i++) {
        if(pcBuffer[i] == '\n') {
            // 출력할 위치를 80의 배수 칼럼으로 옮김
            // 현재 라인 남은 문자열 수만큼 더해서 다음 라인으로 위치시킴
            iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
        }
        else if(pcBuffer[i] == '\t') {
            // 출력할 위치를 8의 배수 칼럼으로 옮김
            iPrintOffset += (8 - (iPrintOffset % 8));
        }
        else {
            pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
            pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset++;
        }

        // 출력할 위치가 화면의 최댓값(80*25)을 벗어났으면 스크롤 처리
        if(iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH)) {
            // 가장 윗줄을 제외한 나머지 한 줄 위로 복사
            kMemCpy(pstScreen, pstScreen + CONSOLE_WIDTH, 
            (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

            // 가장 마지막 라인은 공백으로 채움
            for(j = (CONSOLE_HEIGHT - 1) * (CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++) {
                pstScreen[j].bCharactor = ' ';
                pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            }

            // 출력할 위치를 가장 아래쪽 라인의 처음으로 설정
            iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
        }
    }
    return iPrintOffset;
}

// 전체 화면 삭제
void kClearScreen(void) {
    CHARACTER *pstScreen;
    int i;

    // 화면 버퍼 설정
    pstScreen = gs_stConsoleManager.pstScreenBuffer;

    // 화면 전체를 공백으로 채우고 커서의 위치를 0, 0으로 옮김
    for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        pstScreen[i].bCharactor = ' ';
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }

    // 커서를 화면 상단으로 이동
    kSetCursor(0, 0);
}

BYTE kGetCh(void) {
    KEYDATA stData;

    while(1) {
        // 그래픽 모드가 아니면 커널의 키 큐에서 값 가져옴
        if(kIsGraphicMode() == FALSE) {
            // 키 큐에 데이터가 수신될 때까지 대기
            while(kGetKeyFromKeyQueue(&stData) == FALSE) {
                kSchedule();
            }
        }
        // 그래픽 모드면 그래픽 모드용 키 큐에서 값 가져옴
        else {
            while(kGetKeyFromGUIKeyQueue(&stData) == FALSE) {
                // 그래픽 모드에서 동작하는 셸 태스크를 종료해야 되면 루프 종료
                if(gs_stConsoleManager.bExit == TRUE) {
                    return 0xFF;
                }
                kSchedule();
            }
        }
        
        // 키가 눌렀다는 데이터가 수신되면 아스키 코드 반환
        if(stData.bFlags & KEY_FLAGS_DOWN) {
            return stData.bASCIICode;
        }
    }
}

// 문자열을 X, Y 위치에 출력
void kPrintStringXY(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen;
    int i;

    // 화면 버퍼 설정
    pstScreen = gs_stConsoleManager.pstScreenBuffer;

    // 비디오 메모리 어드레스에서 현재 출력할 위치를 계산
    pstScreen += (iY * CONSOLE_WIDTH) + iX;
    // 문자열의 길이만큼 루프를 돌면서 문자와 속성을 저장
    for(i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
}

// 콘솔을 관리하는 자료구조 반환
CONSOLEMANAGER *kGetConsoleManager(void) {
    return &gs_stConsoleManager;
}

// 그래픽 모드용 키 큐에서 키 데이터 제거
BOOL kGetKeyFromGUIKeyQueue(KEYDATA *pstData) {
    BOOL bResult;

    // 큐에 데이터 없으면 실패
    if(kIsQueueEmpty(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE) {
        return FALSE;
    }

    // 동기화 처리
    kLock(&(gs_stConsoleManager.stLock));

    // 큐에서 데이터 제거
    bResult = kGetQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

    // 동기화 처리
    kUnlock(&(gs_stConsoleManager.stLock));

    return bResult;
}

// 그래픽 모드용 키 큐에 데이터 삽입
BOOL kPutKeyToGUIKeyQueue(KEYDATA *pstData) {
    BOOL bResult;

    // 큐에 데이터 가득 차면 실패
    if(kIsQueueFull(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE) {
        return FALSE;
    }

    // 동기화 처리
    kLock(&(gs_stConsoleManager.stLock));

    // 큐에서 데이터 삽입
    bResult = kPutQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

    // 동기화 처리
    kUnlock(&(gs_stConsoleManager.stLock));

    return bResult;
}

// 콘솔 셸 태스크 종료 플래그 설정
void kSetConsoleShellExitFlag(BOOL bFlag) {
    gs_stConsoleManager.bExit = bFlag;
}