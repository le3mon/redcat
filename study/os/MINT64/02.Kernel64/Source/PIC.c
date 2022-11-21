#include "PIC.h"
#include "AssemblyUtility.h"

void kInitializePIC(void) {
    // 마스터 PIC 컨트롤러 초기화
    kOutPortByte(PIC_MASTER_PORT1, 0x11); // LTIM 비트 = 0, SNGL 비트 = 0, IC4 비트 = 1
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR); // 인터럽트 벡터를 0x20(32)부터 차례대로 할당
    kOutPortByte(PIC_MASTER_PORT2, 0x04); // 슬레이브 PIC 컨트롤러가 마스터 PIC 컨트롤러의 2번째 핀에 연결됨
    kOutPortByte(PIC_MASTER_PORT2, 0x01); // uPM 비트 = 1

    // 슬레이브 PIC 컨트롤러 초기화
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask) {
    kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}

void kSendEOIToPIC(int iIRQNumber) {
    // 마스터 PIC 컨트롤러에 EOI 전송
    // OCW2(포트 0x20), EOI 비트(비트 5) = b100000 = 0x20
    kOutPortByte(PIC_MASTER_PORT1, 0x20);
    if(iIRQNumber >= 8)
        kOutPortByte(PIC_SLAVE_PORT1, 0x20);
}