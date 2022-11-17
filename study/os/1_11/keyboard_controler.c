#include "../MINT64/01.Kernel32/Source/Types.h"

// 출력 버퍼에 수신된 데이터가 있는지 여부 반환
BOOL kIsOutputBufferFull(void) { 
    // 상태 레지스터(포트 0x64)에서 읽은 값에 출력 버퍼 상태 비트(비트 0)의 값이 1인지 확인
    // 1이라면 출력 버퍼에 키보드가 전송한 데이터가 존재하므로 TRUE 반환
    if(kInPortByte(0x64) & 0x01) 
        return TRUE;
    
    return FALSE;
}

// 입력 버퍼에 수신된 데이터가 있는지 여부 반환
BOOL kIsInputBufferFull(void) {
    // 상태 레지스터에 읽은 값에 입력 버퍼 상태 비트(비트 1)의 값이 1인지 확인
    // 
    if(kInPortByte(0x64) & 0x02)
        return TRUE;
    
    return FALSE;
}

BOOL kActivateKeyboard(void) {
    int i, j;

    // 컨트롤 레지스터(포트 0x64)에 키보드 활성화 커맨드(0xAE) 전달
    kOutPortByte(0x64, 0xAE);
    
    // 입력 버퍼가 빌 때까지 기다렸다 키보드에 활성화 커맨드 전송
    for(i = 0; i < 0xFFFF; i++) {
        // FALSE가 반환되었다면 입력 버퍼가 비었다는 것 이므로 반복문을 빠져나온다.
        if(kIsInputBufferFull() == FALSE) 
            break;
    }

    // 입력 버퍼(포트 0x60)으로 키보드 활성화(0xF4) 커맨드를 전달하여 키보드로 전송
    kOutPortByte(0x60, 0xF4);

    // ACK이 올때까지 대기
    // ACK이 오기 전에 키보드 출력 버퍼에 키 데이터가 저장될 수 있으므로 최대 100까지 수신하여 ACK 확인
    for(j = 0; j < 100; j++) {
        for (i = 0; i < 0xFFFF; i++) {
            // TRUE가 반환되었다면 출력 버퍼에 값이 있다는 것이므로 값을 확인하기 위해 반복문을 빠져나간다.
            if(kIsOutputBufferFull() == TRUE)
                break;
        }

        // 출력 버퍼에서 읽은 데이터가 ACK(0xFA)이면 성공
        if(kInPortByte(0x60) == 0xFA)
            return TRUE;
    }

    return FALSE;
}