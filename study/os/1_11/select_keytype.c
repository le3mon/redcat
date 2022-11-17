#include "../MINT64/01.Kernel32/Source/Types.h"

BOOL kIsUseCombinedCode(BYTE bScanCode) {
    BYTE bDownScanCode;
    BOOL bUseCombinedkey = FALSE;

    bDownScanCode = bScanCode & 0x7F;

    // 알파벳은 shift나 caps lock에 영향을 받음
    if(kIsAlphabetScanCode(bDownScanCode) == TRUE) {
        // shift나 caps lock 둘 중 하나가 눌려있다면 조합된 키를 돌려줌
        if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn)
            bUseCombinedkey = TRUE;
        
        else 
            bUseCombinedkey = FALSE;
    }

    // 숫자와 기호 키라면 shift 키의 영향을 받음
    else if(kIsNumberOrSymbolScan(bDownScanCode) == TRUE) {
        if(gs_stKeyboardManager.bShiftDown == TRUE)
            bUseCombinedkey = TRUE;
        
        else
            bUseCombinedkey = FALSE;
    }

    // 숫자 패드 키라면 Num Lock의 영향을 받음
    // 0xE0만 제외하면 확장 키 코드와 숫자 패드의 코드가 겹치므로, 확장 키 코드가 수신되지 않았을 때만 조합된 코드 사용
    else if((kIsNumberPadScanCode(bDownScanCode) == TRUE) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE)) {
        if(gs_stKeyboardManager.bNumLockOn == TRUE)
            bUseCombinedkey = TRUE;
        
        else
            bUseCombinedkey = FALSE;
    }

    return bUseCombinedkey;
}

BOOL kIsAlphabetScanCode(BYTE bScanCode) {
    // 변환 테이블을 이용하여 알파벳 범위인지 확인
    if(('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) && (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z'))
        return TRUE;
    
    return FALSE;
}

BOOL kIsNumberOrSymbolScan(BYTE bScanCode) {
    // 숫자 패드나 확장 키 범위를 제외한 범위(스캔 코드 2 ~ 53)에서 영문자가 아니면 숫자 또는 기호임
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