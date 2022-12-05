#include "list_struct.h"
#include "../MINT64/02.Kernel64/Source/Task.h"

#define TASK_TCBPOOLADDRESS 0x800000
#define TASK_MAXCOUNT       1024

static TCBPOOLMANAGER gs_stTCBPoolManager;

void kInitializeTCBPool(void) {
    int i;

    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(TASK_MAXCOUNT));

    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    for(i = 0; i < TASK_MAXCOUNT; i++) {
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }   
}

TCB *kAllocateTCB(void) {
    TCB *pstEmptyTCB;
    int i;

    if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount)
        return NULL;
    
    for(i = 0; i < gs_stTCBPoolManager.iMaxCount; i++) {
        // ID 상위 32비트가 0이면 할당되지 않은 TCB
        if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
            pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
            break;
        }
    }

    pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if(gs_stTCBPoolManager.iAllocatedCount == 0)
        gs_stTCBPoolManager.iAllocatedCount = 1;

    return pstEmptyTCB;
}

void kFreeTCB(QWORD qwID) {
    int i;

    i = qwID & 0xFFFFFFFF;

    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i]), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    
    gs_stTCBPoolManager.iUseCount--;
}