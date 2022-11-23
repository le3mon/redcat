#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"
#define VGA_PORT_INDEX 0x3D4
#define VGA_PORT_DATA 0x3D5
#define VGA_INDEX_UPPERCURSOR 0x0E
#define VGA_INDEX_LOWERCURSOR 0x0F

#define CONSOLE_WIDTH   80

void kSetCursor(int iX, int iY) {
    int iLinearValue;

    // 커서 위치 계산
    iLinearValue = iY * CONSOLE_WIDTH + iX;

    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
    // CRTC 컨트롤 데이터 레지스터에 커서의 상위 바이트를 출력
    kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
    // CRTC 컨트롤 데이터 레지스터에 커서의 하위 바이트를 출력
    kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

void kGetCursor(int *piX, int *piY) {
    // 지정된 위치를 콘솔 화면의 너비로 나눈 나머지로 X 좌표를 구할 수 있으며,
    // 화면 너비로 나누면 Y 좌표를 구할 수 있음
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}