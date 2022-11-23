long kAToI(const char *pcBuffer, int iRadix) {
    long lReturn;

    switch(iRadix) {
    case 16:
        lReturn = kHexStringToQword(pcBuffer);
        break;
    
    case 10:
    default:
        lReturn = kDecimalStringToLong(pcBuffer);
        break;
    }
    return lReturn;
}