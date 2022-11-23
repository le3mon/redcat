#include "../MINT64/02.Kernel64/Source/Types.h"

BYTE kGetCh(void) {
    KEYDATA stData;

    while(1) {
        while(kGetKeyFromKeyQueue(&stData) == FALSE) {
            ;
        }

        if(stData.bFlags & KEY_FLAGS_DOWN)
            return stData.bASCIICode;
    }
}