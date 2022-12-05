#ifndef __LIST__
#define __LIST__

#include "Types.h"

#pragma pack(push, 1)
// 데이터를 연결하는 자료구조
// 반드시 데이터의 가장 앞 부분에 위치해야 함
typedef struct kListLinkStruct {
    // 다음 데이터의 어드레스와 식별을 위한 ID
    void *pvNext;
    QWORD qwID;
} LISTLINK;

// 리스스 사용할 데이터 정의 예시
struct kListItemExampleStruct {
    LISTLINK stLink;

    int iData1;
    char cData2;
};

// 리스트 관리 자료구조
typedef struct kListManagerStruct {
    // 리스트 데이터 수
    int iItemCount;

    // 리스트 처음과 마지막 어드레스
    void *pvHeader;
    void *pvTail;
} LIST;

#pragma pack(pop)

void kInitializeList(LIST *pstList);
int kGetListCount(const LIST *pstList);
void kAddListToTail(LIST *pstList, void *pvItem);
void kAddListToHeader(LIST *pstList, void *pvItem);
void *kRemoveList(LIST *pstList, QWORD qwID);
void *kRemoveListFromHeader(LIST *pstList);
void *kRemoveListFromTail(LIST *pstList);
void *kFindList(const LIST *pstList, QWORD qwID);
void *kGetHeaderFromList(const LIST *pstList);
void *kGetTailFromList(const LIST *pstList);
void *kGetNextFromList(const LIST *pstList, void *pstCurrent);

#endif