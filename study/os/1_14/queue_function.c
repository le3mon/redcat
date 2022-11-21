#include "queue_struct.h"
#include "../MINT64/02.Kernel64/Source/Utility.h"

BOOL kIsQueueFull(const QUEUE *pstQueue) {
    // 삽입과 제거 인덱스가 같으며, 마지막으로 수행된 명령이 삽입이면 => 큐가 가득참
    if((pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == TRUE))
        return TRUE;
    
    return FALSE;
}

BOOL kIsQueueEmpty(const QUEUE *pstQueue) {
    if((pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == FALSE))
        return TRUE;
    
    return FALSE;
}

BOOL kPutQueue(QUEUE *pstQueue, const void *pvData) {
    if(kIsQueueFull(pstQueue) == TRUE)
        return FALSE;
    
    kMemCpy((char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex), pvData, pstQueue->iDataSize);

    pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = TRUE;
    return TRUE;
}

BOOL kGetQueue(QUEUE *pstQueue, void *pvData) {
    if(kIsQueueEmpty(pstQueue) == TRUE)
        return FALSE;
    
    kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize);

    pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = FALSE;
    return TRUE;
}