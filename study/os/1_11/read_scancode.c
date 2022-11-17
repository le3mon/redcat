#include "../MINT64/01.Kernel32/Source/Types.h"

// 출력 버퍼에 수신된 데이터가 있는지 여부 반환
BOOL kIsOutputBufferFull(void) { 
    // 상태 레지스터(포트 0x64)에서 읽은 값에 출력 버퍼 상태 비트(비트 0)의 값이 1인지 확인
    // 1이라면 출력 버퍼에 키보드가 전송한 데이터가 존재하므로 TRUE 반환
    if(kInPortByte(0x64) & 0x01) 
        return TRUE;
    
    return FALSE;
}

BYTE kGetKeyboardScanCode(void) {
    while(kIsOutputBufferFull() == FALSE) {
        ;
    }
    return kInPortByte(0x60); // 출력 버퍼에서 키 값을 읽어 반환
}