#include "List.h"

// 리스트 초기화
void kInitializeList(LIST *pstList) {
    pstList->iItemCount = 0;
    pstList->pvHeader = NULL;
    pstList->pvTail = NULL;
}

// 리스트 데이터 수 반환
int kGetListCount(const LIST *pstList) {
    return pstList->iItemCount;
}

// 리스트에 데이터 추가
void kAddListToTail(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink;

    // 다음 리스트 주소를 널로 설정
    pstLink = (LISTLINK*)pvItem;
    pstLink->pvNext = NULL;

    // 리스트가 빈 상태라면 header, tail을 추가한 데이터로 설정
    if(pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return ;
    }

    // 리스트 마지막 데이터에 추가한 데이터 주소 설정
    pstLink = (LISTLINK*)pstList->pvTail;
    pstLink->pvNext = pvItem;

    // 리스트 마지막 데이터를 추가한 데이터로 설정
    pstList->pvTail = pvItem;
    pstList->iItemCount++;
}

// 리스트의 첫 부분에 데이터 추가
void kAddListToHeader(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink;

    // 현재 리스트의 첫 데이터를 추가하는 데이터의 다음 데이터로 설정
    pstLink = (LISTLINK*)pvItem;
    pstLink->pvNext = pstList->pvHeader;

    if(pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return ;
    }

    pstList->pvHeader = pvItem;
    pstList->iItemCount++;
}

// 리스트에서 데이터 제거 후, 데이터의 포인터 반환
void *kRemoveList(LIST *pstList, QWORD qwID) {
    LISTLINK *pstLink;
    LISTLINK *pstPreviousLink;

    pstPreviousLink = (LISTLINK*)pstList->pvHeader;
    for(pstLink = pstPreviousLink; pstLink != NULL; pstLink = pstLink->pvNext) {
        // 일차하는 ID가 있다면 제거
        if(pstLink->qwID == qwID) {
            // 만약 데이터가 하나라면 리스트 초기화
            if((pstLink == pstList->pvHeader) && (pstLink == pstList->pvTail)) {
                pstList->pvHeader = NULL;
                pstList->pvTail = NULL;
            }

            // 만약 리스트의 첫 번째 데이터라면 header를 두 번쨰 데이터로 변경
            else if(pstLink == pstList->pvHeader)
                pstList->pvHeader = pstLink->pvNext;
            
            // 만약 리스트의 마지막 데이터라면 tail을 마지막 이전 데이터로 변경
            else if(pstLink == pstList->pvTail)
                pstList->pvTail = pstPreviousLink;
            
            // 나머지는 모두 삭제하는 이전 데이터의 다음 주소를 삭제하는 다음 데이터로 설정
            else
                pstPreviousLink->pvNext = pstLink->pvNext;
            
            pstList->iItemCount--;
            return pstLink;
        }
        pstPreviousLink = pstLink;
    }
    return NULL;
}

// 리스트의 첫 번쨰 데이터를 제거하여 반환
void *kRemoveListFromHeader(LIST *pstList) {
    LISTLINK *pstLink;

    if(pstList->iItemCount == 0)
        return NULL;
    
    pstLink = (LISTLINK*)pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}

// 리스트의 마지막 데이터를 제거하여 반환
void *kRemoveListFromTail(LIST *pstList) {
    LISTLINK *pstLink;

    if(pstList->iItemCount == 0)
        return NULL;
    
    pstLink = (LISTLINK*)pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}

// 리스트에서 특정 아이템 찾음
void *kFindList(const LIST *pstList, QWORD qwID) {
    LISTLINK *pstLink;

    for(pstLink = (LISTLINK*)pstList->pvHeader; pstLink != NULL; pstLink = pstLink->pvNext) {
        if(pstLink->qwID = qwID)
            return pstLink;
    }

    return NULL;
}

// 리스트 헤더 반환
void *kGetHeaderFromList(const LIST *pstList) {
    return pstList->pvHeader;
}

// 리스트 테일 반환
void *kGetTailFromList(const LIST *pstList) {
    return pstList->pvTail;
}

// 현재 아이템의 다음 아이템 반환
void *kGetNextFromList(const LIST *pstList, void *pstCurrent) {
    LISTLINK *pstLink;

    pstLink = (LISTLINK*)pstCurrent;
    return pstLink->pvNext;
}