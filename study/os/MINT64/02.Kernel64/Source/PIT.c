#include "PIT.h"
#include "AssemblyUtility.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic) {
    // PIT 컨트롤 레지스터에 모드 0 설정
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

    // 일정한 주기로 반복하는 타이머라면 모드 2 설정
    if(bPeriodic == TRUE) 
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    
    kOutPortByte(PIT_PORT_COUNTER0, wCount);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0(void) {
    BYTE bHighByte, bLowByte;
    WORD wTemp = 0;

    // 값을 읽기 위해 래치 모드 설정
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);

    wTemp = bHighByte;
    wTemp = (wTemp << 8) | bLowByte;
    return wTemp;
}

void kWaitUsingDirectPIT(WORD wCount) {
    WORD wLastCounter0;
    WORD wCurrentCounter0;

    // PIT 컨트롤러 0 ~ 0xFFFF 까지 반복해서 카운팅하도록 설정
    kInitializePIT(0, TRUE);

    // 지금부터 wCount 이상 증가할 때까지 대기
    wLastCounter0 = kReadCounter0();
    while (1) {
        wCurrentCounter0 = kReadCounter0();
        if(((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount)
            break;
    }   
}