#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"
#include "Synchronization.h"
#include "MultiProcessor.h"
#include "MPConfigurationTable.h"

// 스케줄러 관련 자료구조
static SCHEDULER gs_vstScheduler[MAXPROCESSORCOUNT];
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

    // 스핀락 초기화
    kInitializeSpinLock(&gs_stTCBPoolManager.stSpinLock);
}

// TCB 할당 받음
static TCB *kAllocateTCB(void) {
    TCB *pstEmptyTCB;
    int i;

    // 동기화 처리
    kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);

    if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) {
        kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
        return NULL;
    }

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
    
    // 동기호 처리
    kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);

    return pstEmptyTCB;
}

// TCB 해제
static void kFreeTCB(QWORD qwID) {
    int i;

    i = GETTCBOFFSET(qwID);

    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));

    // 동기화 처리
    kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);

    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;

    kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
}

// 태스크 생성
TCB *kCreateTask(QWORD qwFlags, void *pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity) {
    TCB *pstTask, *pstProcess;
    void *pvStackAddress;
    BYTE bCurrentAPICID;

    // 현재 코어의 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();
    
    pstTask = kAllocateTCB();
    if(pstTask == NULL) {
        return NULL;
    }

    // 임계 영역 시작
    kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

    // 현재 프로세스 또는 스레드가 속한 프로세스 검새
    pstProcess = kGetProcessByThread(kGetRunningTask(bCurrentAPICID));
    // 만약 프로세스가 없다면 아무런 작업도 하지 않음
    if(pstProcess == NULL) {
        kFreeTCB(pstTask->stLink.qwID);
        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
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

    // 스레드의 ID를 태스크 ID와 동일하게 설정
    pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
    
    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

    // TCB 설정 후 준비 리스트에 삽입
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    // 자식 스레드 리스트 초기화
    kInitializeList(&(pstTask->stChildThreadList));

    // FPU 사용 여부를 사용하지 않은 것으로 초기화
    pstTask->bFPUUsed = FALSE;

    // 현재 코어의 로컬 APIC ID를 태스크에 설정
    pstTask->bAPICID = bCurrentAPICID;

    // 프로세서 친화도 설정
    pstTask->bAffinity = bAffinity;

    // 부하 분산을 고려하여 스케줄러에 태스크 추가
    kAddTaskToSchedulerWithLoadBalancing(pstTask);
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
    int i, j;
    BYTE bCurrentAPICID;
    TCB *pstTask;
    
    // 현재 코어의 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();

    // Bootstrap Processor만 태스크 풀과 스케줄러 자료구조 모두 초기화
    if(bCurrentAPICID == 0) {
        // 태스크 풀 초기화
        kInitializeTCBPool();

        // 준비 리스트, 우선순위별 실행 횟수 초기화
        for(j = 0; j < MAXPROCESSORCOUNT; j++) {
            // 준비 리스트 초기화
            for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            kInitializeList(&(gs_vstScheduler[j].vstReadyList[i]));    
            gs_vstScheduler[j].viExecuteCount[i] = 0;
            }

            // 대기 리스트 초기화
            kInitializeList(&(gs_vstScheduler[j].stWaitList));

            // 스핀락 초기화
            kInitializeSpinLock(&(gs_vstScheduler[j].stSpinLock));
        }
        
    }

    // TCB를 할당받아 부팅을 수행한 태스크를 커널 최초의 프로세스로 설정
    pstTask = kAllocateTCB();
    gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstTask;

    // BSP의 콘솔 셸이나 AP의 유휴 태스크는 모두 현재 코어에서만 실행하도록
    // 로컬 APIC ID와 프로세서 친화도를 현재 코어의 로컬 APIC ID로 설정
    pstTask->bAPICID = bCurrentAPICID;
    pstTask->bAffinity = bCurrentAPICID;

    // Bootstrap Processor는 콘솔 셸을 실행
    if(bCurrentAPICID == 0) {
        pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
    }
    // AP는 특별히 긴급한 태스크가 없으므로 유휴 태스크를 실행
    else {
        pstTask->qwFlags = TASK_FLAGS_LOWEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE;
    }

    pstTask->qwParentProcessID = pstTask->stLink.qwID;
    pstTask->pvMemoryAddress = (void*)0x100000;
    pstTask->qwMemorySize = 0x500000;
    pstTask->pvStackAddress = (void*)0x600000;
    pstTask->qwStackSize = 0x100000;

    // 프로세서 사용률 계산 시 사용하는 자료구조 초기화
    gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask = 0;
    gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;

    // FPU를 사용한 태스크 ID를 유효하지 않은 값으로 초기화
    gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
}

// 현재 수행중인 태스크 설정
void kSetRunningTask(BYTE bAPICID, TCB *pstTask) {
    kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    gs_vstScheduler[bAPICID].pstRunningTask = pstTask;

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
}

// 현재 수행 중인 태스크 반환
TCB *kGetRunningTask(BYTE bAPICID) {
    TCB *pstRunningTask;

    kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    pstRunningTask = gs_vstScheduler[bAPICID].pstRunningTask;

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    return pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
static TCB *kGetNextTaskToRun(BYTE bAPICID) {
    TCB *pstTarget = NULL;
    int iTaskCount, i, j;

    for(j = 0; j < 2; j++) {
        for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            iTaskCount = kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));

            if(gs_vstScheduler[bAPICID].viExecuteCount[i] < iTaskCount) {
                pstTarget = (TCB*)kRemoveListFromHeader(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
                gs_vstScheduler[bAPICID].viExecuteCount[i]++;
                break;
            }
            else {
                gs_vstScheduler[bAPICID].viExecuteCount[i] = 0;
            }
        }

        if(pstTarget != NULL)
            break;
    }

    return pstTarget;
}

// 태스크 스케줄러의 준비 리스트에 삽입
static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB *pstTask) {
    BYTE bPriority;

    bPriority = GETPRIORITY(pstTask->qwFlags);
    if(bPriority == TASK_FLAGS_WAIT) {
        kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTask);
        return TRUE;
    }
    else if(bPriority >= TASK_MAXREADYLISTCOUNT)
        return FALSE;
    
    kAddListToTail(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), pstTask);
    return TRUE;
}

// 준비 큐에서 태스크 제거
static TCB *kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;

    if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT)
        return NULL;
    
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if(pstTarget->stLink.qwID != qwTaskID)
        return NULL;
    
    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), qwTaskID);
    return pstTarget;
}

// 태스크가 포함된 스케줄러의 ID를 반환하고, 해당 스케줄러의 스핀락 잠금
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE *pbAPICID) {
    TCB *pstTarget;
    BYTE bAPICID;

    while(1) {
        // 태스크 ID로 태스크 자려구조를 찾아서 어느 스케줄러에서 실행중인지 확인
        pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
        if((pstTarget == NULL) || (pstTarget->stLink.qwID != qwTaskID)) {
            return FALSE;
        }

        // 현재 태스크가 실행되는 코어의 ID 확인
        bAPICID = pstTarget->bAPICID;

        // 임계 영역 시작
        kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

        // 스핀락 획득 후 같은 코어에서 실행되는지 다시 확인
        // 태스크가 수행되는 코어 찾은 후 정확하게 스핀락을 걸기 위해 이중 검사
        pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
        if(pstTarget->bAPICID == bAPICID) {
            break;
        }

        // 로컬 APIC ID 값이 스핀락 획득 전후 값이 다르다면,
        // 스핀락을 획득하는 동안 태스크가 다른 코어로 옮겨간 것임
        // 따라서 설정한 스핀락 해제 후 옮겨진 코어의 스핀락 설정
        kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    }

    *pbAPICID = bAPICID;
    return TRUE;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    TCB *pstTarget;
    BYTE bAPICID;

    if(bPriority > TASK_MAXREADYLISTCOUNT)
        return FALSE;
    
    // 태스크가 포함된 커어의 로컬 APIC ID를 찾은 후 스핀락 잠금
    if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
        return FALSE;
    }

    // 실행중인 태스크이면 우선순위만 변경
    // PIT 컨트롤러의 인터럽트가 발생하여 태스크 전환이 수행될 때 변경된 우선순위의 리스트로 이동
    pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID)
        SETPRIORITY(pstTarget->qwFlags, bPriority);
    else {
        pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL)
                SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
        else {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
            kAddTaskToReadyList(bAPICID, pstTarget);
        }
    }
    
    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    return TRUE;
}

// 다른 태스크 찾아서 전환, 인터럽트나 예외 발생 불가
BOOL kSchedule(void) {
    TCB *pstRunningTask, *pstNextTask;
    BOOL bPreviousInterrupt;
    BYTE bCurrentAPICID;

    // 전환 도중 인터럽트가 발생하여 태스크 전환이 일어나지 않도록 설정
    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    bCurrentAPICID = kGetAPICID();

    if(kGetReadyTaskCount(bCurrentAPICID) < 1) {
        kSetInterruptFlag(bPreviousInterrupt);
        return FALSE;
    }
    
    // 전환 도중 인터럽트 방지
    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // 임계 영역 시작
    kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

    pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
    if(pstNextTask == NULL) {
        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
        kSetInterruptFlag(bPreviousInterrupt);
        return ;
    }

    // 현재 수행 중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
    pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
    gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

    // 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += 
        TASK_PROCESSORTIME - gs_vstScheduler[bCurrentAPICID].iProcessorTime;

    // 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
    if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }

    // 프로세서 사용 시간 업데이트
    gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

    // 태스크 종료 플래그 설정 시 대기 리스트에 삽입 후 콘텍스트 전환
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
        kSwitchContext(NULL, &(pstNextTask->stContext));
    }
    else {
        kAddTaskToReadyList(bCurrentAPICID, pstRunningTask);
        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    kSetInterruptFlag(bPreviousInterrupt);
    return FALSE;
}

// 인터럽트 발생 시 다른 태스크로 전환
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char *pcContextAddress;
    BYTE bCurrentAPICID;
    QWORD qwISTStartAddress;

    // 현재 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();

    // 임계 영역 시작
    kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

    // 전환할 태스크가 없으면 종료
    pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
    if(pstNextTask == NULL) {
        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
        return FALSE;
    }

    // IST 끝부분부터 코어 0 -> 15 순으로 64KB씩 쓰고 있으므로, 로컬 APIC ID를
    // 이용해서 IST 어드레스 계산
    qwISTStartAddress = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * bCurrentAPICID);
    pcContextAddress = (char*)qwISTStartAddress - sizeof(CONTEXT);
    pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
    gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

    // 유휴 태스크에서 전환되었다면 사용한 틱 카운트 증가
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
    }

    // 태스크 종료 플래그가 설정되면 콘텍스트 저장하지 않고 대기 리스트에 삽입
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
    }
    // 태스크 종료되지 않으면 IST에 있는 콘텍스트 복사하고, 현재 태스크클 준비 리스트로 옮김
    else {
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
    }

    // 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
    if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }

    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));


    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사하여
    // 자동으로 태스크 전환이 되도록 만듬
    // gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    // 종료하는 태스크가 아니면 스케줄러에 태스크 추가
    if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK) {
        // 스케줄러에 태스크를 추가, 부하 분산을 고료
        kAddTaskToSchedulerWithLoadBalancing(pstRunningTask);
    }

    // 프로세서 사용 시간을 업데이트
    gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

    return TRUE;
}

// 프로세서 사용 시간 하나 줄임
void kDecreaseProcessorTime(BYTE bAPICID) {
    gs_vstScheduler[bAPICID].iProcessorTime--;
}

// 프로세서 사용할 수 있는 시간이 다 되었는지 여부 반환
BOOL kIsProcessorTimeExpired(BYTE bAPICID) {
    if(gs_vstScheduler[bAPICID].iProcessorTime <= 0)
        return TRUE;
    
    return FALSE;
}

BOOL kEndTask(QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;
    BYTE bAPICID;

    // 태스크가 포함된 코어의 로컬 APIC ID를 찾은 후 스핀락을 잠금
    if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
        return FALSE;
    }

    // 현재 실행 중인 태스크면 EndTask 비트를 설정하고 태스크를 전환
    pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

        // 현재 스케줄러에서 실행 중인 태스크의 경우만 아래를 적용
        if(kGetAPICID() == bAPICID) {
            kSchedule();

            // 태스크 전환되었으므로 아래 코드는 절대 실행되지 않음
            while(1) {
                ;
            }
        }
        return TRUE;
    }

    // 실행 중인 태스크가 아니면 준비 큐에서 직접 찾아서 대기 리스트에 연결
    // 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 태스크 종료 비트를 설정
    pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
    if(pstTarget == NULL) {
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
        if(pstTarget != NULL) {
            pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
            SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        }
        kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
        return TRUE;
    }

    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTarget);

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    return TRUE;
}

// 태스크가 자신을 종료
void kExitTask(void) {
    kEndTask(gs_vstScheduler[kGetAPICID()].pstRunningTask->stLink.qwID);
}

// 준비 큐에 있는 모든 태스크의 수를 반환
int kGetReadyTaskCount(BYTE bAPICID) {
    int iTotalCount = 0;
    int i;

    kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
    }

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    return iTotalCount;
}

// 전체 태스크의 수를 반환
int kGetTaskCount(BYTE bAPICID) {
    int iTotalCount;

    // 준비 큐의 태스크 수를 구한 후 대기 큐의 태스크 수와 현재 수행 중인 태스킄 수를 더함
    iTotalCount = kGetReadyTaskCount(bAPICID);

    kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

    iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].stWaitList)) + 1;

    kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
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

// 프로세서의 사용률을 반환
QWORD kGetProcessorLoad(BYTE bAPICID) {
    return gs_vstScheduler[bAPICID].qwProcessorLoad;
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

// 각 스케줄러의 태스크 수를 이용하여 적절한 스케줄러에 태스크 추가
// 부하 분산 기능을 사용하지 않은 경우 현재 코어에 삽입
// 부하 분산을 사용하지 않는 경우, 태스크가 현재 수행되는 코어에서 계속 수행하므로
// pstTask에는 적어도 APIC ID가 설정되어 있어야 함
void kAddTaskToSchedulerWithLoadBalancing(TCB *pstTask) {
    BYTE bCurrentAPICID;
    BYTE bTargetAPICID;

    // 태스크가 동작하던 코어의 APIC를 확인
    bCurrentAPICID = pstTask->bAPICID;

    // 부하 분산 기능을 사용하고, 프로세서 친화도가 모든 코어(0xFF)로 설정되면 부하 분산 실행
    if((gs_vstScheduler[bCurrentAPICID].bUseLoadBalancing == TRUE) && 
        (pstTask->bAffinity == TASK_LOADBALANCINGID)) {
        // 태스크를 추가할 스케줄러 선택
        bTargetAPICID = kFindSchedulerOfMinumumTaskCount(pstTask);
    }
    // 태스크 부하 분산 기능과 관계 없이 프로세서 친화도 필드에 다른 코어의 APIC ID가
    // 들어 있으면 해당 스케줄러로 이동
    else if((pstTask->bAffinity != bCurrentAPICID) && (pstTask->bAffinity != TASK_LOADBALANCINGID)) {
        bTargetAPICID = pstTask->bAffinity;
    }
    // 부하 분산 기능을 사용하지 않는 경우 현재 스케줄러에 대시 삽입
    else {
        bTargetAPICID = bCurrentAPICID;
    }

    kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

    // 태스크를 추가할 스케줄러가 현재 스케줄러와 다르다면 태스크를 이동
    // FPU는 공유되지 않으므로 현재 태스크가 FPU를 마지막으로 썼다면 FPU 콘텍스트를 메모리에 저장
    if((bCurrentAPICID != bTargetAPICID) && 
        (pstTask->stLink.qwID == gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID)) {
        // FPU를 저장하기 전에 TS 비트를 끄지 않으면, 예외 7이 발생하므로 주의
        kClearTS();
        kSaveFPUContext(pstTask->vqwFPUContext);
        gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
    }

    kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

    kLockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));

    // 태스크를 수행할 코어의 APIC ID를 설정하고, 해당 스케줄러에 태스크 삽입
    pstTask->bAPICID = bTargetAPICID;
    kAddTaskToReadyList(bTargetAPICID, pstTask);

    kUnlockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
}

// 태스크를 추가할 스케줄러의 ID를 반환
// 파라미터로 전달된 태스크 자료구조에는 적어도 플래그와 프로세서 친화도 필드가 채워져 있어야 함
static BYTE kFindSchedulerOfMinumumTaskCount(const TCB *pstTask) {
    BYTE bPriority;
    BYTE i;
    int iCurrentTaskCount;
    int iMinTaskCount;
    BYTE bMinCoreIndex;
    int iTempTaskCount;
    int iProcessorCount;

    // 코어 개수를 확인
    iProcessorCount = kGetProcessorCount();

    // 코어가 하나라면 현재 코어에서 계속 수행
    if(iProcessorCount == 1) {
        return pstTask->bAPICID;
    }

    // 우선순위 추출
    bPriority = GETPRIORITY(pstTask->qwFlags);

    // 태스크가 포함된 스케줄러에서 태스크와 같은 우선순위의 태스크 수를 확인
    iCurrentTaskCount = kGetListCount(&(gs_vstScheduler[pstTask->bAPICID].vstReadyList[bPriority]));

    // 나머지 코어에서 현재 태스크와 같은 레벨을 검사
    // 자신과 태스크 수가 적어도 2 이상 차이 나는 것 중에서 가장 태스크 수가 작은 스케줄러 ID 반환
    iMinTaskCount = TASK_MAXCOUNT;
    bMinCoreIndex = pstTask->bAPICID;
    for(i = 0; i < iProcessorCount; i++) {
        if(i == pstTask->bAPICID) {
            continue;
        }

        // 모든 스케줄러 돌면서 확인
        iTempTaskCount = kGetListCount(&(gs_vstScheduler[i].vstReadyList[bPriority]));

        // 현재 코어와 태스크 수가 2개 이상 차이가 나고 이전까지 태스크 수가 가장 작았던
        // 코어보다 더 작다면 정보를 갱신함
        if((iTempTaskCount + 2 <= iCurrentTaskCount) && (iTempTaskCount < iMinTaskCount)) {
            bMinCoreIndex = i;
            iMinTaskCount = iTempTaskCount;
        }
    }

    return bMinCoreIndex;
}

// 파라미터로 전달된 코어에 태스크 부하 분산 기능 사용 여부 설정
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing) {
    gs_vstScheduler[bAPICID].bUseLoadBalancing = bUseLoadBalancing;
}

// 프로세서 친화도를 변경
BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity) {
    TCB *pstTarget;
    BYTE bAPICID;

    // 태스크가 포함된 코어의 로컬 APIC ID를 찾은 후 스핀락을 잠금
    if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE) {
        return FALSE;
    }

    // 현재 실행 중인 태스크이면 프로세서 친화도만 변경. 
    // 실제 태스크가 옮겨 지는 시점은 태스크 전환 수행될 때
    pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) {
        // 프로세서 친화도 변경
        pstTarget->bAffinity = bAffinity;

        kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
    }
    // 실행 중인 태스크가 안면 준비 리스트에서 찾아서 즉시 이동
    else {
        // 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 친화도 설정
        pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL) {
            // 프로세서 친화도 변경
            pstTarget->bAffinity = bAffinity;
            }
        }
        else {
            // 프로세서 친화도 변경
            pstTarget->bAffinity = bAffinity;
        }

        kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

        // 프로세서 부하 분산을 고려해서 스케줄러 등록
        kAddTaskToSchedulerWithLoadBalancing(pstTarget);
    }

    return TRUE;
}

void kIdleTask(void) {
    TCB *pstTask, *pstChildThread, *pstProcess;
    QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    QWORD qwTaskID, qwChildThreadID;
    int i, iCount;
    void *pstThreadLink;
    BYTE bCurrentAPICID;
    BYTE bProcessAPICID;

    // 현재 코어의 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();

    // 프로세서 사용량 계산을 위해 기준 정보를 저장
    qwLastSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(1) {
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;

        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
            gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;
        
        else {
            gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 100 - 
                (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 
                100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        kHaltProcessorByLoad(bCurrentAPICID);

        if(kGetListCount(&(gs_vstScheduler[bCurrentAPICID].stWaitList)) > 0) {
            while(1) {
                kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

                pstTask = kRemoveListFromHeader(&(gs_vstScheduler[bCurrentAPICID].stWaitList));

                kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
                if(pstTask == NULL) {
                    break;
                }

                if(pstTask->qwFlags & TASK_FLAGS_PROCESS) {
                    // 자식 스레드가 존재하면 모두 종료하고 다시 자식 스레드 리스트에 삽입
                    iCount = kGetListCount(&(pstTask->stChildThreadList));
                    for(i = 0; i < iCount; i++) {
                        kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

                        // 스레드 링크의 어드레스에서 꺼내 스레드 종료
                        pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
                        if(pstThreadLink == NULL) {
                            kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
                            break;
                        }

                        // 자식 스레드 리스트에 연결된 정보는 태스크 자료구조에 있는 stThreadLink의
                        // 시작 어드레스이므로, 태스크 자료구조의 시작 어드레스를 구하기 위해 별도 계산 필요
                        pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);

                        // 다시 자식 스레드 리스트에 삽입하여 해당 스레드가 종료될 때
                        // 자식 스레드가 프로세스를 찾아 스스로 리스트에서 제거하도록 함
                        kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

                        qwChildThreadID = pstChildThread->stLink.qwID;

                        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

                        // 자식 스레드를 찾아서 종료
                        kEndTask(pstChildThread->stLink.qwID);
                    }

                    // 아직 자식 스레드가 남아있다면 종료될 때까지 기다려야 하므로 대기 리스트에 삽입
                    if(kGetListCount(&(pstTask->stChildThreadList)) > 0) {
                        kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

                        kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstTask);

                        kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
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
                        // 프로세스 ID로 프로세스가 속한 스케줄러의 ID를 찾고 스핀락 잠금
                        if(kFindSchedulerOfTaskAndLock(pstProcess->stLink.qwID, &bProcessAPICID) == TRUE) {
                            kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);
                            kUnlockForSpinLock(&(gs_vstScheduler[bProcessAPICID].stSpinLock));
                        }
                    }
                }
                
                // 여기까지오면 태스크가 정상적으로 종료된 것이므로 태스크 자료구조 반환
                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);
                kPrintf("IDLE: TASK ID[0x%q] is completely ended.\n", qwTaskID);
            }
        }
        
        kSchedule();
    }
}

void kHaltProcessorByLoad(BYTE bAPICID) {
    if(gs_vstScheduler[bAPICID].qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    }
    else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    }
    else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 95) {
        kHlt();
    }
}

QWORD kGetLastFPUUsedTaskID(BYTE bAPICID) {
    return gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID) {
    gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID = qwTaskID;
}