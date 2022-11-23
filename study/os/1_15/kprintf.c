void kPrintf(const char *pcFormatString, ...) {
    va_list ap;
    char vcBuffer[100];
    int iNextPrintOffset;

    va_start(ap, pcFormatString);
    kVSPrintf(vcBuffer, pcFormatString, ap);
    va_end(ap);

    iNextPrintOffset = kConsolePrintString(vcBuffer);

    // 커서 위치 업데이트
    kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}