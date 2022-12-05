#include "list_struct.h"

typedef struct kTaskControlBlockStruct {
    LISTLINK stLink;

    QWORD qwFlags;

    CONTEXT stContext;

    void *pvStackAddress;
    QWORD qwStackSize;
} TCB;

// 현재 수행 중인 태스크를 설정
void kSetRunningTask(TCB *pstTask) {
    gs_stScheduler.pstRunningTask = pstTask;
}

// 현재 수행 중인 태스크 반환
TCB *kGetRunningTask(void) {
    return gs_stScheduler.pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크을 얻음
TCB *kGetNextTaskRun(void) {
    if(kGetListCount(&(gs_stScheduler.stReadyList)) == 0)
        return NULL;
    
    return (TCB*)kRemoveListFromHeader(&(gs_stScheduler.stReadyList));
}

void kAddTaskToReadyList(TCB *pstTask) {
    kAddListToTail(&(gs_stScheduler.stReadyList), pstTask);
}