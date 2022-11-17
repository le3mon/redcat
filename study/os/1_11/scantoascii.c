#include "../MINT64/01.Kernel32/Source/Types.h"

#define KEY_SKIPCOUNTFORPAUSE 2
#define KEY_FLAGS_UP          0x00
#define KEY_FLAGS_DOWN        0x01
#define KEY_FLAGS_EXTENDEDKEY 0x02

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE *pbASCIICode, BOOL *pbFlags) {
    BOOL bUseCombinedKey;

    if(gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    if(bScanCode == 0xE1) {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    }
    else if(bScanCode == 0xE0) {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }

    bUseCombinedKey = kIsUseCombinedCode(bScanCode);

    if(bUseCombinedKey == TRUE)
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;

    else
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;

    if(gs_stKeyboardManager.bExtendedCodeIn == TRUE) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    }
    else
        *pbFlags = 0;
    
    if((bScanCode & 0x80) == 0)
        *pbFlags |= KEY_FLAGS_DOWN;
    
    UpdateCombinationKeyStatusAndLED(bScanCode);
    return TRUE;
}