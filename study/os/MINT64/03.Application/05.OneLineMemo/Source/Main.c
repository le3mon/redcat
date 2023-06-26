#include "MINTOSLibrary.h"
#include "Main.h"
#include "HangulInput.h"

// c��� ��Ʈ�� ����Ʈ
int Main(char *pcArgument) {
    QWORD qwWindowID;
    int iX, iY, iWidth, iHeight;
    EVENT stReceivedEvent;
    KEYEVENT *pstKeyEvent;
    WINDOWEVENT *pstWindowEvent;
    RECT stScreenArea;
    BUFFERMANAGER stBufferManager;
    BOOL bHangulMode;

    // �׷��� ��� �Ǵ�
    if(IsGraphicMode() == FALSE) {
        printf("This task can run only gui mode~\n");
        return -1;
    }

    // ������ ȭ�� ����� ���� ���� ũ�⸦ 60���� 40�ȼ��� ����
    GetScreenArea(&stScreenArea);
    iWidth = MAXOUTPUTLENGTH * FONT_ENGLISHWIDTH + 5;
    iHeight = 40;
    iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
    iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
    qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "�� �� �޸���(��/�� ��ȯ�� Alt Ű)");

    // ���� ������ �ʱ�ȭ�ϰ� ���� �Է� ���� ����
    memset(&stBufferManager, 0, sizeof(stBufferManager));
    bHangulMode = FALSE;

    // Ŀ�� �޸� �Է� ���� ���ʿ� ���η� ��� ����ϰ� �����츦 �ٽ� ǥ��
    DrawRect(qwWindowID, 3, 4 + WINDOW_TITLEBAR_HEIGHT, 5, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT,
        RGB(0, 250, 0), TRUE);
    ShowWindow(qwWindowID, TRUE);

    // gui �½�ũ ���� ����
    while(1) {
        // �̺�Ʈ ť���� �̺�Ʈ ����
        if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE) {
            Sleep(10);
            continue;
        }

        // ���ŵ� �̺�Ʈ Ÿ�Կ� ���� ó��
        switch(stReceivedEvent.qwType) {
        case EVENT_KEY_DOWN:
            // Ű �̺�Ʈ ����
            pstKeyEvent = &(stReceivedEvent.stKeyEvent);

            // �Էµ� Ű�� �ѱ� �����ϰų� ���� ���
            switch(pstKeyEvent->bASCIICode) {
            case KEY_LALT:
                // �ѱ� �Է� ��� �߿� alt Ű�� �������� �ѱ� ���� ����
                if(bHangulMode == TRUE) {
                    // Ű �Է� ���� �ʱ�ȭ
                    stBufferManager.iInputBufferLength = 0;
                    if((stBufferManager.vcOutputBufferForProcessing[0] != '\0') &&
                        (stBufferManager.iOutputBufferLength + 2 < MAXOUTPUTLENGTH)) {
                        // ���� ���� �ѱ��� ������ ȭ�鿡 ����ϴ� ���۷� ����
                        memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                            stBufferManager.vcOutputBufferForProcessing, 2);
                        stBufferManager.iOutputBufferLength += 2;

                        // ���� ���� �ѱ��� �����ϴ� ���� �ʱ�ȭ
                        stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                    }
                }
                // ���� �Է� ��� �߿� Alt ű ���������� �ѱ� ���տ� ���� �ʱ�ȭ
                else {
                    stBufferManager.iInputBufferLength = 0;
                    stBufferManager.vcOutputBufferForComplete[0] = '\0';
                    stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                }
                bHangulMode = TRUE - bHangulMode;
                break;
            
            // �齺���̽� Ű ó��
            case KEY_BACKSPACE:
                // �ѱ��� �����ϴ� ���̸� �Է� ���� ������ �����ϰ� ���� Ű �Է� ������ �������� �ѱ� ����
                if((bHangulMode == TRUE) && (stBufferManager.iInputBufferLength > 0)) {
                    // Ű �Է� ������ ������ �ϳ� �����ϰ� �ѱ� �ٽ� ����
                    stBufferManager.iInputBufferLength--;
                    ComposeHangul(stBufferManager.vcInputBuffer,
                        &stBufferManager.iInputBufferLength,
                        stBufferManager.vcOutputBufferForProcessing,
                        stBufferManager.vcOutputBufferForComplete);
                }
                else {
                    if(stBufferManager.iOutputBufferLength > 0) {
                        // ȭ�� ��� ���ۿ� ��� �ִ� ������ 2����Ʈ �̻��̰� ���ۿ�
                        // ����� ���� �ֻ��� ��Ʈ�� ���� ������ �ѱ۷� �����ϰ� ������ 2����Ʈ ��� ����
                        if((stBufferManager.iOutputBufferLength >= 2) &&
                            (stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength - 1] & 0x80)) {
                            stBufferManager.iOutputBufferLength -= 2;
                            memset(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, 0, 2);
                        }
                        // �ѱ��� �ƴϸ� ������ 1����Ʈ ����
                        else {
                            stBufferManager.iOutputBufferLength--;
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = '\0';
                        }
                    }
                }
                break;
            default:
                // Ư�� Ű�� ��� ���� 
                if(pstKeyEvent->bASCIICode & 0x80) {
                    break;
                }
                
                // �ѱ� ���� ���̸� �ѱ� ���� ó��
                if((bHangulMode == TRUE) && (stBufferManager.iOutputBufferLength + 2 <= MAXOUTPUTLENGTH)) {
                    // ��/������ ����Ʈ�� ���յǴ� ��츦 ����Ͽ� �������̳�
                    // �ָ����� ������ �������� �ҹ��ڷ� ��ȯ
                    ConvertJaumMoumToLowerCharactor(&pstKeyEvent->bASCIICode);

                    // �Է� ���ۿ� Ű �Է� ���� ä��� �������� ���� ����
                    stBufferManager.vcInputBuffer[stBufferManager.iInputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iInputBufferLength++;

                    // �ѱ� ���տ� �ʿ��� ���۸� �ѰܢZ ���ѱ� ����
                    if(ComposeHangul(stBufferManager.vcInputBuffer,
                        &stBufferManager.iInputBufferLength,
                        stBufferManager.vcOutputBufferForProcessing,
                        stBufferManager.vcOutputBufferForComplete) == TRUE) {
                        // ������ �Ϸ�� ���ۿ� ���� �ִ°� Ȯ���Ͽ� ������ ȭ�鿡 ����� ���۷� ����
                        if(stBufferManager.vcOutputBufferForComplete[0] != '\0') {
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                                stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;

                            // ���յ� �ѱ��� ������ �ڿ� ���� ���� ���� �ѱ� ����� ������ ���ٸ�
                            // Ű �Է� ���ۿ� ���� ���� �ѱ� ���� ��� �ʱ�ȭ
                            if(stBufferManager.iOutputBufferLength + 2 > MAXOUTPUTLENGTH) {
                                stBufferManager.iInputBufferLength = 0;
                                stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                            }
                        }
                    }
                    // ���տ� �����ϸ� �Է� ���ۿ� ���������� �Էµ� ���� �ѱ� �� ������ �ƴϹǷ�
                    // �ѱ� ������ �Ϸ�� ������ ���� �Է� ���ۿ� �ִ� ���� ��� ��� ���۷� ����
                    else {
                        // ������ �Ϸ�� ���ۿ� ���� �ִ°� Ȯ���Ͽ� ������ ȭ�� ��� ���� ����
                        if(stBufferManager.vcOutputBufferForComplete[0] != '\0') {
                            // �ϼ��� �ѱ� ��� ���۷� ����
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
                                stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;
                        }

                        // ������ ȭ�鿡 ����ϴ� ������ ������ ����ϸ� Ű �Է� ���ۿ� ����������
                        // �Էµ� �ѱ� ��/�� �ƴ� �� ����
                        if(stBufferManager.iOutputBufferLength < MAXOUTPUTLENGTH) {
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = stBufferManager.vcInputBuffer[0];
                            stBufferManager.iOutputBufferLength++;
                        }

                        // Ű �Է� ���� ���
                        stBufferManager.iInputBufferLength = 0;
                    }
                }
                // �ѱ� �Է� ��尡 �ƴ� ���
                else if((bHangulMode == FALSE) && (stBufferManager.iOutputBufferLength + 1 <= MAXOUTPUTLENGTH)) {
                    // Ű �Է��� �״�� ������ ȭ�鿡 ����ϴ� ���۷� ����
                    stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iOutputBufferLength++;
                }
                break;
            }

            // ȭ�� ��� ���ۿ� �ִ� ���ڿ� ���� ���
            DrawText(qwWindowID, 2, WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBuffer, MAXOUTPUTLENGTH);

            // ���� �������� �ѱ��� �ִٸ� ȭ�� ��� ������ ������ ��µ� ���� ��ġ�� ���� ���� �ѱ� ���
            if(stBufferManager.vcOutputBufferForProcessing[0] != '\0') {
                DrawText(qwWindowID, 2 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, WINDOW_TITLEBAR_HEIGHT + 4,
                    RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBufferForProcessing, 2);
            }

            // Ŀ���� ���η� ��� ���
            DrawRect(qwWindowID, 3 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 4 + WINDOW_TITLEBAR_HEIGHT,
                5 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT, RGB(0, 250, 0), TRUE);

            ShowWindow(qwWindowID, TRUE);
            break;
        
        case EVENT_WINDOW_CLOSE:
            DeleteWindow(qwWindowID);
            return 0;
            break;
        
        default:
            break;
        }
    }

    return 0;
}

// �ѱ� ������ ���� �������� �������� �ָ����� ������ �������� ��� �ҹ��ڷ� ��ȯ
void ConvertJaumMoumToLowerCharactor(BYTE *pbInput) {
    if((*pbInput < 'A') || (*pbInput > 'Z')) {
        return;
    }

    // ������ �Ǵ� �ָ��� ���� �Ǻ�
    switch(*pbInput) {
        case 'Q':
        case 'W':
        case 'E':
        case 'R':
        case 'T':
        case 'O':
        case 'P':
            return;
            break;
    }

    *pbInput = TOLOWER(*pbInput);
}