#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
#include "Synchronization.h"

#define TASK_REGISTERCOUNT  (5 + 19)
#define TASK_REGISTERSIZE   8

#define TASK_GSOFFSET       0
#define TASK_FSOFFSET       1
#define TASK_ESOFFSET       2
#define TASK_DSOFFSET       3
#define TASK_R15OFFSET      4
#define TASK_R14OFFSET      5
#define TASK_R13OFFSET      6
#define TASK_R12OFFSET      7
#define TASK_R11OFFSET      8
#define TASK_R10OFFSET      9
#define TASK_R9OFFSET       10
#define TASK_R8OFFSET       11
#define TASK_RSIOFFSET      12
#define TASK_RDIOFFSET      13
#define TASK_RDXOFFSET      14
#define TASK_RCXOFFSET      15
#define TASK_RBXOFFSET      16
#define TASK_RAXOFFSET      17
#define TASK_RBPOFFSET      18
#define TASK_RIPOFFSET      19
#define TASK_CSOFFSET       20
#define TASK_RFLAGSOFFSET   21
#define TASK_RSPOFFSET      22
#define TASK_SSOFFSET       23

// 태스크 풀의 어드레스
#define TASK_TCBPOOLADDRESS 0x800000
#define TASK_MAXCOUNT       1024

// 스택 풀과 스택 크기
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE      8192

// 유효하지 않은 태스크 id
#define TASK_INVALIDID      0xFFFFFFFFFFFFFFFF

// 태스크가 사용할 수 있는 프로세서 시간(5ms)
#define TASK_PROCESSORTIME  5

// 준비 리스트의 수
#define TASK_MAXREADYLISTCOUNT 5

// 태스크 우선 순위
#define TASK_FLAGS_HIGHEST      0
#define TASK_FLAGS_HIGH         1
#define TASK_FLAGS_MEDIUM       2
#define TASK_FLAGS_LOW          3
#define TASK_FLAGS_LOWEST       4
#define TASK_FLAGS_WAIT         0XFF

// 태스크 플래그
#define TASK_FLAGS_ENDTASK      0x8000000000000000
#define TASK_FLAGS_SYSTEM       0x4000000000000000
#define TASK_FLAGS_PROCESS      0x2000000000000000
#define TASK_FLAGS_THREAD       0x1000000000000000
#define TASK_FLAGS_IDLE         0x0800000000000000

// 매크로
#define GETPRIORITY( x ) ( ( x ) & 0xFF )
#define SETPRIORITY( x, priority ) ( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | ( priority ) )
#define GETTCBOFFSET( x ) ( ( x ) & 0xFFFFFFFF )

#define GETTCBFROMTHREADLINK(x) (TCB*) ((QWORD)(x) - offsetof(TCB, stThreadLink))

// 프로세서 친화도 필드에 설정되는 값 중 하나, 해당 값으로 설정 시 태스크 부하 분산 수행
#define TASK_LOADBALANCINGID    0xFF

#pragma pack(push, 1)
typedef struct kContextStruct {
    QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct {
    // 다음 데이터의 위치와 ID
    LISTLINK stLink;
    
    QWORD qwFlags;

    // 프로세스 메모리 영역의 시작과 크기
    void *pvMemoryAddress;
    QWORD qwMemorySize;

    // 자식 스레드 위치, ID
    LISTLINK stThreadLink;

    // 부모 프로세스 ID
    QWORD qwParentProcessID;

    // FPU 콘텍스트는 16의 배수로 정렬되어야 함
    QWORD vqwFPUContext[512 / 8];

    // 자식 스레드 리스트
    LIST stChildThreadList;

    // 콘텍스트
    CONTEXT stContext;

    // 스택 주소와 크기
    void *pvStackAddress;
    QWORD qwStackSize;

    // FPU 사용 여부
    BOOL bFPUUsed;

    // 프로세서 친화도
    BYTE bAffinity;

    // 현재 태스크를 수행하는 코어의 로컬 APIC ID
    BYTE bAPICID;

    // TCB 전체를 16바이트 배수로 맞추기 위한 패딩
    char vcPadding[9];
} TCB;

// TCB 풀의 상태 관리하는 자료구조
typedef struct kTCBPoolManagerStruct {
    // 자료구조 동기화를 위한 스핀락
    SPINLOCK stSpinLock;

    // 태스크 풀에 대한 정보
    TCB *pstStartAddress;
    int iMaxCount;
    int iUseCount;

    // TCB가 할당된 횟수
    int iAllocatedCount;
} TCBPOOLMANAGER;

// 스케줄러의 상태 관리하는 자료구조
typedef struct kSchedulerStruct {
    // 자료구조 동기화를 위한 스핀락
    SPINLOCK stSpinLock;

    // 현재 수행중인 태스크
    TCB *pstRunningTask;

    // 현재 수행중인 태스크가 사용할 수 있는 시간
    int iProcessorTime;

    // 실행할 태스크가 준비 중인 리스트, 태스크의 우선순위에 따라 구분
    LIST vstReadyList[TASK_MAXREADYLISTCOUNT];
    
    // 종료할 태스크 대기 리스트
    LIST stWaitList;

    // 우선순위 별 실행 횟수 저장용 자료구조
    int viExecuteCount[TASK_MAXREADYLISTCOUNT];

    // 프로세서 부하 계산용
    QWORD qwProcessorLoad;

    // 유휴 태스크에서 사용한 프로세서 시간
    QWORD qwSpendProcessorTimeInIdleTask;

    // 마지막으로 FPU를 사용한 태스크의 ID
    QWORD qwLastFPUUsedTaskID;

    // 부하 분산 기능 사용 여부
    BOOL bUseLoadBalancing;
} SCHEDULER;

#pragma pack(pop)

// 태스크 풀과 태스크 관련
static void kInitializeTCBPool(void);
static TCB *kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB *kCreateTask(QWORD qwFlags, void *pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity);
static void kSetUpTask(TCB *pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void *pvStackAddress, QWORD qwStackSize);

// 스케줄러 관련
void kInitializeScheduler(void);
void kSetRunningTask(BYTE bAPICID, TCB *pstTask);
TCB *kGetRunningTask(BYTE bAPICID);
static TCB *kGetNextTaskToRun(BYTE bAPICID);
static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB *pstTask);
BOOL kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(BYTE bAPICID);
BOOL kIsProcessorTimeExpired(BYTE bAPICID);
static TCB *kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID);
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE *pbAPICID);
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask(void);
int kGetReadyTaskCount(BYTE bAPICID);
int kGetTaskCount(BYTE bAPICID);
TCB *kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwID);
QWORD kGetProcessorLoad(BYTE bAPICID);
static TCB *kGetProcessByThread(TCB *pstThread);
void kAddTaskToSchedulerWithLoadBalancing(TCB *pstTask);
static BYTE kFindSchedulerOfMinumumTaskCount(const TCB *pstTask);
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing);
BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity);

// 유휴 태스크 관련
void kIdleTask(void);
void kHaltProcessorByLoad(BYTE bAPICID);

// FPU 관련
QWORD kGetLastFPUUsedTaskID(BYTE bAPICID);
void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID);

#endif