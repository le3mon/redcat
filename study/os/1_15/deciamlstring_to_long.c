long kDecimalStringToLong(const char *pcBuffer) {
    long lValue = 0;
    int i;

    if(pcBuffer[0] == '-')
        i = 1;
    else
        i = 0;
    
    for(; pcBuffer[i] != '\0'; i++) {
        lValue *= 10;
        lValue += pcBuffer[i] - '0';
    }

    if(pcBuffer[0] == '-')
        lValue = -lValue;
    
    return lValue;
}