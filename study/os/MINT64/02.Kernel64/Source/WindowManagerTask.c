#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "MultiProcessor.h"
#include "Utility.h"

// 윈도우 매니저 태스크
void kStartWindowManager(void) {
    VBEMODEINFOBLOCK *pstVBEMode;
    int iRelativeX, iRelativeY;
    int iMouseX, iMouseY;
    BYTE bButton;
    QWORD qwWindowID;
    TCB *pstTask;
    char vcTempTitle[WINDOW_TITLEMAXLENGTH];
    int iWindowCount = 0;

    // 윈도우 매니저 TCB 반환
    pstTask = kGetRunningTask(kGetAPICID());

    // GUI 시스템 초기화
    kInitializeGUISystem();

    // 현재 마우스 위치에 커서 출력
    kGetCursorPosition(&iMouseX, &iMouseY);
    kMoveCursor(iMouseX, iMouseY);

    // 마우스 이동을 처리하고 마우스 버튼에 따라 윈도우 생성과 삭제 수행
    while(1) {
        // 마우스 데이터 수신 대기
        if(kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) == FALSE) {
            kSleep(0);
            continue;
        }

        // 현재 마우스 커서 위치 반환
        kGetCursorPosition(&iMouseX, &iMouseY);

        // 마우스가 움직인 거리를 이전 커서 위치에 더해서 현재 좌표 계산
        iMouseX += iRelativeX;
        iMouseY += iRelativeY;

        // 왼쪽 버튼이 눌러지면 윈도우 생성
        if(bButton & MOUSE_LBUTTONDOWN) {
            // 현재 마우스 커서 위치에 윈도우 생성. 아래 코드에서 윈도우 내부에
            // 문자를 출력한 뒤 윈도우를 표시하려고 윈도우 속성에서 WINDOW_FLAGS_SHOW를 제외
            // kSprintf(vcTempTitle, "MINT... %d", iWindowCount++);
            qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
                400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE, vcTempTitle);
            
            // 윈도우 내부에 텍스트를 출력하고 윈도우를 화면에 나타냄
            kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 10, RGB(0, 0, 0),
                WINDOW_COLOR_BACKGROUND, "This is real window~!!", 22);
            kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 30, RGB(0, 0, 0),
                WINDOW_COLOR_BACKGROUND, "No more prototype~!!", 18);
            kShowWindow(qwWindowID, TRUE);
        }
        // 오른쪽 버튼이 눌러지면 윈도우 매니저 태스크가 생성한 모든 윈도우 삭제
        else if(bButton & MOUSE_RBUTTONDOWN) {
            // 태스크가 생성한 모든 윈도우 삭제
            kDeleteAllWindowInTaskID(pstTask->stLink.qwID);
            iWindowCount = 0;
        }

        // 새로운 위치로 마우스 커서를 이동
        kMoveCursor(iMouseX, iMouseY);
    }
}