#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"
#include "MultiProcessor.h"

// GDT & TSS

// GDT 테이블 초기화
void kInitializeGDTTableAndTSS(void) {
    GDTR *pstGDTR;
    GDTENTRY8 *pstEntry;
    TSSSEGMENT *pstTSS;
    int i;
    
    // GDTR 설정
    pstGDTR = (GDTR*) GDTR_STARTADDRESS;
    pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
    pstGDTR->wLimit = GDT_TABLESIZE - 1;
    pstGDTR->qwBaseAddress = (QWORD)pstEntry;

    // TSS 영역 설정
    pstTSS = (TSSSEGMENT*)((QWORD)pstEntry + GDT_TABLESIZE);
    
    // NULL, 64비트 Code/Data, TSS 총 4개 세그먼트 생성
    kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);
    kSetGDTEntry8(&(pstEntry[1]), 0, 0xFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
    kSetGDTEntry8(&(pstEntry[2]), 0, 0xFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
    
    // 16개 코어 지원을 위해 16개의 TSS 디스크립터 생성
    for(i = 0; i < MAXPROCESSORCOUNT; i++) {
        // TSS는 16바이트이므로 kSetGDTEntry16() 함수 사용
        // pstEntry는 8바이트이므로 2개를 합쳐서 하나로 사용
        kSetGDTEntry16((GDTENTRY16*) &(pstEntry[GDT_MAXENTRY8COUNT + 
        (i * 2)]), (QWORD)pstTSS + (i * sizeof(TSSSEGMENT)),
        sizeof(TSSSEGMENT) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);
    }

    kInitializeTSSSegment(pstTSS);
}

// 8바이크 크기의 GDT 엔트리에 값을 설정
// 코드와 데이터 세그머트 디스크립터 설정 시 사용
void kSetGDTEntry8(GDTENTRY8 *pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
    pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
    pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}

// 16바이트 크기 GDT 엔트리 값 설정
// TSS 세그먼트 설정 시 사용
void kSetGDTEntry16(GDTENTRY16 *pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
    pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
    pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}

void kInitializeTSSSegment(TSSSEGMENT *pstTSS) {
    int i;

    // 최대 프로세서 또는 코어 수만큼 루프를 돌면서 생성
    for(i = 0; i < MAXPROCESSORCOUNT; i++) {
        kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));

        // IST의 뒤에서부터 잘라서 할당함
        pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * i);

        // IO TSS의 limit 값보다 크게 설정하여 I/O Map을 사용하지 않도록 설정
        pstTSS->wIOMapBaseAddress = 0xFFFF;

        // 다음 엔트리로 이동
        pstTSS++;
    }
}

// IDT
// IDT 테이블 초기화
void kInitalizeIDTTables(void) {
    IDTR *pstIDTR;
    IDTENTRY *pstEntry;
    int i;

    // IDTR 시작 어드레스
    pstIDTR = (IDTR*)IDTR_STARTADDRESS;

    // IDT 테이블 정보 생성
    pstEntry = (IDTENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
    pstIDTR->qwBaseAddress = (QWORD)pstEntry;
    pstIDTR->wLimit = IDT_TABLESIZE - 1;

    // 예외 ISR 등록
    kSetIDTEntry(&(pstEntry[0]), kISRDivideError, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[1]), kISRDebug, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[2]), kISRNMI, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[3]), kISRBreakPoint, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[4]), kISROverflow, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[5]), kISRBoundRangeExceeded, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[6]), kISRInvalidOpcode, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[7]), kISRDeviceNotAvailable, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[8]), kISRDoubleFault, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[9]), kISRCoprocessorSegmentOverrun, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[10]), kISRInvalidTSS, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[11]), kISRSegmentNotPresent, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[12]), kISRStackSegmentFault, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[13]), kISRGeneralProtection, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[14]), kISRPageFault, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[15]), kISR15, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[16]), kISRFPUError, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[17]), kISRAlignmentCheck, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[18]), kISRMachineCheck, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[19]), kISRSIMDError, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[20]), kISRETCException, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

    for(i = 21; i < 32; i++)
        kSetIDTEntry(&(pstEntry[i]), kISRETCException, 0x08, IDT_FLAGS_IST1,
         IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

    // 인터럽트 ISR 등록
    kSetIDTEntry(&(pstEntry[32]), kISRTimer, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[33]), kISRKeyboard, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[34]), kISRSlavePIC, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[35]), kISRSerial2, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[36]), kISRSerial1, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[37]), kISRParallel2, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[38]), kISRFloppy, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[39]), kISRParallel1, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[40]), kISRRTC, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[41]), kISRReserved, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[42]), kISRNotUsed1, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[43]), kISRNotUsed2, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[44]), kISRMouse, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[45]), kISRCoprocessor, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[46]), kISRHDD1, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[47]), kISRHDD2, 0x08, IDT_FLAGS_IST1,
     IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

    for(i = 48; i < IDT_MAXENTRYCOUNT; i++)
        kSetIDTEntry(&(pstEntry[i]), kISRETCInterrupt, 0x08, IDT_FLAGS_IST1,
         IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
}

// IDT 게이트 디스크립터에 값 설정
void kSetIDTEntry(IDTENTRY *pstEntry, void *pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType) {
    pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xFFFF;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x3;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xFFFF;
    pstEntry->dwUpperBaseAddress = (QWORD)pvHandler >> 32;
    pstEntry->dwReserved = 0;
}