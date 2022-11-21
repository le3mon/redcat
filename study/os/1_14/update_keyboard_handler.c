#include "queue_struct.h"
#include "../MINT64/02.Kernel64/Source/Utility.h"

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
    KEYDATA stData;
    stData.bScanCode = bScanCode;

    if(kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE)   
        return kPutQueue(&gs_stKeyQueue, &stData);
}

void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT: , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    kPrintString( 0, 0, vcBuffer );

    if(kIsOutputBufferFull() == TRUE) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }
    
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}