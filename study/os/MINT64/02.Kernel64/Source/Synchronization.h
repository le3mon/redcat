#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kMutexStruct {
    volatile QWORD qwTaskID;
    volatile DWORD dwLockCount;

    volatile BOOL bLockFlag;

    BYTE vbPadding[3];
} MUTEX;

// 스핀락 자료구조
typedef struct kSpinLockStruct {
    // 로컬 APIC ID와 잠금을 수행한 횟수
    volatile DWORD dwLockCount;
    volatile BYTE bAPICID;

    // 잠금 플래그
    volatile BOOL bLockFlag;

    // 인터럽트 플래그
    volatile BOOL bInterruptFlag;

    // 자료구조의 크기를 8바이트 단위로 맞추려고 추가한 필드
    BYTE vbPadding[1];
} SPINLOCK;

#pragma pack(pop)

// 함수
#if 0
BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL bInterruptFlag);
#endif

void kInitializeSpinLock(SPINLOCK *pstSpinLock);
void kLockForSpinLock(SPINLOCK *pstSpinLock);
void kUnlockForSpinLock(SPINLOCK *pstSpinLock);

void kInitializeMutex(MUTEX *pstMutex);
void kLock(MUTEX *pstMutex);
void kUnlock(MUTEX *pstMutex);


#endif