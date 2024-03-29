#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Types.h"
#include "Synchronization.h"
#include "Queue.h"
#include "Keyboard.h"

// 비디오 메모리의 속성 값 설정
#define CONSOLE_BACKGROUND_BLACK        0x00
#define CONSOLE_BACKGROUND_BLUE         0x10
#define CONSOLE_BACKGROUND_GREEN        0x20
#define CONSOLE_BACKGROUND_CYAN         0x30
#define CONSOLE_BACKGROUND_RED          0x40
#define CONSOLE_BACKGROUND_MAGENTA      0x50
#define CONSOLE_BACKGROUND_BROWN        0x60
#define CONSOLE_BACKGROUND_WHITE        0x70
#define CONSOLE_BACKGROUND_BLINK        0x80
#define CONSOLE_FOREGROUND_DARKBLACK    0x00
#define CONSOLE_FOREGROUND_DARKBLUE     0x01
#define CONSOLE_FOREGROUND_DARKGREEN    0x02
#define CONSOLE_FOREGROUND_DARKCYAN     0x03
#define CONSOLE_FOREGROUND_DARKRED      0x04
#define CONSOLE_FOREGROUND_DARKMAGENTA  0x05
#define CONSOLE_FOREGROUND_DARKBROWN    0x06
#define CONSOLE_FOREGROUND_DARKWHITE    0x07
#define CONSOLE_FOREGROUND_BRIGHTBLACK  0x08
#define CONSOLE_FOREGROUND_BRIGHTBLUE   0x09
#define CONSOLE_FOREGROUND_BRIGHTGREEN  0x0A
#define CONSOLE_FOREGROUND_BRIGHTCYAN   0x0B
#define CONSOLE_FOREGROUND_BRIGHTRED    0x0C
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA 0x0D
#define CONSOLE_FOREGROUND_BRIGHTYELLOW 0x0E
#define CONSOLE_FOREGROUND_BRIGHTWHITE  0x0F
// 기본 문자 색상
#define CONSOLE_DEFAULTTEXTCOLOR ( CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN )

// 콘솔의 너비와 높이, 그리고 비디오 메모리의 시작 어드레스 설정
#define CONSOLE_WIDTH   80
#define CONSOLE_HEIGHT  25
#define CONSOLE_VIDEOMEMORYADDRESS  0XB8000

// 비디오 컨트롤러의 I/O 포트 어드레스와 레지스터
#define VGA_PORT_INDEX  0X3D4
#define VGA_PORT_DATA   0X3D5
#define VGA_INDEX_UPPERCURSOR   0X0E
#define VGA_INDEX_LOWERCURSOR   0X0F

// 그래픽 모드에서 사용하는 키 큐에 저장할 수 있는 최대 개수
#define CONSOLE_GUIKETQUEUE_MAXCOUNT    100

#pragma pack(push, 1)
typedef struct kConsoleManagerStruct {
    int iCurrentPrintOffset;

    // 출력할 화면 버퍼 어드레스
    CHARACTER *pstScreenBuffer;

    // 그래픽 모드에서 사용할 키 큐와 뮤텍스
    QUEUE stKeyQueueForGUI;
    MUTEX stLock;

    // 셸 태스크 종료 여부
    volatile BOOL bExit;
} CONSOLEMANAGER;
#pragma pack(pop)

void kInitializeConsole(int iX, int iY);
void kSetCursor(int iX, int iY);
void kGetCursor(int *piX, int *piY);
void kPrintf(const char *pcFormatString, ...);
int kConsolePrintString(const char *pcBuffer);
void kClearScreen(void);
BYTE kGetCh(void);
void kPrintStringXY(int iX, int iY, const char *pcString);

CONSOLEMANAGER *kGetConsoleManager(void);
BOOL kGetKeyFromGUIKeyQueue(KEYDATA *pstData);
BOOL kPutKeyToGUIKeyQueue(KEYDATA *pstData);
void kSetConsoleShellExitFlag(BOOL bFlag);

#endif