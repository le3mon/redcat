int kConsolePrintString(const char *pcBuffer) {
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    int i, j;
    int iLength;
    int iPrintOffset;

    // 문자열 출력할 위치를 저장
    iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

    iLength = kStrLen(pcBuffer);
    for(i = 0; i < iLength; i++) {
        if(pcBuffer[i] == '\n') {
            // 출력할 위치를 80의 배수 칼럼으로 옮김
            // 현재 라인의 남은 문자열의 수만큼 더해서 다음 라인으로 위치
            iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
        }
        else if(pcBuffer[i] == '\t') {
            // 출력할 위치를 8의 배수 칼럼으로 옮김
            iPrintOffset += (8 - (iPrintOffset % 8));
        }
        else {
            pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
            pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset++;
        }

        // 출력할 위치가 화면의 최댓값(80*25)를 벗어나면 스크롤 처리
        if(iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH)) {
            kMemCpy(CONSOLE_VIDEOMEMORYADDRESS, CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER), (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACHER));

            for(j = (CONSOLE_HEIGHT - 1) * (CONSOLE_WIDTH); j < (CONSOLE * CONSOLE_WIDTH); j++) {
                pstScreen[iPrintOffset].bCharactor = ' ';
                pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            }

            iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
        }
    }
    return iPrintOffset;
}