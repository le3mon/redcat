#ifndef __SYSTEMCALL_H__
#define __SYSTEMCALL_H__

#include "Types.h"

// 매크로
#define SYSTEMCALL_MAXPARAMETERCOUNT    20

// 구조체
#pragma pack(push, 1)

// 시스템 콜 호출 시 파라미터를 관리하는 자료구조
typedef struct kSystemCallParameterTableStruct {
    QWORD vqwValue[SYSTEMCALL_MAXPARAMETERCOUNT];
} PARAMETERTABLE;

#pragma pack(pop)

// 파라미터 자료구조에서 N번쨰를 가리키는 매크로
#define PARAM(x)    (pstParameter->vqwValue[(x)])

// 함수
void kInitializeSystemCall(void);
void kSystemCallEntryPoint(QWORD qwServiceNumber, PARAMETERTABLE *pstParameter);
QWORD kProcessSystemCall(QWORD qwServiceNumber, PARAMETERTABLE *pstParameter);

void kSystemCallTestTask(void);

#endif