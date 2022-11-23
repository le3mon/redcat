void kReverseString(char *pcBuffer) {
    int iLength;
    int i;
    char cTemp;

    iLength = kStrLen(pcBuffer);
    for(i = 0; i < iLength / 2; i++) {
        cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength - 1 - i];
        pcBuffer[iLength - 1 - i] = cTemp;
    }
}
int kDecimalToString(long lValue, char *pcBuffer) {
    long i;

    // 0이 들어오면 바로 처리
    if(lValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    // 음수면 - 추가하고 양수로 변환
    if(lValue < 0) {
        i = 1;
        pcBuffer[0] = '-';
        lValue = -lValue;
    }
    else {
        i = 0;
    }

    for(; lValue > 0; i++) {
        pcBuffer[i] = '0' + lValue % 10; // 1의 자릿수를 추출하여 문자로 변환
        lValue = lValue / 10; //정수를 10으로 나누어 왼쪽에서 오른쪽으로 이동
    }
    pcBuffer[i] = '\0';

    if(pcBuffer[0] == '-')
        kReverseString(&(pcBuffer[1])); // 음수는 숫자 부분만 뒤집어야 하므로 '-'를 제외한 나머지 전달
    else
        kReverseString(pcBuffer);
    
    return i;
}