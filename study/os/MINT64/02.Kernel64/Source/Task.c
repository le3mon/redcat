#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"

// 스케줄러 관련 자료구조
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

// 태스크 풀과 태스크 관련
// 태스크 풀 초기화
void kInitializeTCBPool(void) {
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

// TCB 해제
void kFreeTCB(QWORD qwID) {
    int i;

    i = qwID & 0xFFFFFFFF;

    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

// 태스크 생성
TCB *kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB *pstTask;
    void *pvStackAddress;

    pstTask = kAllocateTCB();
    if(pstTask == NULL)
        return NULL;
    
    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));

    // TCB 설정 후 준비 리스트에 삽입
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
    kAddTaskToReadyList(pstTask);

    return pstTask;
}

void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void *pvStackAddress, QWORD qwStackSize) {
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
    // 태스크 풀 초기화
    kInitializeTCBPool();

    // 준비 리스트 초기화
    kInitializeList(&(gs_stScheduler.stReadyList));

    // TCB를 할당받아 실행 중인 태스크로 설정하여, 부팅을 수행한 태스클 저장할 TCB 준비
    gs_stScheduler.pstRunningTask = kAllocateTCB();
}

// 현재 수행중인 태스크 설정
void kSetRunningTask(TCB *pstTask) {
    gs_stScheduler.pstRunningTask = pstTask;
}

// 현재 수행 중인 태스크 반환
TCB *kGetRunningTask(void) {
    return gs_stScheduler.pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
TCB *kGetNextTaskToRun(void) {
    if(kGetListCount(&(gs_stScheduler.stReadyList)) == 0)
        return NULL;
    
    return (TCB*)kRemoveListFromHeader(&(gs_stScheduler.stReadyList));
}

// 태스크 스케줄러의 준비 리스트에 삽입
void kAddTaskToReadyList(TCB *pstTask) {
    kAddListToTail(&(gs_stScheduler.stReadyList), pstTask);
}

// 다른 태스크 찾아서 전환, 인터럽트나 예외 발생 불가
void kSchedule(void) {
    TCB *pstRunningTask, *pstNextTask;
    BOOL bPreviousFlag;

    if(kGetListCount(&(gs_stScheduler.stReadyList)) == 0)
        return ;

    bPreviousFlag = kSetInterruptFlag(FALSE);
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kSetInterruptFlag(bPreviousFlag);
        return ;
    }

    pstRunningTask = gs_stScheduler.pstRunningTask;
    kAddTaskToReadyList(pstRunningTask);

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    gs_stScheduler.pstRunningTask = pstNextTask;
    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));

    kSetInterruptFlag(bPreviousFlag);
}

// 인터럽트 발생 시 다른 태스크로 전환
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char *pcContextAddress;

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL)
        return FALSE;
    
    // 태스크 전환 처리
    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    pstRunningTask = gs_stScheduler.pstRunningTask;
    kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
    kAddTaskToReadyList(pstRunningTask);

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사하여
    // 자동으로 태스크 전환이 되도록 만듬
    gs_stScheduler.pstRunningTask = pstNextTask;
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