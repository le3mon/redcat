#include "../MINT64/02.Kernel64/Source/Types.h"
int kHexToString(QWORD qwValue, char *pcBuffer) {
    QWORD i;
    QWORD qwCurrentValue;

    if(qwValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    for(i = 0; qwValue > 0; i++) {
        qwCurrentValue = qwValue % 16;
        if(qwCurrentValue >= 10)
            pcBuffer[i] = 'A' + (qwCurrentValue - 10); // 10이상이면 10을 뺀 값을 'A'를 더해 'A'~'F' 문자로 변환
        else
            pcBuffer[i] = '0' + qwCurrentValue;
        
        qwValue = qwValue / 16;
    }
    pcBuffer[i] = '\0';

    kReverseString(pcBuffer);
    return i;
}