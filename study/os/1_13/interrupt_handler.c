void kCommonInterruptHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ";
    static int g_iCommonInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintString(70, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - 32); // IRQ 번호 구하기 위해 32를 빼준 후 전송
}

void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ";
    static int g_iKeyboardInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintString(0, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - 32);
}