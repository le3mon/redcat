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
TCB *kCreateTask(QWORD qwFlags, void *pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress) {
    TCB *pstTask, *pstProcess;
    void *pvStackAddress;

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));
    
    pstTask = kAllocateTCB();
    if(pstTask == NULL) {
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        return NULL;
    }

    // 현재 프로세스 또는 스레드가 속한 프로세스 검새
    pstProcess = kGetProcessByThread(kGetRunningTask());
    // 만약 프로세스가 없다면 아무런 작업도 하지 않음
    if(pstProcess == NULL) {
        kFreeTCB(pstTask->stLink.qwID);
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        return NULL;
    }

    // 스레드를 생성할 경우 내가 속한 프로세스의 자식 스레드 리스트에 연결
    if(qwFlags & TASK_FLAGS_THREAD) {
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
        pstTask->qwMemorySize = pstProcess->qwMemorySize;

        kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));
    }
    else { // 프로세스는 파라미터로 넘어온 값 그대로 설정
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pvMemoryAddress;
        pstTask->qwMemorySize = qwMemorySize;
    }

    // 스레드의 ID를 태스크 ID와 동일하게 서렂ㅇ
    pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    
    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

    // TCB 설정 후 준비 리스트에 삽입
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    // 자식 스레드 리스트 초기화
    kInitializeList(&(pstTask->stChildThreadList));

    // FPU 사용 여부를 사용하지 않은 것으로 초기화
    pstTask->bFPUUsed = FALSE;

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    kAddTaskToReadyList(pstTask);

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

    return pstTask;
}

static void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void *pvStackAddress, QWORD qwStackSize) {
    // 콘텍스트 초기화
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // rsp, rbp 레지스터 설정
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

    // 리턴 어드레스 영역에 kExitTask() 함수의 어드레스 삽입하여 종료 시 kExitTask() 함수로 이동하도록 설정
    *(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

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
    TCB *pstTask;
    
    // 태스크 풀 초기화
    kInitializeTCBPool();

    // 준비 리스트, 우선순위별 실행 횟수 초기화
    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));    
        gs_stScheduler.viExecuteCount[i] = 0;
    }

    // 대기 리스트 초기화
    kInitializeList(&(gs_stScheduler.stWaitList));

    // TCB를 할당받아 부팅을 수행한 태스크를 커널 최초의 프로세스로 설정
    pstTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask = pstTask;
    pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
    pstTask->qwParentProcessID = pstTask->stLink.qwID;
    pstTask->pvMemoryAddress = (void*)0x100000;
    pstTask->qwMemorySize = 0x500000;
    pstTask->pvStackAddress = (void*)0x600000;
    pstTask->qwStackSize = 0x100000;

    // 프로세서 사용률 계산 시 사용하는 자료구조 초기화
    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;

    // FPU를 사용한 태스크 ID를 유효하지 않은 값으로 초기화
    gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;

    // 스핀락 초기화
    kInitializeSpinLock(&(gs_stScheduler.stSpinLock));
}

// 현재 수행중인 태스크 설정
void kSetRunningTask(TCB *pstTask) {

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    gs_stScheduler.pstRunningTask = pstTask;

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
}

// 현재 수행 중인 태스크 반환
TCB *kGetRunningTask(void) {
    TCB *pstRunningTask;

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    pstRunningTask = gs_stScheduler.pstRunningTask;

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

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
    if(bPriority == TASK_FLAGS_WAIT) {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);
        return TRUE;
    }
    else if(bPriority >= TASK_MAXREADYLISTCOUNT)
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

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    TCB *pstTarget;

    if(bPriority > TASK_MAXREADYLISTCOUNT)
        return FALSE;
    
    kLockForSpinLock(&(gs_stScheduler.stSpinLock));
        
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID)
        SETPRIORITY(pstTarget->qwFlags, bPriority);
    else {
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL)
                SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
        else {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
            kAddTaskToReadyList(pstTarget);
        }
    }
    
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

    return TRUE;
}

// 다른 태스크 찾아서 전환, 인터럽트나 예외 발생 불가
void kSchedule(void) {
    TCB *pstRunningTask, *pstNextTask;
    BOOL bPreviousInterrupt;

    if(kGetReadyTaskCount() < 1) {
        return ;
    }
    
    // 전환 도중 인터럽트 방지
    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // 임계 영역 시작
    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        kSetInterruptFlag(bPreviousInterrupt);
        return ;
    }

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;

    // 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
    if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }

    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        kSwitchContext(NULL, &(pstNextTask->stContext));
    }
    else {
        kAddTaskToReadyList(pstRunningTask);
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    kSetInterruptFlag(bPreviousInterrupt);
}

// 인터럽트 발생 시 다른 태스크로 전환
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char *pcContextAddress;

    // 임계 영역 시작
    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
        return FALSE;
    }
        
    // 태스크 전환 처리
    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;


    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    else {
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

    // 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
    if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사하여
    // 자동으로 태스크 전환이 되도록 만듬
    // gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

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

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

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
            kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
            return TRUE;
        }

        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }
    
    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    return TRUE;
}

void kExitTask(void) {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(void) {
    int iTotalCount = 0;
    int i;

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
    return iTotalCount;
}

int kGetTaskCount(void) {
    int iTotalCount;

    iTotalCount = kGetReadyTaskCount();

    kLockForSpinLock(&(gs_stScheduler.stSpinLock));

    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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

// 스레드가 소속된 프로세스를 반환
static TCB *kGetProcessByThread(TCB *pstThread) {
    TCB *pstProcess;

    // 만약 내가 프로세스라면 자신을 반환
    if(pstThread->qwFlags & TASK_FLAGS_PROCESS) {
        return pstThread;
    }

    // 프로세스가 아니라면 부모 프로세스로 설정된 태스크 ID를 통해 TCB 풀에서 태스크 자료구조 추출
    pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

    // 만약 프로세스가 없거나, 태스크 ID가 일치하지 않으면 널 반환
    if((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)) {
        return NULL;
    }

    return pstProcess;
}

void kIdleTask(void) {
    TCB *pstTask, *pstChildThread, *pstProcess;
    QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    int i, iCount;
    QWORD qwTaskID;
    void *pstThreadLink;

    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(1) {
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
            gs_stScheduler.qwProcessorLoad = 0;
        
        else {
            gs_stScheduler.qwProcessorLoad = 100 - ( qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask ) * 100 / ( qwCurrentMeasureTickCount - qwLastMeasureTickCount );
        }

        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        kHaltProcessorByLoad();

        if(kGetListCount(&(gs_stScheduler.stWaitList)) >= 0) {
            while(1) {
                kLockForSpinLock(&(gs_stScheduler.stSpinLock));

                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if(pstTask == NULL) {
                    kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
                    break;
                }

                if(pstTask->qwFlags & TASK_FLAGS_PROCESS) {
                    // 자식 스레드가 존재하면 모두 종료하고 다시 자식 스레드 리스트에 삽입
                    iCount = kGetListCount(&(pstTask->stChildThreadList));
                    for(i = 0; i < iCount; i++) {
                        // 스레드 링크의 어드레스에서 꺼내 스레드 종료
                        pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
                        if(pstThreadLink == NULL) {
                            break;
                        }

                        // 자식 스레드 리스트에 연결된 정보는 태스크 자료구조에 있는 stThreadLink의
                        // 시작 어드레스이므로, 태스크 자료구조의 시작 어드레스를 구하기 위해 별도 계산 필요
                        pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);

                        // 다시 자식 스레드 리스트에 삽입하여 해당 스레드가 종료될 때
                        // 자식 스레드가 프로세스를 찾아 스스로 리스트에서 제거하도록 함
                        kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

                        // 자식 스레드를 찾아서 종료
                        kEndTask(pstChildThread->stLink.qwID);
                    }

                    // 아직 자식 스레드가 남아있다면 종료될 때까지 기다려야 하므로 대기 리스트에 삽입
                    if(kGetListCount(&(pstTask->stChildThreadList)) > 0) {
                        kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);

                        kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
                        continue;
                    }
                    // 프로세스를 종료해야 하므로 할당받은 메모리 영역 삭제
                    else {
                        // 추후 코드 삽입
                    }
                }
                else if(pstTask->qwFlags & TASK_FLAGS_THREAD) {
                    // 스레드라면 프로세스의 자식 스레드 리스트에서 제거
                    pstProcess = kGetProcessByThread(pstTask);
                    if(pstProcess != NULL) {
                        kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);
                    }
                }
                
                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);
                kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
                
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

QWORD kGetLastFPUUsedTaskID(void) {
    return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID) {
    gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}