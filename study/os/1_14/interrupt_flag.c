#include "queue_struct.h"

BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
    QWORD qwRFLAGS;

    qwRFLAGS = kReadRFLAGS();
    if(bEnableInterrupt == TRUE)
        kEnableInterrupt();
    else
        kDisableInterrupt();
    
    if(qwRFLAGS & 0x0200)
        return TRUE;
    return FALSE;
}