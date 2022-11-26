#include "pit.h"

WORD kReadCounter0(void) {
    BYTE bHighByte, bLowByte;
    BYTE wTemp = 0;

    kOutPortByte(PIT_PORT_CONTROL, PIT_CONTROL_LATCH);

    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);

    wTemp = bHighByte;
    wTemp = (wTemp << 8) | bLowByte;
    return wTemp;
}

void kWaitUsingDirectPIT(WORD wCount) {
    WORD wLastCounter0;
    WORD wCurrentCounter0;

    kInitializePIT(0, TRUE);

    wLastCounter0 = kReadCounter0();
    while (1) {
        wCurrentCounter0 = kReadCounter0();
        if(((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount)
            break;
    }   
}

void kWaitms(long lMillisecon) {
    int i;

    for(i = 0; i < lMillisecon / 30; i++)
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecon % 30));
}