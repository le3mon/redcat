#include "task_struct.h"
#include "../MINT64/02.Kernel64/Source/Utility.h"
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