#include "../MINT64/01.Kernel32/Source/Types.h"

typedef struct kKeyboardManagerStruct {
    BOOL bShiftDown;
    BOOL bCapsLockOn;
    BOOL bNumLockOn;
    BOOL bScrollLockOn;

    BOOL bExtendedCodeIn;
    int iSkipCountForPause;
} KEYBOARDMANAGER;
