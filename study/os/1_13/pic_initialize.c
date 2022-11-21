#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"

#define PIC_MASTER_PORT1    0x20
#define PIC_MASTER_PORT2    0x21
#define PIC_SLAVE_PORT1     0xA0
#define PIC_SLAVE_PORT2     0xA1

// IDT 테이블에서 인터럽트 벡터가 시작되는 위치, 0x20
#define PIC_IRQSTARTECTOR   0x20

void kInitializePIC(void) {
    // 마스터 PIC 컨트롤러 초기화
    kOutPortByte(PIC_MASTER_PORT1, 0x11); // LTIM 비트 = 0, SNGL 비트 = 0, IC4 비트 = 1
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTECTOR); // 인터럽트 벡터를 0x20(32)부터 차례대로 할당
    kOutPortByte(PIC_MASTER_PORT2, 0x04); // 슬레이브 PIC 컨트롤러가 마스터 PIC 컨트롤러의 2번째 핀에 연결됨
    kOutPortByte(PIC_MASTER_PORT2, 0x01); // uPM 비트 = 1

    // 슬레이브 PIC 컨트롤러 초기화
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTECTOR + 8);
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);
    kOutPortByte(PIC_SLAVE_PORT1, 0x01);
}