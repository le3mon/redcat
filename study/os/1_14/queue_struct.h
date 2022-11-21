#ifndef __QUEUE_STRUCT__
#define __QUEUE_STRUCT__
#include "../MINT64/02.Kernel64/Source/Types.h"

typedef struct kQueueManagerStruct {
    int iDataSize;
    int iMaxDataCount;

    void *pvQueueArray;
    int iPutIndex;
    int iGetIndex;

    BOOL bLastOperationPut;
} QUEUE;


#define KEY_MAXQUEUECOUNT 100
static QUEUE gs_stKeyQueue; //키 정보를 저장하는 큐
static KEYDATA gs_vstKeyQueueBuffer[ KEY_MAXQUEUECOUNT ];
typedef struct kKeyDataStruct {
    BYTE bScanCode; // 키보드에서 전달된 스캔 코드
    BYTE bASCIICode; // 스캔 코드를 변환한 ASCII 코드
    BYTE bFlags; // 키 상태를 저장하는 플래그(눌림/떨어짐/확장 키 여부)
} KEYDATA;


#endif