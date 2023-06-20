#ifndef __MAIN_H__
#define __MAIN_H__

#include "../../UserLibrary/Source/Types.h"

// 매크로

// 게임판 너비 / 높이
#define BOARDWIDTH      8
#define BOARDHEIGHT     12


// 블록 하나 크기
#define BLOCKSIZE       32
// 움직이는 블록 수
#define BLOCKCOUNT      3
// 빈 블록 나타내는 값
#define EMPTYBLOCK      0
// 지울 블록 나타내는 값
#define ERASEBLOCK      0xFF
// 블록 종류
#define BLOCKKIND       5

// 게임 정보 영역 높이
#define INFORMATION_HEIGHT  20

// 윈도우 너비와 높이
#define WINDOW_WIDTH    (BOARDWIDTH * BLOCKSIZE)
#define WINDOW_HEIGHT   (WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT + BOARDHEIGHT * BLOCKSIZE)

// 구조체
// 게임 정보 저장하는 자료구조
typedef struct GameInfoStruct {
    // 블록 종류에 따른 색깔
    COLOR vstBlockColor[BLOCKKIND + 1];
    COLOR vstEdgeColor[BLOCKKIND + 1];

    // 현재 움직이는 블록 위치
    int iBlockX;
    int iBlockY;

    // 게임판에 고정된 블록의 상태를 관리하는 영역
    BYTE vvbBoard[BOARDHEIGHT][BOARDWIDTH];

    // 게임판에 고정된 블록 중에서 삭제해야 할 블록 관리하는 영역
    BYTE vvbEraseBlock[BOARDHEIGHT][BOARDWIDTH];

    // 현재 움직이는 블록의 구성을 저장하는 영역
    BYTE vbBlock[BLOCKCOUNT];

    // 게임 시작 여부
    BOOL bGameStart;

    // 유저 점수
    QWORD qwScore;

    // 게임 레벨
    QWORD qwLevel;
} GAMEINFO;

// 함수
void Initialize(void);
void CreateBlock(void);
BOOL IsMovePossible(int iBlockX, int iBlockY);
BOOL FreezeBlock(int iBlockX, int iBlockY);
void EraseAllContinuousBlockOnBoard(QWORD qwWindowID);
BOOL MarkContinuousVerticalBlockOnBoard(void);
BOOL MarkContinuousHorizonBlockOnBoard(void);
BOOL MarkContinuousDiagonalBlockInBoard(void);
void EraseMarkedBlock(void);
void CompactBlockOnBoard(void);
void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID);

#endif