#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "MultiProcessor.h"

// 스핀락 도입 이전에 사용하던 함수들
#if 0
BOOL kLockForSystemData(void) {
    return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag) {
    kSetInterruptFlag(bInterruptFlag);
}
#endif

void kInitializeMutex(MUTEX *pstMutex) {
    pstMutex->bLockFlag = FALSE;
    pstMutex->dwLockCount = 0;
    pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock(MUTEX *pstMutex) {
    BYTE bCurrentAPICID;
    BOOL bInterruptFlag;

    // 인터럽트 비활성화
    bInterruptFlag = kSetInterruptFlag(FALSE);

    // 현재 코어의 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();

    // 이미 잠겨 있다면 내가 잠갔는지 확인하고 잠근 횟수 증가시킨 뒤 종료
    if(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
        // 자신이 잠갔다면 횟수만 증가
        if(pstMutex->qwTaskID == kGetRunningTask(bCurrentAPICID)->stLink.qwID) {
            kSetInterruptFlag(bInterruptFlag);
            pstMutex->dwLockCount++;
            return ;
        }

        while(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
            kSchedule();
        }
    }

    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask(bCurrentAPICID)->stLink.qwID;
    kSetInterruptFlag(bInterruptFlag);
}

void kUnlock(MUTEX *pstMutex) {
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    if((pstMutex->bLockFlag == FALSE) || 
        (pstMutex->qwTaskID != kGetRunningTask(kGetAPICID())->stLink.qwID)) {
        kSetInterruptFlag(bInterruptFlag);
        return ;
    }

    if(pstMutex->dwLockCount > 1) {
        pstMutex->dwLockCount--;
    }
    else {
        // 해제된 것으로 설정, 잠김 플래그를 가장 나중에 해제해야 함
        pstMutex->qwTaskID = TASK_INVALIDID;
        pstMutex->dwLockCount = 0;
        pstMutex->bLockFlag = FALSE;
    }

    kSetInterruptFlag(bInterruptFlag);
}

// 스핀락 초기화
void kInitializeSpinLock(SPINLOCK *pstSpinLock) {
    // 잠김 플래그 횟수, APIC ID, 인터럽트 플래그 초기화
    pstSpinLock->bLockFlag = FALSE;
    pstSpinLock->dwLockCount = 0;
    pstSpinLock->bAPICID = 0xFF;
    pstSpinLock->bInterruptFlag = FALSE;
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
void kLockForSpinLock(SPINLOCK *pstSpinLock) {
    BOOL bInterruptFlag;

    // 인터럽트 비활성화
    bInterruptFlag = kSetInterruptFlag(FALSE);

    // 이미 잠겨있다면 내가 잠갔는지 확인하고 그렇다면 잠근 횟수를 증가시킨 뒤 종료
    if(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE) {
        // 자신이 잠갔다면 횟수만 증가
        if(pstSpinLock->bAPICID == kGetAPICID()) {
            pstSpinLock->dwLockCount++;
            return;
        }

        // 자신이 아닌 경우 잠긴 것이 해제될 때까지 대기
        while(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE) {
            // kTestAndSet함수를 계속 호출하여 메모리 버스가 Lock 되는 것을 방지
            while(pstSpinLock->bLockFlag == TRUE) {
                kPause();
            }
        }
    }

    // 잠김 설정, 잠김 플래그는 위의 kTestAndSet() 함수에서 처리
    pstSpinLock->dwLockCount = 1;
    pstSpinLock->bAPICID = kGetAPICID();

    // 인터럽트 플래그를 저장하여 Unlock 수행 시 복원
    pstSpinLock->bInterruptFlag = bInterruptFlag;
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 해제 함수
void kUnlockForSpinLock(SPINLOCK *pstSpinLock) {
    BOOL bInterruptFlag;

    // 인터럽트 먼저 비활성화
    bInterruptFlag = kSetInterruptFlag(FALSE);

    // 스핀락을 잠근 태스크가 아니면 실패
    if((pstSpinLock->bLockFlag == FALSE) || (pstSpinLock->bAPICID != kGetAPICID())) {
        kSetInterruptFlag(bInterruptFlag);
        return;
    }

    // 스핀락을 중복으로 잠갔으면 잠긴 횟수만 감소
    if(pstSpinLock->dwLockCount > 1) {
        pstSpinLock->dwLockCount--;
        return;
    }

    // 스핀락을 해제된 것으로 설정하고 인터럽트 플래그를 복원
    // 인터럽트 플래그는 미리 저장해두었다가 사용
    bInterruptFlag = pstSpinLock->bInterruptFlag;
    pstSpinLock->bAPICID = 0xFF;
    pstSpinLock->dwLockCount = 0;
    pstSpinLock->bInterruptFlag = FALSE;
    pstSpinLock->bLockFlag = FALSE;

    kSetInterruptFlag(bInterruptFlag);
}