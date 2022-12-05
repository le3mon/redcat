#ifndef __SCHEDULER_STRUCT__
#define __SCHEDULER_STRUCT__

typedef struct kSchedulerStruct {
    // 현재 실행 중인 태스크
    TCB *pstRunningTask;
    
    // 현재 수행 중인 태스크가 할당할 수 있는 프로세서 시간
    int iProcessorTime;

    // 실행할 태스크가 준비 중인 리스트
    LIST stReadyList;
}

#endif