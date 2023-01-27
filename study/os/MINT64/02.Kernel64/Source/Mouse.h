#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "Types.h"
#include "Synchronization.h"

// 매크로
// 마우스 큐에 대한 매크로
#define MOUSE_MAXQUEUECOUNT     100

// 버튼의 상태를 나나태는 매크로
#define MOUSE_LBUTTONDOWN       0x01
#define MOUSE_RBUTTONDOWN       0x02
#define MOUSE_MBUTTONDOWN       0x04

// 구조체
#pragma pack(push, 1)

// ps/2 마우스 패킷을 저장하는 자료구조, 마우스 큐에 삽입하는 데이터
typedef struct kMousePacketStruct {
    // 버튼 상태, x, y 값에 관련된 플래그
    BYTE bButtonStatusAndFlag;

    // x축 이동 거리
    BYTE bXMovement;

    // y축 이동 거리
    BYTE bYMovement;
} MOUSEDATA;

#pragma pack(pop)

// 마우스 상태를 관리하는 자료구조
typedef struct kMouseManagerStruct {
    // 자료구조 동기화를 위한 스핀락
    SPINLOCK stSpinLock;

    // 현재 수신된 데이터의 개수, 마우스 데이터가 세 개이므로 0~2 범위를 계속 반복
    int iByteCount;

    // 현재 수신 중인 마우스 데이터
    MOUSEDATA stCurrentData;
} MOUSEMANAGER;

// 함수
BOOL kInitializeMouse(void);
BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData);
BOOL kActivateMouse(void);
void kEnableMouseInterrupt(void);
BOOL kIsMouseDataInOutputBuffer(void);
BOOL kGetMouseDataFromMouseQueue(BYTE *pbButtonStatus, int *piRelativeX, int *piRelativeY);

#endif