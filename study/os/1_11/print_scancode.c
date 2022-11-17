#include "../MINT64/01.Kernel32/Source/Types.h"

char vcTemp[2] = {0,};
BYTE bFlags;
BYTE bTemp;
int i = 0;

while(1) {
    if(kIsOutputBufferFull() == TRUE) {
        bTemp = kGetKeyboardScanCode();

        if(kConvertScanCodeToASCIICode(bTemp, &(vcTemp[0]), &bFlags) == TRUE) [
            if(bFlags & KEY_FLAGS_DOWN)
                kPrintString(i++, 13, vcTemp);
        ]
    }
}
