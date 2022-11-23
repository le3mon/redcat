int kIToA(long lValue, char *pcBuffer, int iRadix) {
    int iReturn;
    
    switch(iRadix) {
        case 16:
            // 정수 값을 16진수 문자열로 변환하는 함수
            iReturn = kHexToString(lValue, pcBuffer);
            break;
        
        case 10:
        default:
            // 정수 값을 10진수 문자열로 변환하는 함수
            iReturn = kDecimalToString(lValue, pcBuffer);
            break;
    }
    return iReturn;
}   