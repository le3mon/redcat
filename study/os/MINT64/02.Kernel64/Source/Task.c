#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

void kSetUpTask(TCB *pstTCB, QWORD qwID, QWORD qwFlags, QWORD qwEntryPointAddress, void *pvStackAddress, QWORD qwStackSize) {
    // 콘텍스트 초기화
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // rsp, rbp 레지스터 설정
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize;

    // 세그먼트 셀렉터 설정
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

    // rip 레지스터와 인터럽트 플래그 설정
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

    // rflags 레지스터의 if 비트를 1로 설정
    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

    // id, stack, flag 설정
    pstTCB->qwID = qwID;
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}