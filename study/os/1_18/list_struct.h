#ifndef __LIST_STRUCT__
#define __LIST_STRUCT__
#include "../MINT64/02.Kernel64/Source/Types.h"

typedef struct kListLinkStruct {
    void *pvNext;
    QWORD qwID;
} LISTLINK;

typedef struct kListManagerStruct {
    int iTemCount;
    void *pvHeader;
    void *pvTail;
} LIST;

struct kListItemExampleStruct {
    LISTLINK stLink;
    int iData1;
    char cData2;
};

#endif