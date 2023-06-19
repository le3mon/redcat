#ifndef __MAIN_H__
#define __MAIN_H__

#include "../../UserLibrary/Source/Types.h"

// 매크로
// 물방울의 최대 개수
#define MAXBUBBLECOUNT  50

// 물방울 반지름
#define RADIUS          16

// 물방울 기본속도
#define DEFAULTSPEED    3

// 플레이어 최대 생명
#define MAXLIFE         20

// 윈도우 너비 및 높이
#define WINDOW_WIDTH    250
#define WINDOW_HEIGHT   350

// 게임 정보 영역의 높이
#define INFORMATION_HEIGHT  20

// 구조체
// 물방울의 정보를 저장하는 자료구조
typedef struct BubbleStruct {
    // X, Y 좌표
    QWORD qwX;
    QWORD qwY;

    // 떨어지는 속도
    QWORD qwSpeed;

    // 물방울 색깔
    COLOR stColor;

    // 생존 여부
    BOOL bAlive;
} BUBBLE;

// 게임 정보를 저장하는 자료구조
typedef struct GameInfoStruct {
    // 물방울 정보 저장 버퍼
    BUBBLE *pstBubbleBuffer;

    // 생존 물방울 수
    int iAliveBubbleCount;

    // 플레이어 생명
    int iLife;

    // 유저 점수
    QWORD qwScore;

    // 게임 시작 여부
    BOOL bGameStart;
} GAMEINFO;

// 함수
BOOL Initalize(void);
BOOL CreateBubble(void);
void MoveBubble(void);
void DeleteBubbleUnderMouse(POINT *pstMouseXY);
void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID, POINT *pstMouseXY);

#endif