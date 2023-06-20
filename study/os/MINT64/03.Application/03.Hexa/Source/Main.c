#include "MINTOSLibrary.h"
#include "Main.h"

// 게임 관련 정보 저장 자료구조
GAMEINFO g_stGameInfo = {0, };


// 응용프로그램 c언어 엔트리 포인트
int Main(char *pcArgument) {
    QWORD qwWindowID;
    EVENT stEvent;
    KEYEVENT *pstKeyEvent;
    QWORD qwLastTickCount;
    char *pcStartMessage = "Please LButton Down To Start~!";
    RECT stScreenArea;
    int iX, iY;
    BYTE bBlockKind;

    // 윈도우 화면 가운데 생성
    GetScreenArea(&stScreenArea);
    iX = (GetRectangleWidth(&stScreenArea) - WINDOW_WIDTH) / 2;
    iY = (GetRectangleHeight(&stScreenArea) - WINDOW_HEIGHT) / 2;
    qwWindowID = CreateWindow(iX, iY, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT, "Hexa");

    if(qwWindowID == WINDOW_INVALIDID) {
        printf("Window create fail\n");
        return -1;
    }

    // 게임 관련 정보 초기화 및 버퍼 할당
    // 게임 정보 초기화
    Initialize();

    // 난수 초기값 설정
    srand(GetTickCount());

    // 게임 정보 / 영역 출력 및 게임 시작 대기 메시지 표시
    DrawInformation(qwWindowID);
    DrawGameArea(qwWindowID);
    DrawText(qwWindowID, 7, 200, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));

    // 출력된 메시시 화면에 표시
    ShowWindow(qwWindowID, TRUE);


    qwLastTickCount = GetTickCount();
    // gui 태스크 이벤트와 게임 루프 처리
    while(1) {
        // 이벤트 처리
        if(ReceiveEventFromWindowQueue(qwWindowID, &stEvent) == TRUE) {
            // 수신된 이벤트 타입에 따라 처리
            switch(stEvent.qwType) {
            // 마우스 클릭 처리
            case EVENT_MOUSE_LBUTTONDOWN:
                // 게임 시작 원하는 클릭이면 게임 시작
                if(g_stGameInfo.bGameStart == FALSE) {
                    // 게임 정보 초기화
                    Initialize();

                    // 게임 시작 플래그 설정
                    g_stGameInfo.bGameStart = TRUE;
                    break;
                }
                break;
            
            // 키보드 눌림 처리
            case EVENT_KEY_DOWN:
                pstKeyEvent = &(stEvent.stKeyEvent);
                if(g_stGameInfo.bGameStart == FALSE) {
                    break;
                }

                switch(pstKeyEvent->bASCIICode) {
                // 왼쪽 이동
                case KEY_LEFT:
                    if(IsMovePossible(g_stGameInfo.iBlockX - 1, g_stGameInfo.iBlockY) == TRUE) {
                        g_stGameInfo.iBlockX -= 1;
                        DrawGameArea(qwWindowID);
                    }
                    break;
                
                // 오른쪽 이동
                case KEY_RIGHT:
                    if(IsMovePossible(g_stGameInfo.iBlockX + 1, g_stGameInfo.iBlockY) == TRUE) {
                        g_stGameInfo.iBlockX += 1;
                        DrawGameArea(qwWindowID);
                    }
                    break;
                
                // 움직이는 블록을 구성하는 작은 블록의 순서 변경
                case KEY_UP:
                    bBlockKind = g_stGameInfo.vbBlock[0];
                    memcpy(&(g_stGameInfo.vbBlock), &(g_stGameInfo.vbBlock[1]), BLOCKCOUNT - 1);
                    g_stGameInfo.vbBlock[BLOCKCOUNT - 1] = bBlockKind;

                    DrawGameArea(qwWindowID);
                    break;
                
                // 블록을 아래로 이동
                case KEY_DOWN:
                    if(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == TRUE) {
                        g_stGameInfo.iBlockY += 1;
                    }
                    DrawGameArea(qwWindowID);
                    break;
                
                // 블록을 아래로 끝까지 이동
                case ' ':
                    while(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == TRUE) {
                        g_stGameInfo.iBlockY += 1;
                    }
                    DrawGameArea(qwWindowID);
                    break;
                }

                // 변경된 내용 화면에 표시
                ShowWindow(qwWindowID, TRUE);
                break;
            
            // 윈도우 닫기 버튼 처리
            case EVENT_WINDOW_CLOSE:
                // 윈도우 삭제
                DeleteWindow(qwWindowID);
                return 0;
                break;
            }
        }

        // 게임 루프 처리
        // 게임 시작이 되었을 경우 레벨에 따라 일정 시간  대기한 뒤에 블록을 아래로 이동
        if((g_stGameInfo.bGameStart == TRUE) && 
            ((GetTickCount() - qwLastTickCount) > (300 - (g_stGameInfo.qwLevel * 10)))) {
            qwLastTickCount = GetTickCount();

            // 블록을 한 칸 아래로 내리고 더 이상 내릴 수 없다면 고정
            if(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == FALSE) {
                // 블록 고정을 할 수 없으면 게임 종료
                if(FreezeBlock(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY) == FALSE) {
                    g_stGameInfo.bGameStart = FALSE;

                    // 게임 종료 메시지 출력
                    DrawText(qwWindowID, 82, 230, RGB(255, 255, 255), RGB(0, 0, 0), "Game Over~!!!", 13);
                    DrawText(qwWindowID, 7, 250, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));
                }

                // 보드에 블록을 검사해 세 개 이상 연속된 블록을 삭제하고 화면 표시
                EraseAllContinuousBlockOnBoard(qwWindowID);

                // 새로운 블록 생성
                CreateBlock();
            }
            else {
                g_stGameInfo.iBlockY++;

                // 게임 영역을 새로 그림
                DrawGameArea(qwWindowID);
            }

            // 변경된 내용 화면에 표시
            ShowWindow(qwWindowID, TRUE);
        }
        else {
            Sleep(0);
        }
    }
}

// 게임 정보 초기화
void Initialize(void) {
    // 게임 정보 자료구조 초기화
    memset(&g_stGameInfo, 0, sizeof(g_stGameInfo));

    // 블록 색깔 설정
    g_stGameInfo.vstBlockColor[1] = RGB(230, 0, 0);
    g_stGameInfo.vstBlockColor[2] = RGB(0, 230, 0);
    g_stGameInfo.vstBlockColor[3] = RGB(230, 0, 230);
    g_stGameInfo.vstBlockColor[4] = RGB(230, 230, 0);
    g_stGameInfo.vstBlockColor[5] = RGB(0, 230, 230);
    g_stGameInfo.vstEdgeColor[1] = RGB(150, 0, 0);
    g_stGameInfo.vstEdgeColor[2] = RGB(0, 150, 0);
    g_stGameInfo.vstEdgeColor[3] = RGB(150, 0, 150);
    g_stGameInfo.vstEdgeColor[4] = RGB(150, 150, 0);
    g_stGameInfo.vstEdgeColor[5] = RGB(0, 150, 150);

    // 움직이는 블록 위치 설정
    g_stGameInfo.iBlockX = -1;
    g_stGameInfo.iBlockX = -1;
}

// 움직이는 블록 생성
void CreateBlock(void) {
    int i;

    // 블록 시작 좌표 위쪽 가운데 가장 아래 블록부터 화면에 표시되도록 함
    g_stGameInfo.iBlockX = BOARDWIDTH / 2;
    g_stGameInfo.iBlockY = -BLOCKCOUNT;

    // 움직이는 블록을 구성하는 작은 블록의 종류 결정
    for(i = 0; i < BLOCKCOUNT; i++) {
        g_stGameInfo.vbBlock[i] = (rand() % BLOCKKIND) + 1;
    }
}

// 움직이는 블록을 특정 위치로 옮길 수 있는지 확인
BOOL IsMovePossible(int iBlockX, int iBlockY) {
    // 블록 좌표가 게임 영역 안인지 확인
    if((iBlockX < 0) || (iBlockX >= BOARDWIDTH) || ((iBlockY + BLOCKCOUNT) > BOARDHEIGHT)) {
        return FALSE;
    }

    // 마지막 블록이 위치할 곳을 확인해 게임판에 움직일 영역이 비어 있지 않으면 실패
    if(g_stGameInfo.vvbBoard[iBlockY + BLOCKCOUNT - 1][iBlockX] != EMPTYBLOCK) {
        return FALSE;
    }

    return TRUE;
}

// 블록을 게임판에 고정
BOOL FreezeBlock(int iBlockX, int iBlockY) {
    int i;

    // 블록을 고정하는 위치가 0보다 작으면 현재 위치에 블록이 가득 찼다는 뜻이므로 실패
    if(iBlockY < 0) {
        return FALSE;
    }

    // 움직이는 블록을 현재 좌표 그대로 게임판에 고정
    for(i = 0; i < BLOCKCOUNT; i++) {
        g_stGameInfo.vvbBoard[iBlockY + i][iBlockX] = g_stGameInfo.vbBlock[i];
    }

    // 블록이 고정되었으므로 블록의 x축 좌표를 -1로 설정
    g_stGameInfo.iBlockX = -1;
    return TRUE;
}

// 가로 방향으로 일치하는 블록을 찾아서 표시
BOOL MarkContinuousHorizonBlockOnBoard(void) {
    int iMatchCount;
    BYTE bBlockKind;
    int i, j, k;
    BOOL bMarked;

    bMarked = FALSE;

    // 게임판 전체를 검색해 가로 방향으로 세 개 이상인 것을 찾아 표시
    for(j = 0; j < BOARDHEIGHT; j++) {
        iMatchCount = 0;
        bBlockKind = 0xFF;
        
        for(i = 0; i < BOARDWIDTH; i++) {
            // 첫 번째 블록이면 블록 종류 저장
            if((iMatchCount == 0) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)) {
                bBlockKind = g_stGameInfo.vvbBoard[j][i];
                iMatchCount++;
            }
            else {
                // 종류가 일치하면 일치한 블록의 수 증가
                if(g_stGameInfo.vvbBoard[j][i] ==  bBlockKind) {
                    iMatchCount++;

                    // 3개 연속이면 이전 3개의 블록을 지울 블록으로 표시
                    if(iMatchCount == BLOCKCOUNT) {
                        for(k = 0; k < iMatchCount; k++) {
                            g_stGameInfo.vvbEraseBlock[j][i - k] = ERASEBLOCK;
                        }

                        // 표시된 것이 있는 것으로 저장
                        bMarked = TRUE;
                    }

                    // 연속된 블록이 네 개 이상이면 즉시 지울 블록으로 표시
                    else if(iMatchCount > BLOCKCOUNT) {
                        g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
                    }
                }
                // 일치하지 않으면 새로운 블록으로 설정
                else {
                    if(g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) {
                        // 새로운 블록 설정
                        iMatchCount = 1;
                        bBlockKind = g_stGameInfo.vvbBoard[j][i];
                    }
                    else {
                        iMatchCount = 0;
                        bBlockKind = 0xFF;
                    }
                }
            }
        }
    }

    return bMarked;
}

// 세로 방향으로 일치하는 블록 찾아서 표시
BOOL MarkContinuousVerticalBlockOnBoard(void) {
    int iMatchCount;
    BYTE bBlockKind;
    int i, j, k;
    BOOL bMarked;

    bMarked = FALSE;
    // 게임판 전체를 검색해 세로 방향으로 세 개 이상인 것을 찾아서 표시
    for(i = 0; i < BOARDWIDTH; i++) {
        iMatchCount = 0;
        bBlockKind = 0xFF;

        for(j = 0; j < BOARDHEIGHT; j++) {
            // 첫 번째 블록이면 블록 종류 저장
            if((iMatchCount == 0) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)) {
                bBlockKind = g_stGameInfo.vvbBoard[j][i];
                iMatchCount++;
            }
            else {
                // 종류가 일치하면 일치한 블록 수 증가
                if(g_stGameInfo.vvbBoard[j][i] == bBlockKind) {
                    iMatchCount++;

                    // 3개 이상 연속되면 이전 3블록 모두 지울 블록으로 표시
                    if(iMatchCount == BLOCKCOUNT) {
                        for(k = 0; k < iMatchCount; k++) {
                            g_stGameInfo.vvbEraseBlock[j - k][i] = ERASEBLOCK;
                        }

                        bMarked = TRUE;
                    }
                    // 연속된 블록이 4개 이상이면 바로 지울 블록으로 표시
                    else if(iMatchCount > BLOCKCOUNT) {
                        g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
                    }
                }
                // 일치하지 않으면 새로운 블록 설정
                else {
                    if(g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) {
                        iMatchCount = 1;
                        bBlockKind = g_stGameInfo.vvbBoard[j][i];
                    }
                    else {
                        iMatchCount = 0;
                        bBlockKind = 0xFF;
                    }
                }
            }
        }
    }

    return bMarked;
}

// 대각선 방향으로 일치하는 블록 찾아서 표시
BOOL MarkContinuousDiagonalBlockInBoard(void) {
    int iMatchCount;
    BYTE bBlockKind;
    int i, j, k, l;
    BOOL bMarked;

    bMarked = FALSE;

    // 게임판 전체를 검색해 위 아래 대각선 방향으로 세 개 이상인 것을 찾아 표시
    for(i = 0; i < BOARDWIDTH; i++) {
        for(j = 0; j < BOARDHEIGHT; j++) {
            iMatchCount = 0;
            bBlockKind = 0xFF;
            
            for(k = 0; ((i + k) < BOARDWIDTH) && ((j + k) < BOARDHEIGHT); k++) {
                // 첫 번째 블록 종류 저장
                if((iMatchCount == 0) && (g_stGameInfo.vvbBoard[j + k][i + k] != EMPTYBLOCK)) {
                    bBlockKind = g_stGameInfo.vvbBoard[j + k][i + k];
                    iMatchCount++;
                }
                else {
                    // 종류가 일치하면 일치한 블록 수 증가
                    if(g_stGameInfo.vvbBoard[j + k][i + k] == bBlockKind) {
                        iMatchCount++;

                        if(iMatchCount == BLOCKCOUNT) {
                            for(l = 0; l < iMatchCount; l++) {
                                g_stGameInfo.vvbEraseBlock[j + k - l][i + k - l] = ERASEBLOCK;
                            }
                            bMarked = TRUE;
                        }
                        // 연속된 블록이 네 개 이상이면 즉시 지울 블록으로 표시
                        else if(iMatchCount > BLOCKCOUNT) {
                            g_stGameInfo.vvbEraseBlock[j + k][i + k] = ERASEBLOCK;
                        }
                    }
                    // 일치하지 않으면 새로운 블록으로 설정
                    else {
                        if(g_stGameInfo.vvbBoard[j + k][i + k] != EMPTYBLOCK) {
                            iMatchCount = 1;
                            bBlockKind = g_stGameInfo.vvbBoard[j + k][i + k];
                        }
                        else {
                            iMatchCount = 0;
                            bBlockKind = 0xFF;
                        }
                    }
                }
            }
        }
    }

    // 게임판 전체를 검색해 아래에서 위의 대각선 방향으로 세 개 이상인 것을 찾아 표시
    for(i = 0; i < BOARDWIDTH; i++) {
        for(j = 0; j < BOARDHEIGHT; j++) {
            iMatchCount = 0;
            bBlockKind = 0xFF;

            for(k = 0; ((i + k) < BOARDWIDTH) && ((j - k) >= 0); k++) {
                // 첫 번째 블록 종류 저장
                if((iMatchCount == 0) && (g_stGameInfo.vvbBoard[j - k][i + k] != EMPTYBLOCK)) {
                    bBlockKind = g_stGameInfo.vvbBoard[j - k][i + k];
                    iMatchCount++;
                }
                else {
                    // 종류가 일치하면 일치한 블록 수 증가
                    if(g_stGameInfo.vvbBoard[j - k][i + k] == bBlockKind) {
                        iMatchCount++;

                        if(iMatchCount == BLOCKCOUNT) {
                            for(l = 0; l < iMatchCount; l++) {
                                g_stGameInfo.vvbEraseBlock[j - k + l][i + k - l] = ERASEBLOCK;
                            }
                            bMarked = TRUE;
                        }
                        // 연속된 블록이 네 개 이상이면 즉시 지울 블록으로 표시
                        else if(iMatchCount > BLOCKCOUNT) {
                            g_stGameInfo.vvbEraseBlock[j - k][i + k] = ERASEBLOCK;
                        }
                    }
                    // 일치하지 않으면 새로운 블록으로 설정
                    else {
                        if(g_stGameInfo.vvbBoard[j - k][i + k] != EMPTYBLOCK) {
                            iMatchCount = 1;
                            bBlockKind = g_stGameInfo.vvbBoard[j - k][i + k];
                        }
                        else {
                            iMatchCount = 0;
                            bBlockKind = 0xFF;
                        }
                    }
                }
            }
        }
    }

    return bMarked;
}

// 제거할 블록으로 표시된 블록을 게임판에서 제거하고 점수 증가
void EraseMarkedBlock(void) {
    int i, j;

    for(j = 0; j < BOARDHEIGHT; j++) {
        for(i = 0; i < BOARDWIDTH; i++) {
            // 지울 블록이면 겡미판에서 삭제
            if(g_stGameInfo.vvbEraseBlock[j][i] == ERASEBLOCK) {
                // 게임판에서 블록을 빈 것으로 설정
                g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;

                // 점수 증가
                g_stGameInfo.qwScore++;
            }
        }
    }
}

// 빈 영역의 블록을 아래로 이동
void CompactBlockOnBoard(void) {
    int i, j, iEmptyPosition;

    // 게임판 모든 영역을 돌면서 빈 영역에 블록을 채움
    for(i = 0; i < BOARDWIDTH; i++) {
        iEmptyPosition = -1;

        // 아래에서 위로 올라가면서 빈 영역을 찾아 블록을 채움
        for(j = BOARDHEIGHT - 1; j >= 0; j--) {
            // 빈 블록이면 현재 위치를 저장해뒁ㅆ다가 블록을 옮길 때 사용
            if((iEmptyPosition == -1) && (g_stGameInfo.vvbBoard[j][i] == EMPTYBLOCK)) {
                iEmptyPosition = j;
            }
            // 중간에 빈 블록이 검출되고 현재 위치에 블록이 있으면 아래로 이동
            else if((iEmptyPosition != -1) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)) {
                g_stGameInfo.vvbBoard[iEmptyPosition][i] = g_stGameInfo.vvbBoard[j][i];

                // 빈 블록 좌표의 y좌표를 위로 한 칸 올려서 계속 쌓아 올릴 수 있도록 함
                iEmptyPosition--;

                // 현재 위치의 블록이 옮겨졌으면 빈 블록으로 설정
                g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;
            }
        }
    }
}

// 더 이상 제거할 블록이 없을 때까지 반복해 게임판의 블록을 제거하고 압축
void EraseAllContinuousBlockOnBoard(QWORD qwWindowID) {
    BOOL bMarked;

    // 게임 정보 자료구조에 있는 삭제할 블록 필드 초기화
    memset(g_stGameInfo.vvbEraseBlock, 0, sizeof(g_stGameInfo.vvbEraseBlock));

    while(1) {
        // 블록을 제거하기 전에 잠시 대기하여 현재 게임판의 상태와 블록의 상태 유지
        Sleep(300);

        bMarked = FALSE;

        // 가로 방향 삭제할 블록 표시
        bMarked |= MarkContinuousHorizonBlockOnBoard();

        // 세로 방향 삭제할 블록 표시
        bMarked |= MarkContinuousVerticalBlockOnBoard();

        // 대각선 방향 삭제할 블록 표시
        bMarked |= MarkContinuousDiagonalBlockInBoard();

        // 제거할 블록이 없으면 더 이상 수행할 필요가 없음
        if(bMarked == FALSE) {
            break;
        }

        // 표시된 블록 제거
        EraseMarkedBlock();

        // 블록을 위에서 아래로 이동시켜 빈 영역 채움
        CompactBlockOnBoard();

        // 게임의 레벨을 30점 단위로 증가
        g_stGameInfo.qwLevel = (g_stGameInfo.qwScore / 30) + 1;

        // 지운 블록이 있으면 겡미 정보 영역과 겡미 영역 다시 그림
        DrawGameArea(qwWindowID);
        DrawInformation(qwWindowID);

        // 윈도우 화면 표시
        ShowWindow(qwWindowID, TRUE);
    }
}

// 게임 정보 화면 출력
void DrawInformation(QWORD qwWindowID) {
    char vcBuffer[200];
    int iLength;

    // 게임 정보 영역의 배경 출력
    DrawRect(qwWindowID, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2,
        WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT, RGB(55, 215, 47), TRUE);
    
    // 임시 버퍼에 출력 정보 저장
    sprintf(vcBuffer, "Level: %d, Score: %d\n", g_stGameInfo.qwLevel, g_stGameInfo.qwScore);
    iLength = strlen(vcBuffer);

    // 저장된 정보를 게임 정보 표시 영역 가운데 출력
    DrawText(qwWindowID, (WINDOW_WIDTH - iLength * FONT_ENGLISHWIDTH) / 2,
        WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), RGB(55, 215, 47), vcBuffer, strlen(vcBuffer));
}

// 게임 영역 화면에 출력
void DrawGameArea(QWORD qwWindowID) {
    COLOR stColor;
    int i, j, iY;

    // 게임 영역이 시작되는 위치
    iY = WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT;

    // 게임 영역 배경 출력
    DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH,
        iY + BLOCKSIZE * BOARDHEIGHT, RGB(0, 0, 0), TRUE);
    
    // 게임판 내용 화면에 표시
    for(j = 0; j < BOARDHEIGHT; j++) {
        for(i = 0; i < BOARDWIDTH; i++) {
            // 빈 블록이 아니면 블록 표시
            if(g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) {
                // 블록 내부 그림
                stColor = g_stGameInfo.vstBlockColor[g_stGameInfo.vvbBoard[j][i]];
                DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE),
                    (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE), stColor, TRUE);
                
                // 블록 외부 테두리 그림
                stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vvbBoard[j][i]];
                DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE),
                    (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE), stColor, FALSE);
                stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vvbBoard[j][i]];
                DrawRect(qwWindowID, i * BLOCKSIZE + 1, iY + (j * BLOCKSIZE) + 1,
                    (i + 1) * BLOCKSIZE - 1, iY + ((j + 1) * BLOCKSIZE) - 1, stColor, FALSE);
            }
        }
    }

    // 현재 움직이는 블록 화면에 표시
    if(g_stGameInfo.iBlockX != -1) {
        for(i = 0; i < BLOCKCOUNT; i++) {
            // 제목 표시줄 아래 블록이 표시될 때만 표시
            if(WINDOW_TITLEBAR_HEIGHT < (iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE))) {
                // 블록 내부 그림
                stColor = g_stGameInfo.vstBlockColor[g_stGameInfo.vbBlock[i]];
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE,
                    iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
                    (g_stGameInfo.iBlockX + 1) * BLOCKSIZE,
                    iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE), stColor, TRUE);

                // 블록 외부 테두리를 그림
                stColor = g_stGameInfo.vstEdgeColor[g_stGameInfo.vbBlock[i]];
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE,
                    iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
                    (g_stGameInfo.iBlockX + 1) * BLOCKSIZE,
                    iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE), stColor, FALSE);
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE + 1,
                    iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE) + 1,
                    (g_stGameInfo.iBlockX + 1) * BLOCKSIZE - 1,
                    iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE) - 1, stColor, FALSE);
            }
        }
    }

    // 게임 여역 테두리 그림
    DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH - 1, iY + BLOCKSIZE * BOARDHEIGHT - 1, RGB(0, 255, 0), FALSE);
}