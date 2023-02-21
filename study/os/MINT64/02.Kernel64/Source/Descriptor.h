#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"
#include "MultiProcessor.h"

// 매크로
// GDT
#define GDT_TYPE_CODE           0X0A
#define GDT_TYPE_DATA           0X02
#define GDT_TYPE_TSS            0X09
#define GDT_FLAGS_LOWER_S       0X10
#define GDT_FLAGS_LOWER_DPL0    0X00
#define GDT_FLAGS_LOWER_DPL1    0X20
#define GDT_FLAGS_LOWER_DPL2    0X40
#define GDT_FLAGS_LOWER_DPL3    0X60
#define GDT_FLAGS_LOWER_P       0X80
#define GDT_FLAGS_UPPER_L       0X20
#define GDT_FLAGS_UPPER_DB      0X40
#define GDT_FLAGS_UPPER_G       0X80

// 실제로 사용할 매크로
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS (GDT_FLAGS_UPPER_G)

// Lower Flags는 Code/Data/TSS, DPL 0/3, Present로 설정
// 커널 레벨 코드/데이터 세그먼트 디스크립터
#define GDT_FLAGS_LOWER_KERNELCODE  (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA  (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

// 유저 레벨 코드/데이터 세그먼트 디스크립터
#define GDT_FLAGS_LOWER_USERCODE    (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA    (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

// TSS 세그먼트 디스크립터
#define GDT_FLAGS_LOWER_TSS         (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

// 세그먼트 디스크립터 오프셋
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_USERDATASEGMENT   0x18
#define GDT_USERCODESEGMENT   0x20
#define GDT_TSSSEGMENT        0x18

// 세그먼트 셀렉터에 설정할 RPL
#define SELECTOR_RPL_0      0x00
#define SELECTOR_RPL_3      0x03

// 기타 GDT 관련 매크로
// GDTR 시작 어드레스, 1MB에서 264KB까지는 페이지 테이블 영역
#define GDTR_STARTADDRESS   0X14200
// 8바이트 엔트리 개수(널, 디스크립터/커널코드/커널데이터)
#define GDT_MAXENTRY8COUNT  5
// 16바이트 엔트리 개수(TSS)
#define GDT_MAXENTRY16COUNT (MAXPROCESSORCOUNT)
// GDT 테이블 크기
#define GDT_TABLESIZE ((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE (sizeof(TSSSEGMENT) * MAXPROCESSORCOUNT)

// IDT
// 기본 매크로
#define IDT_TYPE_INTERRUPT      0X0E
#define IDT_TYPE_TRAP           0X0F
#define IDT_FLAGS_DPL0          0X00
#define IDT_FLAGS_DPL1          0X20
#define IDT_FLAGS_DPL2          0X40
#define IDT_FLAGS_DPL3          0X60
#define IDT_FLAGS_P             0X80
#define IDT_FLAGS_IST0          0
#define IDT_FLAGS_IST1          1

// 실제로 사용할 매크로
#define IDT_FLAGS_KERNEL        (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER          (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// 기타 IDT 관련 매크로
// IDT 엔트리 개수
#define IDT_MAXENTRYCOUNT       100

// IDTR의 시작 어드레스, TSS 세그먼트 뒤쪽에 위치
#define IDTR_STARTADDRESS       (GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)

// IDT 테이블의 시작 어드레스
#define IDT_STARTADDRESS        (IDTR_STARTADDRESS + sizeof(IDTR))

// IDT 테이블 전체 크기
#define IDT_TABLESIZE           (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

// IST의 시작 어드레스
#define IST_STARTADDRESS        0X700000
// IST 크기
#define IST_SIZE                0X100000

#pragma pack(push, 1)

typedef struct kGDTRStruct {
    WORD wLimit;
    QWORD qwBaseAddress;
    WORD wPading;
    DWORD dwPading;
} GDTR, IDTR;

typedef struct kGDTEntry8Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bUpperBaseAddress1;
    BYTE bTypeAndLowerFlag;
    BYTE bUpperLimitAndUpperFlag;
    BYTE bUpperBaseAddress2;
} GDTENTRY8;

typedef struct kGDTEntry16Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bMiddleBaseAddress1;
    BYTE bTypeAndLowerFlag;
    BYTE bUpperLimitAndUpperFlag;
    BYTE bMiddleBaseAddress2;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} GDTENTRY16;

typedef struct kTSSDataStruct {
    DWORD dwReserved1;
    QWORD qwRsp[3];
    QWORD qwReserved2;
    QWORD qwIST[7];
    QWORD qwReserved3;
    WORD wReserved;
    WORD wIOMapBaseAddress;
} TSSSEGMENT;

typedef struct kIDTEntryStruct {
    WORD wLowerBaseAddress;
    WORD wSegmentSelector;
    BYTE bIST;
    BYTE bTypeAndFlags;
    WORD wMiddleBaseAddress;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} IDTENTRY;

#pragma pack(pop)

// 함수
void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8 *pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16 *pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT *pstTSS);

void kInitalizeIDTTables(void);
void kSetIDTEntry(IDTENTRY *pstEntry, void *pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);

#endif