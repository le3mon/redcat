#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"
#include "Synchronization.h"

// 스케줄러 관련 자료구조
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

// 태스크 풀과 태스크 관련
// 태스크 풀 초기화
static void kInitializeTCBPool(void) {
    int i;

    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));
    
    // 태스크 풀의 어드레스를 지정하고 초기화
    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    // TCB에 ID 할당
    for(i = 0; i < TASK_MAXCOUNT; i++)
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    // TCB 최대 개수와 할당된 횟수 초기화
    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

// TCB 할당 받음
static TCB *kAllocateTCB(void) {
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

// TCB 해제
static void kFreeTCB(QWORD qwID) {
    int i;

    i = GETTCBOFFSET(qwID);

    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

// 태스크 생성
TCB *kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB *pstTask;
    void *pvStackAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();
    pstTask = kAllocateTCB();
    if(pstTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }
    kUnlockForSystemData(bPreviousFlag);
        
    
    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

    // TCB 설정 후 준비 리스트에 삽입
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    bPreviousFlag = kLockForSystemData();

    kAddTaskToReadyList(pstTask);

    kUnlockForSystemData(bPreviousFlag);

    return pstTask;
}

static void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void *pvStackAddress, QWORD qwStackSize) {
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
    // pstTCB->qwID = qwID;
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

// 스케줄러 관련

// 스케줄러 초기화
void kInitializeScheduler(void) {
    int i;
    
    // 태스크 풀 초기화
    kInitializeTCBPool();

    // 준비 리스트, 우선순위별 실행 횟수 초기화
    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));    
        gs_stScheduler.viExecuteCount[i] = 0;
    }

    // 대기 리스트 초기화
    kInitializeList(&(gs_stScheduler.stWaitList));    
    

    // TCB를 할당받아 실행 중인 태스크로 설정하여, 부팅을 수행한 태스클 저장할 TCB 준비
    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    // 프로세서 사용률 계산 시 사용하는 자료구조 초기화
    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;
}

// 현재 수행중인 태스크 설정
void kSetRunningTask(TCB *pstTask) {
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    gs_stScheduler.pstRunningTask = pstTask;

    kUnlockForSystemData(bPreviousFlag);
}

// 현재 수행 중인 태스크 반환
TCB *kGetRunningTask(void) {
    BOOL bPreviousFlag;
    TCB *pstRunningTask;

    bPreviousFlag = kLockForSystemData();

    pstRunningTask = gs_stScheduler.pstRunningTask;

    kUnlockForSystemData(bPreviousFlag);

    return pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
static TCB *kGetNextTaskToRun(void) {
    TCB *pstTarget = NULL;
    int iTaskCount, i, j;

    for(j = 0; j < 2; j++) {
        for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

            if(gs_stScheduler.viExecuteCount[i] < iTaskCount) {
                pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
                gs_stScheduler.viExecuteCount[i]++;
                break;
            }
            else {
                gs_stScheduler.viExecuteCount[i] = 0;
            }
        }

        if(pstTarget != NULL)
            break;
    }

    return pstTarget;
}

// 태스크 스케줄러의 준비 리스트에 삽입
static BOOL kAddTaskToReadyList(TCB *pstTask) {
    BYTE bPriority;

    bPriority = GETPRIORITY(pstTask->qwFlags);
    if(bPriority >= TASK_MAXREADYLISTCOUNT)
        return FALSE;
    
    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
    return TRUE;
}

static TCB *kRemoveTaskFromReadyList(QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;

    if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT)
        return NULL;
    
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if(pstTarget->stLink.qwID != qwTaskID)
        return NULL;
    
    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
    return pstTarget;
}

BOOL kChangePriority(QWORD qwID, BYTE bPriority) {
    TCB *pstTarget;
    BOOL bPreviousFlag;

    if(bPriority > TASK_MAXREADYLISTCOUNT)
        return FALSE;
    
    bPreviousFlag = kLockForSystemData();
        
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwID)
        SETPRIORITY(pstTarget->qwFlags, bPriority);
    else {
        pstTarget = kRemoveTaskFromReadyList(qwID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
            if(pstTarget != NULL)
                SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
        else {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
            kAddTaskToReadyList(pstTarget);
        }
    }
    
    kUnlockForSystemData(bPreviousFlag);

    return TRUE;
}

// 다른 태스크 찾아서 전환, 인터럽트나 예외 발생 불가
void kSchedule(void) {
    TCB *pstRunningTask, *pstNextTask;
    BOOL bPreviousFlag;

    if(kGetReadyTaskCount() < 1)
        return ;
    
    bPreviousFlag = kLockForSystemData();

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return ;
    }

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;

    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kSwitchContext(NULL, &(pstNextTask->stContext));
    }
    else {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    kUnlockForSystemData(bPreviousFlag);

}

// 인터럽트 발생 시 다른 태스크로 전환
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char *pcContextAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return FALSE;
    }
        
    // 태스크 전환 처리
    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    else {
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kUnlockForSystemData(bPreviousFlag);

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사하여
    // 자동으로 태스크 전환이 되도록 만듬
    // gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    return TRUE;
}

// 프로세서 사용 시간 하나 줄임
void kDecreaseProcessorTime(void) {
    if(gs_stScheduler.iProcessorTime > 0)
        gs_stScheduler.iProcessorTime--;
}

// 프로세서 사용할 수 있는 시간이 다 되었는지 여부 반환
BOOL kIsProcessorTimeExpired(void) {
    if(gs_stScheduler.iProcessorTime <= 0)
        return TRUE;
    
    return FALSE;
}

BOOL kEndTask(QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSystemData(bPreviousFlag);

        kSchedule();

        while(1);
    }
    else {
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL) {
                pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
                SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
            }
            kUnlockForSystemData(bPreviousFlag);
            return FALSE;
        }

        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }
    
    return TRUE;
}

void kExitTask(void) {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(void) {
    int iTotalCount = 0;
    int i;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}

int kGetTaskCount(void) {
    int iTotalCount;
    BOOL bPreviousFlag;

    iTotalCount = kGetReadyTaskCount();

    bPreviousFlag = kLockForSystemData();

    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}

TCB *kGetTCBInTCBPool(int iOffset) {
    if((iOffset < -1) || (iOffset > TASK_MAXCOUNT))
        return NULL;
    
    return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist(QWORD qwID) {
    TCB *pstTCB;

    pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

    if((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID))
        return FALSE;
    
    return TRUE;
}

QWORD kGetProcessorLoad(void) {
    return gs_stScheduler.qwProcessorLoad;
}

void kIdleTask(void) {
    TCB *pstTask;
    QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    BOOL bPreviousFlag;
    QWORD qwTaskID;

    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(1) {
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
            gs_stScheduler.qwProcessorLoad = 0;
        
        else {
            gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        kHaltProcessorByLoad();

        if(kGetListCount(&(gs_stScheduler.stWaitList)) >= 0) {
            while(1) {
                bPreviousFlag = kLockForSystemData();

                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if(pstTask == NULL) {
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }
                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);
                kUnlockForSystemData(bPreviousFlag);
                
                kPrintf("IDLE: TASK ID[0x%q] is completely ended.\n", qwTaskID);
            }
        }
        
        kSchedule();
    }
}

void kHaltProcessorByLoad(void) {
    if(gs_stScheduler.qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    }
    else if(gs_stScheduler.qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    }
    else if(gs_stScheduler.qwProcessorLoad < 95) {
        kHlt();
    }
}