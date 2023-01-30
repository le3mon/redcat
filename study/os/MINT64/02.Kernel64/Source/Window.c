#include "Window.h"
#include "VBE.h"
#include "Task.h"
#include "Font.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Console.h"
#include "MultiProcessor.h"

// GUI 시스템 관련 자료구조
static WINDOWPOOLMANAGER gs_stWindowPoolManager;

// 윈도우 매니자 관련 자료구조
static WINDOWMANAGER gs_stWindowManager;

// 윈도우 풀 관련

// 윈도우 풀 초기화
static void kInitializeWindowPool(void) {
    int i;
    void *pvWindowPoolAddress;

    // 자료구조 초기화
    kMemSet(&gs_stWindowPoolManager, 0, sizeof(gs_stWindowPoolManager));

    // 윈도우 풀 메모리 할당
    pvWindowPoolAddress = (void*)kAllocateMemory(sizeof(WINDOW) * WINDOW_MAXCOUNT);
    if(pvWindowPoolAddress == NULL) {
        kPrintf("Window Pool Allocate Fail\n");
        while(1) {
            ;
        }
    }

    // 윈도우 풀 어드레스 지정 및 초기화
    gs_stWindowPoolManager.pstStartAddress = (WINDOW*)pvWindowPoolAddress;
    kMemSet(pvWindowPoolAddress, 0, sizeof(WINDOW) * WINDOW_MAXCOUNT);

    // 윈도우 풀에 ID 할당
    for(i = 0; i < WINDOW_MAXCOUNT; i++) {
        gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    // 윈도우최대 개수와 할당된 횟수 초기화
    gs_stWindowPoolManager.iMaxCount = WINDOW_MAXCOUNT;
    gs_stWindowPoolManager.iAllocatedCount = 1;

    // 뮤텍스 초기화
    kInitializeMutex(&(gs_stWindowPoolManager.stLock));
}

// 윈도우 자료구조 할당
static WINDOW *kAllocateWindow(void) {
    WINDOW *pstEmptyWindow;
    int i;

    // 동기화 처리
    kLock(&(gs_stWindowPoolManager.stLock));

    // 윈도우가 모두 할당되었으면 실패
    if(gs_stWindowPoolManager.iUseCount == gs_stWindowPoolManager.iMaxCount) {
        // 동기화 처리
        kUnlock(&(gs_stWindowPoolManager.stLock));
        return NULL;
    }

    // 윈도우 풀을 돌면서 빈 영역 탐색
    for(i = 0; i < gs_stWindowPoolManager.iMaxCount; i++) {
        // ID 상위 32비트가 0이면 비어 있는 윈도우 자료구조임
        if((gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
            pstEmptyWindow = &(gs_stWindowPoolManager.pstStartAddress[i]);
            break;
        }
    }

    // 상위 32비트를 0이 아닌 갓ㅂ으로 설정해서 할당도니 윈도우 설정
    pstEmptyWindow->stLink.qwID = ((QWORD)gs_stWindowPoolManager.iAllocatedCount << 32) | i;

    // 자료구조가 사용 중인 개수와 할당된 횟수를 증가
    gs_stWindowPoolManager.iUseCount++;
    gs_stWindowPoolManager.iAllocatedCount++;
    if(gs_stWindowPoolManager.iAllocatedCount == 0) {
        gs_stWindowPoolManager.iAllocatedCount = 1;
    }

    // 동기화 처리
    kUnlock(&(gs_stWindowPoolManager.stLock));

    // 윈도우 뮤텍스 초기화
    kInitializeMutex(&(pstEmptyWindow->stLock));

    return pstEmptyWindow;
}

// 윈도우 자료구조 해제
static void kFreeWindow(QWORD qwID) {
    int i;

    // 윈도우 ID로 윈도우 풀의 오프셋 계산, 윈도우 ID 하위 32비트가 인덱스 역할을 함
    i = GETWINDOWOFFSET(qwID);

    // 동기화 처리
    kLock(&(gs_stWindowPoolManager.stLock));

    // 윈도우 자료구조를 초기화하고 ID 설정
    kMemSet(&(gs_stWindowPoolManager.pstStartAddress[i]), 0, sizeof(WINDOW));
    gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;

    // 사용 중인 자료구조 개수 감소
    gs_stWindowPoolManager.iUseCount--;

    // 동기화 처리
    kUnlock(&(gs_stWindowPoolManager.stLock));
}

// 윈도우와 윈도우 매니저 관련

// GUI 시스템 초기화
void kInitializeGUISystem(void) {
    VBEMODEINFOBLOCK *pstModeInfo;
    QWORD qwBackgroundWindowID;

    // 윈도우 풀 초기화
    kInitializeWindowPool();

    // VBE 모드 정보 블록을 반환
    pstModeInfo = kGetVBEModeInfoBlock();

    // 비디오 메모리 어드레스 설정
    gs_stWindowManager.pstVideoMemory = 
        (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer & 0xFFFFFFFF);
    
    // 마우스 커서 초기 위치 설정
    gs_stWindowManager.iMouseX = pstModeInfo->wXResolution / 2;
    gs_stWindowManager.iMouseY = pstModeInfo->wYResolution / 2;

    // 화면 영역의 범위 설정
    gs_stWindowManager.stScreenArea.iX1 = 0;
    gs_stWindowManager.stScreenArea.iY1 = 0;
    gs_stWindowManager.stScreenArea.iX2 = pstModeInfo->wXResolution - 1;
    gs_stWindowManager.stScreenArea.iY2 = pstModeInfo->wYResolution - 1;

    // 뮤텍스 초기화
    kInitializeMutex(&(gs_stWindowManager.stLock));

    // 윈도우 리스트 초기화
    kInitializeList(&(gs_stWindowManager.stWindowList));

    // 배경 윈도우 새엉

    // 플래그에 0을 넘겨서 화면에 윈도우를 그리지 않도록 함.
    // 배경 윈도우는 윈도우 내에 배경색을 모두 칠한 뒤 나타냄
    qwBackgroundWindowID = kCreateWindow(0, 0, pstModeInfo->wXResolution,
        pstModeInfo->wYResolution, 0, WINDOW_BACKGROUNDWINDOWTITLE);
    gs_stWindowManager.qwBackgroundWindowID = qwBackgroundWindowID;

    // 배경 윈도우 내부에 배경색을 채움
    kDrawRect(qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1,
        pstModeInfo->wYResolution - 1, WINDOW_COLOR_SYSTEMBACKGROUND, TRUE);
    
    // 배경 윈도우를 화면에 나타냄
    kShowWindow(qwBackgroundWindowID, TRUE);
}

// 윈도우 매니저를 반환
WINDOWMANAGER *kGetWindowManager(void) {
    return &gs_stWindowManager;
}

// 배경 윈도우의 ID를 반환
QWORD kGetBackgroundWindowID(void) {
    return gs_stWindowManager.qwBackgroundWindowID;
}

// 화면 영역의 크기를 반환
void kGetScreenArea(RECT *pstScreenArea) {
    kMemCpy(pstScreenArea, &(gs_stWindowManager.stScreenArea), sizeof(RECT));
}

// 윈도우 생성
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags, const char *pcTitle) {
    WINDOW *pstWindow;
    TCB *pstTask;

    // 크기가 0인 윈도우는 만들 수 없음
    if((iWidth <= 0) || (iHeight <= 0)) {
        return WINDOW_INVALID;
    }

    // 윈도우 자료구조 할당
    pstWindow = kAllocateWindow();
    if(pstWindow == NULL) {
        return WINDOW_INVALID;
    }

    // 윈도우 영역 설정
    pstWindow->stArea.iX1 = iX;
    pstWindow->stArea.iY1 = iY;
    pstWindow->stArea.iX2 = iX + iWidth - 1;
    pstWindow->stArea.iY2 = iY + iHeight - 1;

    // 윈도우 제목 저장
    kMemCpy(pstWindow->vcWindowTitle, pcTitle, WINDOW_TITLEMAXLENGTH);

    // 윈도우 화면 버퍼 할당
    pstWindow->pstWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight * sizeof(COLOR));
    if(pstWindow == NULL) {
        // 윈도우 화면 버퍼 할당에 실패하면 윈도우 자료구조 반환
        kFreeWindow(pstWindow->stLink.qwID);
        return WINDOW_INVALID;
    }

    // 윈도우를 생성한 태스크의 ID 저장
    pstTask = kGetRunningTask(kGetAPICID());
    pstWindow->qwTaskID = pstTask->stLink.qwID;

    // 윈도우 속성 설정
    pstWindow->dwFlags = dwFlags;

    // 윈도우 배경 그리기
    kDrawWindowBackground(pstWindow->stLink.qwID);

    // 윈도우 테두리 그리기
    if(dwFlags & WINDOW_FLAGS_DRAWFRAME) {
        kDrawWindowFrame(pstWindow->stLink.qwID);
    }

    // 윈도우 제목 표시줄 그리기
    if(dwFlags & WINDOW_FLAGS_DRAWTITLE) {
        kDrawWindowTitle(pstWindow->stLink.qwID, pcTitle);
    }

    // 동기화 처리
    kLock(&(gs_stWindowManager.stLock));

    // 윈도우 리스트의 가장 마지막에 추가하여 최상위 윈도우로 설정
    kAddListToTail(&gs_stWindowManager.stWindowList, pstWindow);

    // 동기화 처리
    kUnlock(&(gs_stWindowManager.stLock));

    // 윈도우를 그리는 옵션이 들어 있으면 해당 윈도우를 그림
    if(dwFlags & WINDOW_FLAGS_SHOW) {
        // 윈도우 영역만큼 화면에 업데이트
        kRedrawWindowByArea(&(pstWindow->stArea));
    }

    return pstWindow->stLink.qwID;
}

// 윈도우를 삭제
BOOL kDeleteWindow(QWORD qwWindowID) {
    WINDOW *pstWindow;
    RECT stArea;

    // 동기화 처리
    kLock(&(gs_stWindowManager.stLock));

    // 윈도우 검색
    pstWindow = kGetWindowWithWindowLock(qwWindowID);
    if(pstWindow == NULL) {
        // 동기화 처리
        kUnlock(&(gs_stWindowManager.stLock));
        return FALSE;
    }

    // 윈도우 삭제 전 영역 저장
    kMemCpy(&stArea, &(pstWindow->stArea), sizeof(RECT));

    // 윈도우 리스트에서 윈도우 삭제
    if(kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID) == NULL) {
        // 동기화 처리
        kUnlock(&(pstWindow->stLock));
        kUnlock(&(gs_stWindowManager.stLock));

        return FALSE;
    }

    // 동기화 처리
    kLock(&(pstWindow->stLock));

    // 윈도우 화면 버퍼를 반환
    kFreeMemory(pstWindow->pstWindowBuffer);

    // 동기화 처ㅣ
    kUnlock(&(pstWindow->stLock));

    // 윈도우 자료구조 반환
    kFreeWindow(qwWindowID);

    kUnlock(&(gs_stWindowManager.stLock));

    // 삭제되기 전에 윈도우가 있던 영역을 화면에 다시 업데이트
    kRedrawWindowByArea(&stArea);

    return TRUE;
}

// 태스크 ID가 일치하는 모든 윈도우 삭제
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID) {
    WINDOW *pstWindow;
    WINDOW *pstNextWindow;

    // 동기화 처리
    kLock(&(gs_stWindowManager.stLock));

    // 리스트에서 첫 번쨰 윈도우 반환
    pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
    while(pstWindow != NULL) {
        // 다음 윈도우를 미리 구함
        pstNextWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);

        // 배경 윈도우가 아니고 태스크 ID가 일치하면 윈도우 삭제
        if((pstWindow->stLink.qwID != gs_stWindowManager.qwBackgroundWindowID) &&
            (pstWindow->qwTaskID == qwTaskID)) {
            kDeleteWindow(pstWindow->stLink.qwID);
        }

        // 미리 구해둔 다음 윈도우 값을 설정
        pstWindow = pstNextWindow;
    }

    // 동기화 처리
    kUnlock(&(gs_stWindowManager.stLock));
}

// 윈도우 ID로 윈도우 포인터 반환
WINDOW *kGetWindow(QWORD qwWindowID) {
    WINDOW *pstWindow;

    // 윈도우 ID의 유효 범위 검사
    if(GETWINDOWOFFSET(qwWindowID) >= WINDOW_MAXCOUNT) {
        return NULL;
    }

    // ID로 윈도우 포인터를 찾은 뒤 ID가 일치하면 반환
    pstWindow = &gs_stWindowPoolManager.pstStartAddress[GETWINDOWOFFSET(qwWindowID)];
    if(pstWindow->stLink.qwID == qwWindowID) {
        return pstWindow;
    }

    return NULL;
}

// 윈도우 ID로 윈도우 포인터를 찾아 윈도우 뮤텍스를 잠근 뒤 반환
WINDOW *kGetWindowWithWindowLock(QWORD qwWindowID) {
    WINDOW *pstWindow;
    BOOL bResult;

    // 윈도우 검색
    pstWindow = kGetWindow(qwWindowID);
    if(pstWindow == NULL) {
        return NULL;
    }

    // 동기화 처리한 뒤 다시 윈도우 ID로 윈도우 검색
    kLock(&(pstWindow->stLock));

    // 윈도우 동기화 후 윈도우 ID로 윈도우 검색할 수 없다면
    // 도중에 변경된 것 이므로 NULL 반환
    pstWindow = kGetWindow(qwWindowID);
    if(pstWindow == NULL) {
        kUnlock(&(pstWindow->stLock));
        return NULL;
    }

    return pstWindow;
}

// 화면에 윈도우를 나타내거나 숨김
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow) {
    WINDOW *pstWindow;

    // 윈도우 검색과 동기화 처리
    pstWindow = kGetWindowWithWindowLock(qwWindowID);
    if(pstWindow == NULL) {
        return FALSE;
    }

    // 윈도우 속성 설정
    if(bShow == TRUE) {
        pstWindow->dwFlags |= WINDOW_FLAGS_SHOW;
    }
    else {
        pstWindow->dwFlags &= WINDOW_FLAGS_SHOW;
    }

    // 동기화 처리
    kUnlock(&(pstWindow->stLock));

    // 윈도우가 있던 영역을 다시 업데이트 하여 윈도우를 나타내거나 숨김
    kRedrawWindowByArea(&(pstWindow->stArea));
    
    return TRUE;
}

// 특정 영역을 포함하는 윈도우는 모두 그림
BOOL kRedrawWindowByArea(const RECT *pstArea) {
    WINDOW *pstWindow;
    WINDOW *pstTargetWindow = NULL;
    RECT stOverlappedArea;
    RECT stCursorArea;

    // 화면 영역과 겹치는 영역이 없으면 그릴 필요 없음
    if(kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea,
        &stOverlappedArea) == FALSE) {
        return FALSE;
    }

    // Z 순서의 최하위, 즉 윈도우 리스트 첫 번쨰부터 마지막까지 루프를 돌면서
    // 업데이트할 영역과 겹치는 윈도우를 찾아 비디오 메모리로 전송
    kLock(&(gs_stWindowManager.stLock));

    // 현재 윈도우 리스트는 처음이 최하위 마지막이 최상위가 되도록 정렬됨
    // 따라서 윈도우 리스트를 처음부터 따라가면서 그릴 영역을 포함하는 윈도우를 찾고
    // 그 윈도우부터 최상위 윈도우까지 화면에 전송하면 됨
    pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
    while(pstWindow != NULL) {
        // 윈도우를 화면에 나타내는 옵션이 설정되 있으며,
        // 업데이트할 부분과 윈도우가 차지하는 영역이 겹치면 겹치는 만큼을 화면에 전송
        if((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
            (kIsRectangleOverlapped(&(pstWindow->stArea), &stOverlappedArea) == TRUE)) {
            kLock(&(pstWindow->stLock));

            // 실제로 비디오 메모리로 전송하는 ㅏㅎㅁ수
            kCopyWindowBufferToFrameBuffer(pstWindow, &stOverlappedArea);

            // 동기화 처리
            kUnlock(&(pstWindow->stLock));
        }

        // 다음 윈도우를 찾음
        pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
    }

    // 동기화 처리
    kUnlock(&(gs_stWindowManager.stLock));

    // 마우스 커서 영역이 포함되면 마우스 커서도 같이 그림
    // 마우스 영역을 RECT 자료구조에 설정
    kSetRectangleData(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY,
        gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH,
        gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT, &stCursorArea);

    // 겹치는지 확인하여 겹친다면 마우스 커서도 그림
    if(kIsRectangleOverlapped(&stOverlappedArea, &stCursorArea) == TRUE) {
        kDrawCursor(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY);
    }
}

// 윈도우 화면 버퍼의 일부 또는 전체를 프레임 버퍼로 복사
static void kCopyWindowBufferToFrameBuffer(const WINDOW *pstWindow, const RECT *pstCopyArea) {
    RECT stTempArea;
    RECT stOverlappedArea;
    int iOverlappedWidth;
    int iOverlappedHeight;
    int iScreenWidth;
    int iWindowWidth;
    int i;
    COLOR *pstCurrentVideoMemoryAddress;
    COLOR *pstCurrentWindowBufferAddress;

    // 전송해야 하는 영역과 화면 영역이 겹치는 부분을 임시로 계산
    if(kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstCopyArea, &stTempArea) == FALSE) {
        return;
    }

    // 윈도우 영역과 임시로 계산한 영역이 겹치는 부분을 다시 계산
    // 두 영역이 겹치지 않는다면 비디오 메로리로 전송할 필요 없음
    if(kGetOverlappedRectangle(&stTempArea, &(pstWindow->stArea), &stOverlappedArea) == FALSE) {
        return;
    }

    // 각 영역의 너비와 높이를 계산
    iScreenWidth = kGetRectangleWidth(&(gs_stWindowManager.stScreenArea));
    iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
    iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
    iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

    // 전송을 시작할 비디오 메모리 어드레스와 윈도우 화면 버퍼의 어드레스를 계산
    pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory +
        stOverlappedArea.iY1 * iScreenWidth + stOverlappedArea.iX1;
    
    // 윈도우 화면 버퍼는 화면 전체가 아닌 윈도우를 기준으로 한 좌표이므로,
    // 겹치는 영역을 윈도우 내부 좌표를 기준으로 변환
    pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer +
        (stOverlappedArea.iY1 - pstWindow->stArea.iY1) * iWindowWidth +
        (stOverlappedArea.iX1 - pstWindow->stArea.iX1);
    
    // 루프를 돌면서 윈도우 화면 버퍼의 내용을 비디오 메모리로 복사
    for(i = 0; i < iOverlappedHeight; i++) {
        // 라인별로 한 번에 전송
        kMemCpy(pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress,
            iOverlappedWidth * sizeof(COLOR));
        
        // 다음 라인으로 메모리 어드레스 이동
        pstCurrentVideoMemoryAddress += iScreenWidth;
        pstCurrentWindowBufferAddress += iWindowWidth;
    }
}

// 윈도우 내부에 그리는 함수와 마우스 커서 관련

// 윈도우 화면 버퍼에 윈도우 테두리 그리기
BOOL kDrawWindowFrame(QWORD qwWindowID) {
    WINDOW *pstWindow;
    RECT stArea;
    int iWidth;
    int iHeight;

    // 윈도우 검색과 동기화 처리
    pstWindow = kGetWindowWithWindowLock(qwWindowID);

    if(pstWindow == NULL) {
        return FALSE;
    }

    // 윈도우 너비와 높이 계산
    iWidth = kGetRectangleWidth(&(pstWindow->stArea));
    iHeight = kGetRectangleHeight(&(pstWindow->stArea));

    // 클리핑 영역 설정
    kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

    // 윈도우 프레임의 가장자리를 그림, 2 픽셀 두께
    kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
        0, 0, iWidth - 1, iHeight - 1, WINDOW_COLOR_FRAME, FALSE);
    
    kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
        1, 1, iWidth - 2, iHeight - 2, WINDOW_COLOR_FRAME, FALSE);
    
    // 동기화 처리
    kUnlock(&(pstWindow->stLock));

    return TRUE;
}

// 윈도우 화면 버퍼에 배경 그리기
BOOL kDrawWindowBackground(QWORD qwWindowID) {
    WINDOW *pstWindow;
    int iWidth;
    int iHeight;
    RECT stArea;
    int iX, iY;

    // 윈도우 검색과 동기화 처리
    pstWindow = kGetWindowWithWindowLock(qwWindowID);
    if(pstWindow == NULL) {
        return FALSE;
    }

    // 윈도우 너비와 높이 계산
    iWidth = kGetRectangleWidth(&(pstWindow->stArea));
    iHeight = kGetRectangleHeight(&(pstWindow->stArea));

    // 클리핑 영역 설정
    kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

    // 윈도우에 제목 표시줄이 있으면 그 아래부터 채움
    if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) {
        iY = WINDOW_TITLEBAR_HEIGHT;
    }
    else {
        iY = 0;
    }

    // 윈도우 테두리를 그리는 옵션이 설정되어 있으면 테두리를 제외한 영역을 채움
    if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME) {
        iX = 2;
    }
    else {
        iX = 0;
    }

    // 윈도우 내부 채움
    kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
        iX, iY, iWidth - 1 - iX, iHeight - 1 - iX, WINDOW_COLOR_BACKGROUND, TRUE);
    
    kUnlock(&(pstWindow->stLock));

    return TRUE;
}

