#ifndef __PIT_H__
#define __PIT_H__

#include "../MINT64/02.Kernel64/Source/Types.h"
#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"
#define PIT_FREQUENCY 1193180
#define MSTOCOUNT(x)    (PIT_FREQUENCY * (x) / 1000)
#define USTOCOUNT(x)    (PIT_FREQUENCY * (x) / 1000000)

#define PIT_PORT_CONTROL 0x43
#define PIT_PORT_COUNTER0 0x40

#define PIT_CONTROL_COUNTER0 0x00
#define PIT_CONTROL_LSBMSBRW 0x30
#define PIT_CONTROL_LATCH 0x00
#define PIT_CONTROL_MODE0 0x00
#define PIT_CONTROL_MODE2 0x04
#define PIT_CONTROL_BINARYCOUNTER 0x00
#define PIT_CONTROL_BCDCOUNTER 0x01

#define PIT_COUNTER0_ONCE   (PIT_PORT_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER)

#define PIT_COUNTER0_PERIODIC (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)

void kInitializePIT(WORD wCount, BOOL bPeriodic) {
    // PIT 컨트롤 레지스터(0x43)에 값을 초기화하여 카운트를 멈춘 뒤
    // 모드 0 설정
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

    if(bPeriodic == TRUE)
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    
    // 카운터 0(0x40)에 LSB -> MSB 순으로 카운터 초기 값 설정
    kOutPortByte(PIT_PORT_COUNTER0, wCount);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

#endif