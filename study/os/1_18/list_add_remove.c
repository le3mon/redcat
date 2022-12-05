#include "list_struct.h"

void kAddListToTail(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink;
    pstLink = (LISTLINK*)pvItem;
    pstLink->pvNext = NULL;

    // Header == NULL => 리스트가 비었다는 것을 의미
    if(pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iTemCount = 1;
        return ;
    }

    pstLink = (LISTLINK*)pstList->pvTail;
    pstLink->pvNext = pvItem;

    pstList->pvTail = pvItem;
    pstList->iTemCount++;
}

void kAddListToHeader(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink;

    pstLink = (LISTLINK*)pvItem;
    pstLink->pvNext = pstList->pvHeader;

    if(pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iTemCount = 1;
        return ;
    }

    pstList->pvHeader = pvItem;
    pstList->iTemCount++;
}

void *kRemoveList(LIST *pstList, QWORD qwID) {
    LISTLINK *pstLink;
    LISTLINK *pstPreviousLink;

    pstPreviousLink = (LISTLINK*)pstList->pvHeader;
    for(pstLink = pstPreviousLink; pstLink != NULL; pstLink = pstLink->pvNext) {
        if(pstLink->qwID == qwID) {
            if((pstLink == pstList->pvHeader) && (pstLink == pstList->pvTail)) {
                pstList->pvHeader = NULL;
                pstList->pvTail = NULL;
            }

            else if(pstLink == pstList->pvHeader) {
                pstList->pvHeader = pstLink->pvNext;
            }

            else if(pstLink == pstList->pvTail) {
                pstList->pvTail = pstPreviousLink;
            }

            else {
                pstPreviousLink->pvNext = pstLink->pvNext
            }

            pstList->iTemCount--;
            return pstLink;
        }
        pstPreviousLink = pstLink;
    }
    return NULL;
}

void *kRemoveListFromHeader(LIST *pstList) {
    LISTLINK *pstLink;

    if(pstList->iTemCount == 0)
        return NULL;
    
    pstLink = (LISTLINK*)pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}

void *kRemoveListFromTail(LIST *pstList) {
    LISTLINK *pstLink;

    if(pstList->iTemCount == 0)
        return NULL;
    
    pstLink = (LISTLINK*)pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}