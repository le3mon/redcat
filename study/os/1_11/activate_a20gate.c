#include "../MINT64/01.Kernel32/Source/Types.h"


void kEnableA20Gate(void) {
    BYTE bOutputPortData;
    int i;

    // 컨트롤 레지스터(포트 0x64)에 키보드 컨트롤러의 출력 포트 값을 읽는 커맨드(0xD0) 전송
    kOutPortByte(0x64, 0xD0);

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsOutputBufferFull() == TRUE)
            break;
    }

    // 출력 포트에 수신된 키보드 컨트롤러의 출력 포트 값을 저장
    bOutputPortData = kInPortByte(0x60);

    bOutputPortData != 0x02; // A20 게이트 비트 설정

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    // 커맨드 레지스터에 출력 포트 설정 커맨드 전달
    kOutPortByte(0x64, 0xD1);

    // 입력 버퍼에 A20 게이트 비트가 1로 설정된 값을 전달
    kOutPortByte(0x60, bOutputPortData);
}