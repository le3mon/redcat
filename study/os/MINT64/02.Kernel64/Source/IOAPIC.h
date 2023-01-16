#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include "Types.h"

// 매크로
// I/O APIC 레지스터 오프셋 관련 매크로
#define IOAPIC_REGISTER_IOREGISTERSELECTOR              0X00
#define IOAPIC_REGISTER_IOWINDOW                        0X10

// 위 두 레지스터로 접근할 때 사용하는 레지스터 인덱스
#define IOAPIC_REGISTERINDEX_IOAPICID                   0X00
#define IOAPIC_REGISTERINDEX_IOAPICVERSION              0X01
#define IOAPIC_REGISTERINDEX_IOAPICARBID                0X02
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE      0X10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE     0X11

// I/O 리다이렉션 테이블의 최대 개수
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT               24

// 인터럽트 마스크 관련 매크로
#define IOAPIC_INTERRUPT_MASK                           0X01

// 트리거 모드 관련 매크로
#define IOAPIC_TRIGGERMODE_LEVEL                        0X80
#define IOAPIC_TRIGGERMODE_EDGE                         0X00

// 리모트 IRR 관련 매크로
#define IOAPIC_REMOTEIRR_EOI                            0X40
#define IOAPIC_REMOTEIRR_ACCEPT                         0X00

// 인터럽트 입력 핀 극성 관련 매크로
#define IOAPIC_POLARITY_ACTIVELOW                       0X20
#define IOAPIC_POLARITY_ACTIVEHIGH                      0X00

// 전달 상태 관련 매크로
#define IOAPIC_DELIFVERYSTATUS_SENDPENDING              0X10
#define IOAPIC_DELIFVERYSTATUS_IDLE                     0X00

// 목적지 모드 관련 매크로
#define IOAPIC_DESTINATIONMODE_LOGICALMODE              0X08
#define IOAPIC_DESTINATIONMODE_PHYSICALMODE             0X00

// 전달 모드 관련 매크로
#define IOAPIC_DELIVERYMODE_FIXED                       0X00
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY              0X01
#define IOAPIC_DELIVERYMODE_SMI                         0X02
#define IOAPIC_DELIVERYMODE_NMI                         0X04
#define IOAPIC_DELIVERYMODE_INIT                        0X05
#define IOAPIC_DELIVERYMODE_EXTINT                      0X07

// IRQ를 I/O APIC의 인터럽트 입력 핀으로 대응시키는 테이블 최대 크기
#define IOAPIC_MAXIRQTOINTINMAPCOUNT                    16

// 구조체

#pragma pack(push, 1)

// I/O 리다이렉션 테이블의 자료구조
typedef struct kIORedirectionTableStruct {
    // 인터럽트 벡터
    BYTE bVector;

    // 트리거 모두, 리모트 IRR, 인터럽트 입력 핀 극성, 전달 상태, 목적지 모드, 전달 모드를 담당하는 필드
    BYTE bFlagsAndDeliveryMode;

    // 인터럽트 마스크
    BYTE bInterruptMask;

    // 예약된 영역
    BYTE vbReserved[4];

    // 인터럽트 전달할 APIC ID
    BYTE bDestination;
} IOREDIRECTIONTABLE;

// I/O APIC를 관리하는 자료구조
typedef struct kIOAPICManagerStruct {
    // ISA 버스가 연결된 I/O APIC의 메모리 맵 어드레스
    QWORD qwIOAPICBaseAddressOfISA;

    // IRQ와 I/O APIC 인터럽트 입력 핀 간의 연결 관계를 저장하는 테이블
    BYTE vbIRQToINTINMap[IOAPIC_MAXIRQTOINTINMAPCOUNT];
} IOAPICMANAGER;

#pragma pack(pop)


// 함수
QWORD kGetIOAPICBaseAddressOfISA(void);
void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE *pstEntry, BYTE bAPICID, BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector);
void kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE *pstEntry);
void kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE *pstEntry);
void kMaskAllInterruptInIOAPIC(void);
void kInitializeIORedirectionTable(void);
void kPrintIRQToINTINMap(void);

#endif