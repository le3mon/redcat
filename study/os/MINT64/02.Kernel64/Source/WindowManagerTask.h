#ifndef __WINDOWMANAGERTASK_H__
#define __WINDOWMANAGERTASK_H__

#include "Types.h"

// 매크로
// 윈도우 매니저 태스크가 처리할 데이터나 이벤트를 통합하는 최대 개수
#define WINDOWMANAGER_DATAACCUMULATECOUNT       20

// 함수
void kStartWindowManager(void);
BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

#endif