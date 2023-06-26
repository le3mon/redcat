#include "MINTOSLibrary.h"
#include "Main.h"
#include "HangulInput.h"

// c언어 엔트리 포인트
int Main(char *pcArgument) {
    QWORD qwWindowID;
    int iX, iY, iWidth, iHeight;
    EVENT stReceivedEvent;
    KEYEVENT *pstKeyEvent;
    WINDOWEVENT *pstWindowEvent;
    RECT stScreenArea;
    BUFFERMANAGER stBufferManager;
    BOOL bHangulMode;

    // 그래픽 모드 판단
    if(IsGraphicMode() == FALSE) {
        printf("This task can run only gui mode~\n");
        return -1;
    }

    // 윈도우 화면 가운데에 가로 세로 크기를 60문자 40픽셀로 형성
    GetScreenArea(&stScreenArea);
    iWidth = MAXOUTPUTLENGTH * FONT_ENGLISHWIDTH + 5;
    iHeight = 40;
    iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
    iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
    qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "한 줄 메모장(한/영 전환은 Alt 키)");

    // 버퍼 정보를 초기화하고 영문 입력 모드로 설정
    memset(&stBufferManager, 0, sizeof(stBufferManager));
    bHangulMode = FALSE;

    // 커서 메모 입력 영역 앞쪽에 세로로 길게 출력하고 윈도우를 다시 표시
    DrawRect(qwWindowID, 3, 4 + WINDOW_TITLEBAR_HEIGHT, 5, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT,
        RGB(0, 250, 0), TRUE);
    ShowWindow(qwWindowID, TRUE);

    // gui 태스크 무한 루프
    while(1) {
        // 이벤트 큐에서 이벤트 수신
        if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE) {
            Sleep(10);
            continue;
        }

        // 수신된 이벤트 타입에 따라 처리
        switch(stReceivedEvent.qwType) {
        case EVENT_KEY_DOWN:
            // 키 이벤트 추출
            pstKeyEvent = &(stReceivedEvent.stKeyEvent);

            // 입력된 키로 한글 조합하거나 영문 출력
            switch(pstKeyEvent->bASCIICode) {
            case KEY_LALT:
                // 한글 입력 모드 중에 alt 키가 눌러지면 한글 조합 종료
                if(bHangulMode == TRUE) {
                    // 키 입력 버퍼 초기화
                    stBufferManager.iInputBufferLength = 0;
                    if((stBufferManager.vcOutputBufferForProcessing[0] != '\0') &&
                        (stBufferManager.iOutputBufferLength + 2 < MAXOUTPUTLENGTH)) {
                        // 조합 중인 한글을 윈도우 화면에 출력하는 버퍼로 복사
                        memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                            stBufferManager.vcOutputBufferForProcessing, 2);
                        stBufferManager.iOutputBufferLength += 2;

                        // 조합 중이 한글을 저장하는 버퍼 초기화
                        stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                    }
                }
                // 영문 입력 모드 중에 Alt 킥 ㅏ눌러지면 한글 조합용 버퍼 초기화
                else {
                    stBufferManager.iInputBufferLength = 0;
                    stBufferManager.vcOutputBufferForComplete[0] = '\0';
                    stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                }
                bHangulMode = TRUE - bHangulMode;
                break;
            
            // 백스페이스 키 처리
            case KEY_BACKSPACE:
                // 한글을 조합하는 중이면 입력 버퍼 내용을 삭제하고 남은 키 입력 버퍼의 내용으로 한글 조합
                if((bHangulMode == TRUE) && (stBufferManager.iInputBufferLength > 0)) {
                    // 키 입력 버퍼의 내용을 하나 제거하고 한글 다시 조합
                    stBufferManager.iInputBufferLength--;
                    ComposeHangul(stBufferManager.vcInputBuffer,
                        &stBufferManager.iInputBufferLength,
                        stBufferManager.vcOutputBufferForProcessing,
                        stBufferManager.vcOutputBufferForComplete);
                }
                else {
                    if(stBufferManager.iOutputBufferLength > 0) {
                        // 화면 출력 버퍼에 들어 있는 내용이 2바이트 이상이고 버퍼에
                        // 저장된 값에 최상위 비트가 켜져 있으면 한글로 간주하고 마지막 2바이트 모두 삭제
                        if((stBufferManager.iOutputBufferLength >= 2) &&
                            (stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength - 1] & 0x80)) {
                            stBufferManager.iOutputBufferLength -= 2;
                            memset(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, 0, 2);
                        }
                        // 한글이 아니면 마지막 1바이트 삭제
                        else {
                            stBufferManager.iOutputBufferLength--;
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = '\0';
                        }
                    }
                }
                break;
            default:
                // 특수 키는 모두 무시 
                if(pstKeyEvent->bASCIICode & 0x80) {
                    break;
                }
                
                // 한글 조합 중이면 한글 조합 처리
                if((bHangulMode == TRUE) && (stBufferManager.iOutputBufferLength + 2 <= MAXOUTPUTLENGTH)) {
                    // 자/모음이 시프트와 조합되는 경우를 대비하여 쌍자음이나
                    // 쌍모음을 제외한 나머지는 소문자로 변환
                    ConvertJaumMoumToLowerCharactor(&pstKeyEvent->bASCIICode);

                    // 입력 버퍼에 키 입력 값을 채우고 데이터의 길이 증가
                    stBufferManager.vcInputBuffer[stBufferManager.iInputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iInputBufferLength++;

                    // 한글 조합에 필요한 버퍼를 넘겨줫 ㅓ한글 조합
                    if(ComposeHangul(stBufferManager.vcInputBuffer,
                        &stBufferManager.iInputBufferLength,
                        stBufferManager.vcOutputBufferForProcessing,
                        stBufferManager.vcOutputBufferForComplete) == TRUE) {
                        // 조합이 완료된 버퍼에 값이 있는가 확인하여 윈도우 화면에 출력할 버퍼로 복사
                        if(stBufferManager.vcOutputBufferForComplete[0] != '\0') {
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                                stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;

                            // 조합된 한글을 복사한 뒤에 현재 조합 중인 한글 출력할 공간이 없다면
                            // 키 입력 버퍼와 조합 중인 한글 버퍼 모두 초기화
                            if(stBufferManager.iOutputBufferLength + 2 > MAXOUTPUTLENGTH) {
                                stBufferManager.iInputBufferLength = 0;
                                stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                            }
                        }
                    }
                    // 조합에 실패하면 입력 버퍼에 마짐가으로 입력된 값이 한글 자 모음이 아니므로
                    // 한글 조합이 완료된 버퍼의 값과 입력 버퍼에 있는 값을 모두 출력 버퍼로 복사
                    else {
                        // 조합이 완료된 버퍼에 값이 있는가 확인하여 윈도우 화면 출력 버퍼 복사
                        if(stBufferManager.vcOutputBufferForComplete[0] != '\0') {
                            // 완성된 한글 출력 버퍼로 복사
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                                stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;
                        }

                        // 윈도우 화면에 출력하는 버퍼의 공간이 충분하면 키 입력 버퍼에 마지막으로
                        // 입력된 한글 자/모가 아닌 값 복사
                        if(stBufferManager.iOutputBufferLength < MAXOUTPUTLENGTH) {
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = stBufferManager.vcInputBuffer[0];
                            stBufferManager.iOutputBufferLength++;
                        }

                        // 키 입력 버퍼 비움
                        stBufferManager.iInputBufferLength = 0;
                    }
                }
                // 한글 입력 모드가 아닌 경우
                else if((bHangulMode == FALSE) && (stBufferManager.iOutputBufferLength + 1 <= MAXOUTPUTLENGTH)) {
                    // 키 입력을 그대로 윈도우 화면에 출력하는 버퍼로 저장
                    stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iOutputBufferLength++;
                }
                break;
            }

            // 화면 출력 버퍼에 있는 문자열 전부 출력
            DrawText(qwWindowID, 2, WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBuffer, MAXOUTPUTLENGTH);

            // 현재 조합중이 한글이 있다면 화면 출력 버퍼의 내용이 출력된 다음 위치에 조합 중인 한글 출력
            if(stBufferManager.vcOutputBufferForProcessing[0] != '\0') {
                DrawText(qwWindowID, 2 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, WINDOW_TITLEBAR_HEIGHT + 4,
                    RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBufferForProcessing, 2);
            }

            // 커서를 세로로 길게 출력
            DrawRect(qwWindowID, 3 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 4 + WINDOW_TITLEBAR_HEIGHT,
                5 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT, RGB(0, 250, 0), TRUE);

            ShowWindow(qwWindowID, TRUE);
            break;
        
        case EVENT_WINDOW_CLOSE:
            DeleteWindow(qwWindowID);
            return 0;
            break;
        
        default:
            break;
        }
    }

    return 0;
}

// 한글 자음과 모음 범위에서 쌍자음과 쌍모음을 제외한 나머지는 모두 소문자로 변환
void ConvertJaumMoumToLowerCharactor(BYTE *pbInput) {
    if((*pbInput < 'A') || (*pbInput > 'Z')) {
        return;
    }

    // 쌍자음 또는 쌍모음 여부 판별
    switch(*pbInput) {
        case 'Q':
        case 'W':
        case 'E':
        case 'R':
        case 'T':
        case 'O':
        case 'P':
            return;
            break;
    }

    *pbInput = TOLOWER(*pbInput);
}