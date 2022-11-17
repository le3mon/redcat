#include "../MINT64/01.Kernel32/Source/Types.h"

BOOL kChangeKeyboatdLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScroolLockOn) {
    int i, j;

    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE) // 입력 버퍼가 비어있는지 확인
            break;
    }

    kOutPortByte(0x60, 0xED); // 입력 버퍼로 LED 상태 변경 커맨드(0xED) 전송
    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE) // 입력 버퍼를 통해 키보드가 커맨드를 가져갔는지 확인
            break;
    }

    for(j = 0; j < 100; j++) {
        for(i = 0; i < 0xFFFF; i++) {
            if(kIsOutputBufferFull() == TRUE)
                break;
        }

        if(kInPortByte(0x60) == 0xFA) // 정상적으로 커맨드가 처리되었는지 ACK(0xFA) 코드를 읽어 확인
            break;
    }

    if(j >= 100) // j 값이 100이상이면 반복문을 그대로 빠져나온 것 이므로 FALSE 반환
        return FALSE;
    
    // LED 변경 값을 키보드로 전송
    kOutPortByte(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | bScroolLockOn);
    for(i = 0; i < 0xFFFF; i++) {
        if(kIsInputBufferFull() == FALSE) // 이전과 같이 키보드가 커맨드를 가져갔는지 확인
            break;
    }

    for(j = 0; j < 100; j++) {
        for(i = 0; i < 0xFFFF; i++) {
            if(kIsOutputBufferFull() == TRUE)
                break;
        }

        if(kInPortByte(0x60) == 0xFA) // 정상적으로 처리되었는지 확인
            break;
    }
    
    if(j >= 100) // 실패 시 FALSE 반환
        return FALSE;

    return TRUE; // 성공 시 TRUE 반환
}