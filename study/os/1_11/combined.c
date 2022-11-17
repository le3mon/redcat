#include "../MINT64/01.Kernel32/Source/Types.h"

void UpdateCombinationKeyStatusAndLED(BYTE bScanCode) {
    BOOL bDown;
    BYTE bDownScanCode;
    BOOL bLEDStatusChanged = FALSE;

    // 눌렀다 떨어질 때 상태이면 최상위 비트(비트 7)이 1인 경우(0x80이 더한 값)는 UP 상태 임을 알 수 있다.
    // 반대로 최상위 비트가 0인 경우는 Down 상태임을 알 수 있다.
    if(bScanCode & 0x80) {
        bDown = FALSE;
        bDownScanCode = bScanCode & 0x7F;
    }
    else {
        bDown = TRUE;
        bDownScanCode = bScanCode;
    }

    // 조합 키 검색
    // shift 키의 스캔 코드(42 or 54)이면 shift 키의 상태 갱신
    if((bDownScanCode == 42) || (bDownScanCode == 54))
        gs_stKeyboardManager.bShiftDown = bDown;

    // Caps Lock 키의 스캔 코드(58)이면 Caps Lock의 상태 갱신하고 LED 상태 변경
    else if((bDownScanCode == 58) && (bDown == TRUE)) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    // Num Lock 키의 스캔 코드(69)이면 Num Lock의 상태를 갱신하고 LED 상태 변경
    else if((bDownScanCode == 69) && (bDown == TRUE)) {
        gs_stKeyboardManager.bNumLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    // Scroll Lock 키의 스캔 코드(70)이면 Scroll Lock 상태 갱신 및 LED 상태 변경
    else if((bDownScanCode == 70) && (bDown == TRUE)) {
        gs_stKeyboardManager.bScrollLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    if(bLEDStatusChanged == TRUE)
        kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
}