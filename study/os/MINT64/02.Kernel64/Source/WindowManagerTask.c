#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "MultiProcessor.h"
#include "Utility.h"
#include "GUITask.h"

// 윈도우 매니저 태스크
void kStartWindowManager(void) {
    int iMouseX, iMouseY;
    BOOL bMouseDataResult;
    BOOL bKeyDataResult;
    BOOL bEventQueueResult;

    // GUI 시스템 초기화
    kInitializeGUISystem();

    // 현재 마우스 위치에 커서 출력
    kGetCursorPosition(&iMouseX, &iMouseY);
    kMoveCursor(iMouseX, iMouseY);

    // 윈도우 매니저 태스크 루프
    while(1) {
        // 마우스 데이터를 처리
        bMouseDataResult = kProcessMouseData();

        // 키 데이터 처리
        bKeyDataResult = kProcessKeyData();

        // 윈도우 매니저의 이벤트 큐에 수신된 데이터를 추리. 수신된 모든 이벤트를 처리함
        bEventQueueResult = FALSE;
        while(kProcessEventQueueData() == TRUE) {
            bEventQueueResult = TRUE;
        }

        // 처리한 데이터가 하나도 없다면 Sleep()을 수행하여 프로세서 양보
        if((bMouseDataResult == FALSE) && (bKeyDataResult == FALSE) && (bEventQueueResult == FALSE)) {
            kSleep(0);
        }
    }
}

// 수신된 마우스 데이터를 처리
BOOL kProcessMouseData(void) {
    QWORD qwWindowIDUnderMouse;
    BYTE bButtonStatus;
    int iRelativeX, iRelativeY;
    int iMouseX, iMouseY;
    int iPreviousMouseX, iPreviousMouseY;
    BYTE bChangeButton;
    RECT stWindowArea;
    EVENT stEvent;
    WINDOWMANAGER *pstWindowManager;
    char vcTempTitle[WINDOW_TITLEMAXLENGTH];
    static int iWindowCount = 0;
    QWORD qwWindowID;
    int i;

    // 마우스 데이터 수신 대기
    if(kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) == FALSE) {
        return FALSE;
    }

    // 윈도우 매니저 반환
    pstWindowManager = kGetWindowManager();
    
    // 마우스 이벤트 통합
    for(i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++) {
        // 마우스 데이터 수신 대기
        if(kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) == FALSE) {
            // 처음으로 확인했는데 데이터가 없다면 종료
            if(i == 0) {
                return FALSE;
            }
            // 처음이 아닌 경우 이전 로프에서 마우스 이벤트를 수신했으므로 통합한 이벤트 처리
            else {
                break;
            }
        }

        // 현재 마우스 커서 위치 반환
        kGetCursorPosition(&iMouseX, &iMouseY);

        // 처음 마우스 이벤트가 수신된 것이면 현재 좌표를 이전 마우스 위치로 저장
        if(i == 0) {
            // 움직이기 이전의 좌표를 저장
            iPreviousMouseX = iMouseX;
            iPreviousMouseY = iMouseY;
        }

        // 마우스가 움직인 거리를 이전 커서 위치에 더해서 현재 좌표 계산
        iMouseX += iRelativeX;
        iMouseY += iRelativeY;

        // 새로운 위치로 마우스 커서 이동 후 다시 현재 커서 위치 반환
        // 마우스 커서가 화면을 벗어나지 못하도록 방지 처리
        kMoveCursor(iMouseX, iMouseY);
        kGetCursorPosition(&iMouseX, &iMouseY);

        // 버튼 상태는 이전 버튼 상태와 현재 버튼 상태를 XOR 하여 1로 설정도ㅒㅆ는지 확인
        bChangeButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;

        // 마우스가 움직였으나 버튼의 변화가 있다면 바로 이벤트 처리
        if(bChangeButton != 0) {
            break;
        }
    }

    // 현재 마우스 커서 아래에 있는 윈도우 검색
    qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);

    // 마우스 왼쪽 버튼에 변화가 생긴 경우 처리
    if(bChangeButton & MOUSE_LBUTTONDOWN) {
        // 왼쪽 버튼이 눌린 경우 처리
        if(bButtonStatus & MOUSE_LBUTTONDOWN) {
            // 마우스로 윈도을 선택한 것이므로, 마우스 아래 윈도우가
            // 배경 윈도우가 아닌 경우 최상위로 이동
            if(qwWindowIDUnderMouse != pstWindowManager->qwBackgroundWindowID) {
                // 선택된 윈도우를 최상위로 만듬
                // 윈도우를 최상위로 만들면서 동시에 윈도우 선택과 선택 해제 이벤트도 같이 전송
                kMoveWindowToTop(qwWindowIDUnderMouse);
            }

            // 왼쪽 버튼이 눌린 위치가 제목 표시줄 위치이면 윈도우 이동인지
            // 또는 닫기 버튼 위치에서 눌렸는지 확인하여 처리
            if(kIsInTitleBar(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE) {
                // 닫기 버튼에서 눌러졌으면 윈도우에 닫기 전송
                if(kIsInCloseButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE) {
                    // 윈도우 닫기 이벤트 전송
                    kSetWindowEvent(qwWindowIDUnderMouse, EVENT_WINDOW_CLOSE, &stEvent);

                    kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

                    // 테스트용 코드
                    kDeleteWindow(qwWindowIDUnderMouse);
                }
                // 닫기 버튼이 아니면 윈도우 이동 모드로 변경
                else {
                    // 윈도우 이동 모드 설정
                    pstWindowManager->bWindowMoveMode = TRUE;

                    // 현재 윈도우를 이동하는 윈도우로 설정
                    pstWindowManager->qwMovingWindowID = qwWindowIDUnderMouse;
                }
            }
            // 윈도우 내부에서 눌린 것이면 왼쪽 버튼이 눌러졌음을 전송
            else {
                // 왼쪽 버튼 눌림 이벤트 전송
                kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONDOWN,
                    iMouseX, iMouseY, bButtonStatus, &stEvent);
                kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
            }
        }
        // 왼쪽 버튼이 떨어진 경우 처리
        else {
            // 윈도우가 이동 중이면 모드만 해제
            if(pstWindowManager->bWindowMoveMode == TRUE) {
                // 이동 중이라는 플래그 해제
                pstWindowManager->bWindowMoveMode = FALSE;

                pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
            }
            // 윈도우가 이동중이 아니면 왼쪽 버튼 떨어짐 이벤트 전송
            else {
                // 왼쪽 버튼 떨어짐 이벤트 전송
                kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONUP,
                    iMouseX, iMouseY, bButtonStatus, &stEvent);
                
                kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
            }
        }
    }
    // 마우스 오른쪽 버튼에 변화가 생긴 경우 처리
    else if(bChangeButton & MOUSE_RBUTTONDOWN) {
        // 오른쪽 버튼이 눌린 경우 처리
        if(bButtonStatus & MOUSE_RBUTTONDOWN) {
            // 오른쪽 버튼 눌림 이벤트 전송
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

            // 테스트용 코드
            kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, NULL, NULL,
                (QWORD)kHelloWorldGUITask, TASK_LOADBALANCINGID);
        }
        else {
            // 오른쪽 벝느 떨어짐 이벤트 전송
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONUP,
                iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
    }
    // 마우스 가운데 버튼에 변화가 생긴 경우 처리
    else if(bChangeButton & MOUSE_MBUTTONDOWN) {
        // 가운데 버튼이 눌린 경우 처리
        if(bButtonStatus & MOUSE_MBUTTONDOWN) {
            // 가운데 버튼 눌림 이벤트 전송
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONDOWN,
                iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
        else {
            // 가운데 버튼 떨어짐 이벤트 전송
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONUP,
                iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
    }
    // 마우스 버튼이 변경되지 않았으면 마우스 이동 처리만 수행
    else {
        // 마우스 이동 이벤트 전송
        kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MOVE,
            iMouseX, iMouseY, bButtonStatus, &stEvent);
        kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
    }

    // 윈도우가 이동 중이면 윈도우 이동 처리
    if(pstWindowManager->bWindowMoveMode == TRUE) {
        // 윈도우 위치 얻음
        if(kGetWindowArea(pstWindowManager->qwMovingWindowID, &stWindowArea) == TRUE) {
            // 윈도우의 현재 위치를 얻어서 마우스가 이동한 만큼 이동
            // 윈도우 이동 이벤트는 함수 내부에서 전달
            kMoveWindow(pstWindowManager->qwMovingWindowID,
                stWindowArea.iX1 + iMouseX - iPreviousMouseX,
                stWindowArea.iY1 + iMouseY - iPreviousMouseY);
        }
        // 윈도우 위치를 못 얻으면 윈도우가 존재하지 않은 것 이므로 이동 모드 해제
        else {
            // 이동 중 플래그 해제
            pstWindowManager->bWindowMoveMode = FALSE;
            pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
        }
    }

    // 다음 처리에 사용하려고 현재 버튼 상태 저장
    pstWindowManager->bPreviousButtonStatus = bButtonStatus;
    
    return TRUE;
}

// 수신된 키 데이터 처리
BOOL kProcessKeyData(void) {
    KEYDATA stKeyData;
    EVENT stEvent;
    QWORD qwActiveWindowID;

    // 키보드 데이터가 수신되기 대기
    if(kGetKeyFromKeyQueue(&stKeyData) == FALSE) {
        return FALSE;
    }

    // 최상위 윈도우, 즉 선택된 윈도우로 메시지 전송
    qwActiveWindowID = kGetTopWindowID();
    kSetKeyEvent(qwActiveWindowID, &stKeyData, &stEvent);
    
    return kSendEventToWindow(qwActiveWindowID, &stEvent);
}

// 이벤트 큐에 수신된 이벤트 처리
BOOL kProcessEventQueueData(void) {
    EVENT vstEvent[WINDOWMANAGER_DATAACCUMULATECOUNT];
    int iEventCount;
    WINDOWEVENT *pstWindowEvent;
    WINDOWEVENT *pstNextWindowEvent;
    QWORD qwWindowID;
    RECT stArea;
    RECT stOverlappedArea;
    int i, j;

    // 윈도우 매니저 태스크의 이벤트 큐에 수신된 이벤트를 통합하는 부분
    for(i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++) {
        // 이벤트 수신 대기
        if(kReceiveEventFromWindowManagerQueue(&(vstEvent[i])) == FALSE) {
            // 처음부터 이벤트가 수신되지 않았으면 종료
            if(i == 0) {
                return FALSE;
            }
            else {
                break;
            }
        }

        pstWindowEvent = &(vstEvent.stWindowEvent);

        // 윈도우 ID로 업데이트하는 이벤트가 수신되면 윈도우 영역을 이벤트 데이터에 삽입
        if(vstEvent[i].qwType == EVENT_WINDOWMANAGER_UPDATESCREENBYID) {
            // 윈도우의 크기를 이벤트 자료구조에 삽입
            if(kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == FALSE) {
                kSetRectangleData(0, 0, 0, 0, &(pstWindowEvent->stArea));
            }
            else {
                kSetRectangleData(0, 0, kGetRectangleWidth(&stArea) - 1,
                    kGetRectangleHeight(&stArea) - 1, &(pstWindowEvent->stArea));
            }
        }
    }

    // 저장된 이벤트를 검사하면서 합칠 수 있는 이벤트는 하나로 만듬
    iEventCount = i;

    for(j = 0; j < iEventCount; j++) {
        // 수신된 이벤트 중에 이벤트 중에서 이번에 처리할 것과 같은 윈도우에서
        // 발생하는 윈도우 이벤트 검색
        pstWindowEvent = &(vstEvent[j].stWindowEvent);
        if((vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
            (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
            (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) {
            continue;
        }

        // 수신한 이벤트의 끝까지 루프를 수행하면서 수신된 이벤트를 검사
        for(i = j + 1; i < iEventCount; i++) {
            pstNextWindowEvent = &(vstEvent[i].stWindowEvent);

            // 화면 업데이트가 아니거나 윈도우 ID가 일치하지 않으면 제외
            if(((vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
                (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
                (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) ||
                (pstWindowEvent->qwWindowID != pstNextWindowEvent->qwWindowID)) {
                continue;
            }

            // 겹치는 영역을 계산하여 겹치지 않으면 제외
            if(kGetOverlappedRectangle(&(pstWindowEvent->stArea),
                &(pstNextWindowEvent->stArea), &stOverlappedArea) == FALSE) {
                continue;
            }

            // 두 영역이 일치하거나 어느 한쪽이 포함되면 이벤트 통합
            if(kMemCmp(&(pstWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0) {
                // 현재 이벤트의 윈도우 영역이 겹치는 영역과 일치한다면
                // 다음 이벤트의 윈도우 영역이 현재 윈도우 영역과 같거나 포함
                // 따라서 현재 이벤트에 다음 이벤트의 윈도우 영역을 복사하고
                // 다음 이벤트는 삭제
                kMemCpy(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea), sizeof(RECT));
                vstEvent[i].qwType = EVENT_UNKNOWN;
            }
            else if(kMemCmp(&(pstNextWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0) {
                vstEvent[i].qwType = EVENT_UNKNOWN;
            }
        }
    }

    // 통합된 이벤트 모두 처리
    for(i = 0; i < iEventCount; i++) {
        pstWindowEvent = &(vstEvent[i].stWindowEvent);

        // 타입별로 처리
        switch (vstEvent[i].qwType) {
            // 현재 윈도우가 있는 영역을 화면에 업데이트
        case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
            // 윈도우의 내부 영역을 화면에 업데이트
        case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
            // 윈도우를 기준으로 한 좌표를 화면 좌표로 변호나하여 업데이트 처리
            if(kConvertRectClientToScreen(pstWindowEvent->qwWindowID,
                &(pstWindowEvent->stArea), &stArea) == TRUE) {
                // 윈도우 영역은 위엣 ㅓ했으므로 그대로 화면 업데이트 함수 호출
                kRedrawWindowByArea(&stArea, pstWindowEvent->qwWindowID);
            }
            break;

            // 화면 좌표로 전달된 영역을 화면에 업데이트
        case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
            kRedrawWindowByArea(&(pstWindowEvent->stArea), WINDOW_INVALIDID);
            break;
        default:
            break;
        }
    }

    return TRUE;
}