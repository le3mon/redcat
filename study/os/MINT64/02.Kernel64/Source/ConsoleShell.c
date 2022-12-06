#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"

SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decimal/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
    {"settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"date", "Show Date And Time", kShowDateAndTime},
    {"createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask},
};

void kStartConsoleShell(void) {
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferindex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    // 프롬프트 출력
    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while(1) {
        // 키 수신 대기
        bKey = kGetCh();

        // Backspace 키 처리
        if(bKey == KEY_BACKSPACE) {
            if(iCommandBufferindex > 0) {
                // 현재 커서 위치를 얻어서 한 문자 앞으로 이동 후 공백으로 출력
                // 커맨드 버퍼에서 마지막 문자 삭제
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX - 1, iCursorY, " ");
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferindex--;
            }
        }

        // 엔터 키 처리
        else if(bKey == KEY_ENTER) {
            kPrintf("\n");
            if(iCommandBufferindex > 0) {
                vcCommandBuffer[iCommandBufferindex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferindex = 0;
        }

        // shift, caps lock, num lock, scroll lock 무시
        else if((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) 
        || (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLLOCK)) {
            ;
        }
        else {
            // 탭은 공백으로 전환
            if(bKey == KEY_TAB)
                bKey = ' ';
            
            // 버퍼에 공간이 남아 있을 때만 가능
            if(iCommandBufferindex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT) {
                vcCommandBuffer[iCommandBufferindex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

// 커맨드 버퍼에 있는 커맨드를 비교하여 해당 커맨드를 처리하는 함수 실행
void kExecuteCommand(const char *pcCommandBuffer) {
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    // 공백으로 구분된 커맨드를 추출
    iCommandBufferLength = kStrLen(pcCommandBuffer);
    for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++) {
        if(pcCommandBuffer[iSpaceIndex] == ' ')
            break;
    }

    // 커맨드 테이블을 검사하아여 같은 이름의 커맨드 확인
    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for(i = 0; i < iCount; i++) {
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        // 커맨드의 길이와 내용이 일치하는지 검사
        if((iCommandLength == iSpaceIndex) && 
        (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)) {
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }
    if(i >= iCount)
        kPrintf("'%s' is not found \n", pcCommandBuffer);
}

// 파라미터 자료구조를 초기화
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter) {
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

// 공백으로 구분된 파라미터의 내용과 길이를 반환
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter) {
    int i;
    int iLength;

    // 더 이상 파라미터가 없으면 나감
    if(pstList->iLength <= pstList->iCurrentPosition)
        return 0;
    
    // 버퍼의 길이만큼 이동하면서 공백을 검색
    for(i = pstList->iCurrentPosition; i < pstList->iLength; i++) {
        if(pstList->pcBuffer[i] == ' ')
            break;
    }

    // 파라미터를 복사하고 길이를 반환
    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    // 파라미터 위치 업데이트
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}

// 셸 도움말 출력
void kHelp(const char *pcParameterBuffer) {
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kPrintf( "=========================================================\n" );
    kPrintf( "                    MINT64 Shell Help                    \n" );
    kPrintf( "=========================================================\n" );

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    // 가장 긴 커맨드의 길이를 계산
    for(i = 0; i < iCount; i++) {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength)
            iMaxCommandLength = iLength;
    }

    // 도움말 출력
    for(i = 0; i < iCount; i++) {
        kPrintf("%s", gs_vstCommandTable[i].pcCommand);
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);
    }
}

// 화면을 지움
void kCls(const char *pcParameterBuffer) {
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
void kStringToDecimalHexTest(const char *pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;

    kInitializeParameter(&stList, pcParameterBuffer);

    while(1) {
        // 다음 파라미터를 구함, 파라미터의 길이가 0이면 파라미터가 없는 것 이므로 종료
        iLength = kGetNextParameter(&stList, vcParameter);
        if(iLength == 0)
            break;
        
        // 파라미터에 대한 정보를 출력하고 16진수인지 10진수인 판단하여 변환 후 결과를 printf로 출력

        kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

        // 0x로 시작하면 16진수, 그 외에는 10진수로 판단
        if(kMemCmp(vcParameter, "0x", 2) == 0) {
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX Value = %q\n", lValue);
        }
        else {
            lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }
        iCount++;
    }
}

void kShutdown(const char *pcParameterBuffer) {
    kPrintf("System Shutdown Start...\n");

    // 키보드 컨트롤러를 통해 PC를 재시작
    kPrintf("Pass Any key To Reboot PC...");
    kGetCh();
    kReboot();
}

void kSetTimer(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    kInitializeParameter(&stList, pcParameterBuffer);

    // ms 추출
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    lValue = kAToI(vcParameter, 10);

    // periodic 추출
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    bPeriodic = kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

void kWaitUsingPIT(const char *pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)wait 100(ms)\n");
        return;
    }
    lMillisecond = kAToI(pcParameterBuffer, 10);
    kPrintf("%d ms Sleep Start...\n", lMillisecond);

    kDisableInterrupt();
    for(i = 0; i < lMillisecond / 30; i++)
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();
    kPrintf("%d ms Sleep Complete\n", lMillisecond);

    kInitializePIT(MSTOCOUNT(1), TRUE);
}

void kReadTimeStampCounter(const char *pcParameterBuffer) {
    QWORD qwTSC;
    qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

void kMeasureProcessorSpeed(const char *pcParameterBuffer) {
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;

    kPrintf("Now Measuring.");

    kDisableInterrupt();
    for(i = 0; i < 200; i++) {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;
        kPrintf(".");
    }

    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

void kShowDateAndTime(const char *pcParameterBuffer) {
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("Date : %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("Time : %d:%d:%d\n", bHour, bMinute, bSecond);
}

static TCB gs_vstTask[2] = {0, };
static QWORD gs_vstStack[1024] = {0, };

void kTestTask(void) {
    int i = 0;
    while (1) {
        // 메시지 출력 후 키 입력 대기
        kPrintf("[%d] This message is from kTestTask. Press any key to switch kConsoleShell~!!\n", i++);
        kGetCh();

        // 위에서 키가 입력되면 태스크 전환
        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}

void kTestTask1(void) {
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;

    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    while (1) {
        switch (i) {
        case 0:
            iX++;
            if(iX >= (CONSOLE_WIDTH - iMargin))
                i = 1;
            break;
        
        case 1:
            iY++;
            if(iY >= (CONSOLE_HEIGHT - iMargin))
                i = 2;
            break;
        
        case 2:
            iX--;
            if(iX < iMargin)
                i = 3;
            break;
        
        case 3:
            iY--;
            if(iY < iMargin)
                i = 0;
            break;
        }

        pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
        bData++;

        kSchedule();
    }
}

void kTestTask2(void) {
    int i = 0, iOffset;
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while (1) {
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        kSchedule();
    }
}

void kCreateTestTask(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    switch (kAToI(vcType, 10)) {
    case 1:
        for(i = 0; i < kAToI(vcCount, 10); i++) {
            if(kCreateTask(0, (QWORD)kTestTask1) == NULL)
                break;
        }
        kPrintf("Task1 %d Created\n", i);
        break;
    
    case 2:
    default:
        for(i = 0; i < kAToI(vcCount, 10); i++) {
            if(kCreateTask(0, (QWORD)kTestTask2) == NULL)
                break;
        }
        kPrintf("Task2 %d Created\n", i);
        break;
    }    
}
