#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "MultiProcessor.h"
#include "IOAPIC.h"
#include "PIC.h"
#include "InterruptHandler.h"

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
    {"changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)", kChangeTaskPriority},
    {"tasklist", "Show Task List", kShowTaskList},
    {"killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask},
    {"cpuload", "Show Processor Load", kCPULoad},
    {"testmutex", "Test Mutex Function", kTestMutex},
    {"testthread", "Test Thread And Process Function", kTestThread},
    {"showmatrix", "Show Matrix Screen", kShowMatrix},
    {"testpie", "Test PIE Calculation", kTestPIE},
    {"dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation},
    {"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
    {"testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
    {"hddinfo", "Show HDD Information", kShowHDDInformation},
    {"readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", kReadSector},
    {"writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", kWriteSector},
    {"mounthdd", "Mount HDD", kMountHDD},
    {"formathdd", "Format HDD", kFormatHDD},
    {"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
    {"createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory},
    {"deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory},
    {"dir", "Show Directory", kShowRootDirectory},
    {"writefile", "Write Data to File, ex) writefile a.txt", kWriteDataToFile},
    {"readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile},
    {"testfileio", "Test File I/O Function", kTestFileIO},
    {"testperformance", "Test File Read/WritePerformance", kTestPerformance},
    {"flush", "Flush File System Cache", kFlushCache},
    {"download", "Download Data From Serial, ex) download a.txt", kDownloadFile},
    {"showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable},
    {"startap", "Start Application Processor", kStartApplicationProcessor},
    {"startsymmetricio", "Start Symmetric I/O Mode", kStartSymmetricIOMode},
    {"showirqintinmap", "Show IRQ->INITIN Mapping Table", kShowIRQINTINMappingTable},
    {"showintproccount", "Show Interrupt Processing Count", kShowInterruptProcessingCount},
    {"startintloadbal", "Start Interrupt Load Balancing", kStartInterruptLoadBalancing},
    {"starttaskloadbal", "Start Task Load Balancing", kStartTaskLoadBalancing},
    {"changeaffinity", "Change Task Affinity, ex)changeaffinity 1(ID) 0xFF(Affinity)", kChangeTaskAffinity},
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
static void kHelp(const char *pcParameterBuffer) {
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

        // 목록이 많을 경우 나눠서 보여줌
        if((i != 0) && ((i % 20) == 0)) {
            kPrintf("Press any key to continue... ('q' is exit : ");
            if(kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }
    }
}

// 화면을 지움
static void kCls(const char *pcParameterBuffer) {
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
static void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
static void kStringToDecimalHexTest(const char *pcParameterBuffer) {
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

static void kShutdown(const char *pcParameterBuffer) {
    kPrintf("System Shutdown Start...\n");

    // 파일 시스템 캐시에 들어 있는 내용을 하드 디스크로 옮김
    kPrintf("Cache Flush... ");
    if(kFlushFileSystemCache() == TRUE) {
        kPrintf("[Pass]\n");
    }
    else {
        kPrintf("[Fail]\n");
    }

    // 키보드 컨트롤러를 통해 PC를 재시작
    kPrintf("Pass Any key To Reboot PC...");
    kGetCh();
    kReboot();
}

static void kSetTimer(const char *pcParameterBuffer) {
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

static void kWaitUsingPIT(const char *pcParameterBuffer) {
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

static void kReadTimeStampCounter(const char *pcParameterBuffer) {
    QWORD qwTSC;
    qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

static void kMeasureProcessorSpeed(const char *pcParameterBuffer) {
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

static void kShowDateAndTime(const char *pcParameterBuffer) {
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

static void kTestTask(void) {
    int i = 0;
    while (1) {
        // 메시지 출력 후 키 입력 대기
        kPrintf("[%d] This message is from kTestTask. Press any key to switch kConsoleShell~!!\n", i++);
        kGetCh();

        // 위에서 키가 입력되면 태스크 전환
        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}

static void kTestTask1(void) {
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin, j;
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;

    pstRunningTask = kGetRunningTask(kGetAPICID());
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    for(j = 0; j < 2000; j++) {
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

        // kSchedule();
    }

    kExitTask();
}

static void kTestTask2(void) {
    int i = 0, iOffset;
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    pstRunningTask = kGetRunningTask(kGetAPICID());
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while (1) {
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        // kSchedule();
    }
}

static void kCreateTestTask(const char *pcParameterBuffer) {
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
            if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1, TASK_LOADBALANCINGID) == NULL)
                break;
        }
        kPrintf("Task1 %d Created\n", i);
        break;
    
    case 2:
    default:
        for(i = 0; i < kAToI(vcCount, 10); i++) {
            if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2, TASK_LOADBALANCINGID) == NULL)
                break;
        }
        kPrintf("Task2 %d Created\n", i);
        break;
    }    
}

static void kChangeTaskPriority(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcID[30];
    char vcPriority[30];
    QWORD qwID;
    BYTE bPriority;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcPriority);

    if(kMemCmp(vcID, "0x", 2) == 0)
        qwID = kAToI(vcID + 2, 16);
    else
        qwID = kAToI(vcID, 10);
    
    bPriority = kAToI(vcPriority, 10);

    kPrintf("Change Task Priority ID [0x%q] Priority[%d]", qwID, bPriority);
    if(kChangePriority(qwID, bPriority) == TRUE)
        kPrintf("Success\n");
    else
        kPrintf("Fail\n");
}

static void kShowTaskList(const char *pcParameterBuffer) {
    int i;
    TCB *pstTCB;
    int iCount = 0;
    int iTotalTaskCount = 0;
    char vcBuffer[20];
    int iRemainLength;
    int iProcessorCount;

    // 코어 수만큼 루프 돌면서 각 스케줄러에 있는 태스크의 수를 더함
    iProcessorCount = kGetProcessorCount();

    for(i = 0; i < iProcessorCount; i++) {
        iTotalTaskCount += kGetTaskCount(i);
    }

    kPrintf("=========== Task Total Count [%d] ===========\n", iTotalTaskCount);
    
    // 코어가 두 개 이상이면 각 스케줄러별로 개수 출력
    if(iProcessorCount > 1) {
        // 각 스케줄러별로 태스크의 개수 출력
        for(i = 0; i < iProcessorCount; i++) {
            if((i != 0) && ((i % 4) == 0)) {
                kPrintf("\n");
            }
            kSPrintf(vcBuffer, "Core %d : %d", i, kGetTaskCount(i));
            kPrintf(vcBuffer);

            // 출력하고 남은 공간을 모두 스페이스바로 채움
            iRemainLength = 19 - kStrLen(vcBuffer);
            kMemSet(vcBuffer, ' ', iRemainLength);
            vcBuffer[iRemainLength] = '\0';
            kPrintf(vcBuffer);
        }

        kPrintf("\nPress any key to continue... ('q' is exit) :");
        if(kGetCh() == 'q') {
            kPrintf("\n");
            return;
        }
        kPrintf("\n\n");
    }

    for(i = 0; i < TASK_MAXCOUNT; i++) {
        pstTCB = kGetTCBInTCBPool(i);
        if((pstTCB->stLink.qwID >> 32) != 0) {
            if((iCount != 0) && ((iCount % 6) == 0)) {
                kPrintf("Press any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q') {
                    kPrintf("\n");
                    break;
                }
                kPrintf("\n");
            }
            kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n",
                1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
                pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)));
            kPrintf("   Core ID[0x%X] CPU Affinity[0x%X]\n", pstTCB->bAPICID, pstTCB->bAffinity);
            kPrintf("   Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
                pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
        }
    }
}

static void kKillTask(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcID[30];
    QWORD qwID;
    TCB *pstTCB;
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);

    if(kMemCmp(vcID, "0x", 2) == 0)
        qwID = kAToI(vcID + 2, 16);
    else
        qwID = kAToI(vcID, 10);

    if(qwID != 0xFFFFFFFF) {
        pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
        qwID = pstTCB->stLink.qwID;

        // 시스템 테스크는 제외
        if((qwID >> 32) != 0 && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
            kPrintf("Kill Task ID [0x%q]", qwID);
            if(kEndTask(qwID) == TRUE) {
            kPrintf("Success\n");
            }
            else {
            kPrintf("Fail\n");
            }
        }
        else {
            kPrintf("Task does not exist or task is system task\n");
        }
    }
    else {
        for(i = 0; i < TASK_MAXCOUNT; i++) {
            pstTCB = kGetTCBInTCBPool(i);
            qwID = pstTCB->stLink.qwID;

            // 시스템 테스크 제외
            if(((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)) {
                kPrintf("Kill Task ID [0x%q]", qwID);
                if(kEndTask(qwID) == TRUE) {
                    kPrintf("Success\n");
                }
                else {
                    kPrintf("Fail\n");
                }
            }
        }
    }
}

static void kCPULoad(const char *pcParameterBuffer) {
    int i;
    char vcBuffer[50];
    int iRemainLength;

    kPrintf("================= Processor Load =================\n");

    // 각 코어별로 부하 출력
    for(i = 0; i < kGetProcessorCount(); i++) {
        if((i != 0) && ((i % 4) == 0)) {
            kPrintf("\n");
        }
        kSPrintf(vcBuffer, "Core %d : %d", i, kGetProcessorLoad(i));
        kPrintf("%s", vcBuffer);

        // 출력하고 남은 공간을 모두 스페이스바로 채움
        iRemainLength = 19 - kStrLen(vcBuffer);
        kMemSet(vcBuffer, ' ', iRemainLength);
        vcBuffer[iRemainLength] = '\0';
        kPrintf(vcBuffer);
    }
    kPrintf("\n");
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(const char *pcParameterBuffer) {
    int i, j;
    QWORD qwTickCount;

    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 50) {
        kSchedule();
    }

    for(i = 0; i < 5; i++) {
        kLock(&(gs_stMutex));
        kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask(kGetAPICID())->stLink.qwID, gs_qwAdder);
        gs_qwAdder += 1;
        kUnlock(&(gs_stMutex));

        for(j = 0; j < 30000; j++) ;
    }

    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 1000) {
        kSchedule();
    }

    kExitTask();
}

static void kTestMutex(const char *pcParameterBuffer) {
    int i;

    gs_qwAdder = 1;

    kInitializeMutex(&gs_stMutex);

    for(i = 0; i < 3; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask, kGetAPICID());
    }
    kPrintf("Wait Util %d Task End...\n", i);
    kGetCh();
}

static void kCreateThreadTask(void) {
    int i;

    for(i = 0; i < 3; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2, TASK_LOADBALANCINGID);
    }

    while(1) {
        kSleep(1);
    }
}

static void kTestThread(const char *pcParameterBuffer) {
    TCB *pstProcess;

    pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000,
        (QWORD)kCreateThreadTask, TASK_LOADBALANCINGID);

    if(pstProcess != NULL) {
        kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);
    }
    else {
        kPrintf("Process Create Fail\n");
    }
}

static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom(void) {
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

static void kDropCharactorThread(void) {
    int iX, iY;
    int i;
    char vcText[2] = {0, };

    iX = kRandom() % CONSOLE_WIDTH;

    while(1) {
        kSleep(kRandom() % 20);

        if((kRandom() % 20) < 15) {
            vcText[0] = ' ';
            for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        }
        else {
            for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
                vcText[0] = i + kRandom();
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
        }
    }
}

static void kMatrixProcess(void) {
    int i;

    for(i = 0; i < 300; i++) {
        if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kDropCharactorThread, TASK_LOADBALANCINGID) == NULL) {
            break;
        }

        kSleep(kRandom() % 5 + 5);
    }
    kPrintf("%d Thread is created\n", i);

    kGetCh();
}

static void kShowMatrix(const char *pcParameterBuffer) {
    TCB *pstProcess;

    pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xE00000, 0xE00000,
        (QWORD)kMatrixProcess, TASK_LOADBALANCINGID);

    if(pstProcess != NULL) {
        kPrintf("Matrix Process [0x%Q] Create Success\n");

        while((pstProcess->stLink.qwID >> 32) != 0) {
            kSleep(100);
        }
    }
    else {
        kPrintf("Matrix Process Create Fail\n");
    }
}

static void kFPUTestTask(void) {
    double dValue1;
    double dValue2;
    TCB *pstRunningTask;
    QWORD qwCount = 0;
    QWORD qwRandomValue;
    int i;
    int iOffset;
    char vcData[4] = {'-', '\\', '|', '/'};
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

    pstRunningTask = kGetRunningTask(kGetAPICID());

    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(1) {
        dValue1 = 1;
        dValue2 = 1;

        for(i = 0; i < 10; i++) {
            qwRandomValue = kRandom();
            dValue1 *= (double)qwRandomValue;
            dValue2 *= (double)qwRandomValue;

            // kSleep(1);

            qwRandomValue = kRandom();
            dValue1 /= (double)qwRandomValue;
            dValue2 /= (double)qwRandomValue;
        }

        if(dValue1 != dValue2) {
            kPrintf("Value is not same~!!! [%f] != [%f]\n", dValue1, dValue2);
            break;
        }
        qwCount++;

        // 회전하는 바람개비 표시
        pstScreen[iOffset].bCharactor = vcData[qwCount % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
    }
}

static void kTestPIE(const char *pcParameterBuffer) {
    double dResult;
    int i;

    kPrintf("PIE Cacluation Test\n");
    kPrintf("Result : 355 / 113 = ");
    dResult = (double)355 / 113;
    kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10), ((QWORD)(dResult * 100) % 10));

    for(i = 0; i < 100; i++) {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask, TASK_LOADBALANCINGID);
    }
}

// 동적 메모리 정보 표시
static void kShowDynamicMemoryInformation(const char *pcParameterBuffer) {
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

    kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

    kPrintf("============ Dynamic Memory Information ============\n");
    kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
    kPrintf("Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize, qwTotalSize / 1024 / 1024);
    kPrintf("Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize, qwMetaSize / 1024);
    kPrintf("Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);
}

// 모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
static void kTestSequentialAllocation(const char *pcParameterBuffer) {
    DYNAMICMEMORY *pstMemory;
    long i, j, k;
    QWORD *pqwBuffer;

    kPrintf("============ Dynamic Memory Test ============\n");
    pstMemory = kGetDynamicMemoryManager();

    for(i = 0; i < pstMemory->iMaxLevelCount; i++) {
        kPrintf("Block List [%d] Test Start\n", i);
        kPrintf("Allocation And Compare: ");

        // 모든 블록을 할당받아서 값을 채운 후 검사
        for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++) {
            pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
            if(pqwBuffer == NULL) {
                kPrintf("\nAllocation Fail\n");
                return ;
            }

            // 값을 채운 후 다시 검사
            for(k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++) {
                pqwBuffer[k] = k;
            }

            for(k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++) {
                if(pqwBuffer[k] != k) {
                    kPrintf("\nCompare Fail\n");
                    return ;
                }
            }
            // 진행 과정을 .으로 표시
            kPrintf(".");
        }

        // 할당받은 블록을 모두 반환
        for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++) {
            if(kFreeMemory((void*)(pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j)) == FALSE) {
                kPrintf("\nFree Fail\n");
                return ;
            }
            kPrintf(".");
        }
        kPrintf("\n");
    }
    kPrintf("Test Complete~!\n");
}

// 임의로 메모리를 할당하고 해제하는 것을 반복하는 태스크
static void kRandomAllocationTask(void) {
    TCB *pstTask;
    QWORD qwMemorySize;
    char vcBuffer[200];
    BYTE *pbAllocationBuffer;
    int i, j;
    int iY;

    pstTask = kGetRunningTask(kGetAPICID());
    iY = (pstTask->stLink.qwID) % 15 + 9;

    for(j = 0; j < 10; j++) {
        // 1KB ~ 32M까지 할당하도록 함
        do {
            qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
            pbAllocationBuffer = kAllocateMemory(qwMemorySize);

            // 만일 버퍼를 할당하지 못하면 다른 태스크가 메모리를 사용하고 있을 수 있으므로 잠시 대기한 후 시도
            if(pbAllocationBuffer == 0) {
                kSleep(1);
            }
        }while(pbAllocationBuffer == 0);

        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", pbAllocationBuffer, qwMemorySize);
        // 자신의 ID를 y 좌표로 하여 데이터 출력
        kPrintStringXY(20, iY, vcBuffer);
        kSleep(200);

        // 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움
        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for(i = 0; i < qwMemorySize / 2; i++) {
            pbAllocationBuffer[i] = kRandom() & 0xFF;
            pbAllocationBuffer[i + (qwMemorySize / 2)] = pbAllocationBuffer[i];
        }
        kSleep(200);

        // 채운 데이터가 정상적인지 다시 확인
        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        for(i = 0; i < qwMemorySize / 2; i++) {
            if(pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize / 2)]) {
                kPrintf("Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID);
                kExitTask();
            }
        }

        kFreeMemory(pbAllocationBuffer);
        kSleep(200);
    }
    kExitTask();
}

// 태스크를 여러 개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
static void kTestRandomAllocation(const char *pcParameterBuffer) {
    int i;

    for(i = 0; i < 1000; i++) {
        kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandomAllocationTask, TASK_LOADBALANCINGID);
    }
}

static void kShowHDDInformation(const char *pcParameterBuffer) {
    HDDINFORMATION stHDD;
    char vcBuffer[100];

    if(kGetHDDInformation(&stHDD) == FALSE) {
        kPrintf("HDD Information Read Fail\n");
        return;
    }

    kPrintf("============ Primary Master HDD Information ============\n");

    // 모델 번호 출력
    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
    kPrintf("Model Number:\t %s\n", vcBuffer);

    // 시리얼 번호 출력
    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
    kPrintf("Serial Number:\t %s\n", vcBuffer);

    // 헤드, 실린더, 실린더 당 섹터 수 출력
    kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

    // 총 섹터 수 출력
    kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);
}

static void kReadSector(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcLBA[50], vcSectorCount[50];
    DWORD dwLBA;
    int iSectorCount;
    char *pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    if((kGetNextParameter(&stList, vcLBA) == 0) || (kGetNextParameter(&stList, vcSectorCount) == 0)) {
        kPrintf("ex) readsector 0(LBA) 10(count)\n");
        return;
    }

    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);

    // 섹터 수만큼 메모리를 할당받아 읽기 수행
    pcBuffer = kAllocateMemory(iSectorCount * 512);
    if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount) {
        kPrintf("LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount);
        
        // 데이터 버퍼의 내용 출력
        for(j = 0; j < iSectorCount; j++) {
            for(i = 0; i < 512; i++) {
                if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
                    kPrintf("\nPress any key to continue... ('q' is exit) : ");
                    if(kGetCh() == 'q') {
                        bExit = TRUE;
                        break;
                    }
                }

                if((i % 16) == 0) {
                    kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가
                bData = pcBuffer[j * 512 + i] & 0xFF;
                if(bData < 16) {
                    kPrintf("0");
                }
                kPrintf("%X ", bData);
            }

            if(bExit == TRUE) {
                break;
            }
        }
        kPrintf("\n");
    }
    else {
        kPrintf("Read Fail\n");
    }
    
    kFreeMemory(pcBuffer);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수만큼 씀
static void kWriteSector(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcLBA[50], vcSectorCount[50];
    DWORD dwLBA;
    int iSectorCount;
    char *pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    if((kGetNextParameter(&stList, vcLBA) == 0 ) || (kGetNextParameter(&stList, vcSectorCount) == 0)) {
        kPrintf("ex) writesector 0(LBA) 10(count)\n");
        return;
    }

    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);

    s_dwWriteCount++;
    // 버퍼를 할당받아 데이터를 채움
    // 패턴은 4바이트의 LBA 어드레스와 4바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory(iSectorCount * 512);
    for(j = 0; j < iSectorCount; j++) {
        for(i = 0; i < 512; i += 8) {
            *(DWORD*) &(pcBuffer[j * 512 + i]) = dwLBA + j;
            *(DWORD*) &(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
        }
    }

    // 쓰기 수행
    if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount) {
        kPrintf("Write Fail\n");
        return;
    }

    kPrintf("LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount);

    for(j = 0; j < iSectorCount; j++) {
        for(i = 0; i < 512; i++) {
            if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
                kPrintf("\nPress any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q') {
                    bExit = TRUE;
                    break;
                }
            }

            if((i % 16) == 0) {
                kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
            }

            bData = pcBuffer[j * 512 + i] & 0xFF;
            if(bData < 16) {
                kPrintf("0");
            }
            kPrintf("%X ", bData);
        }

        if(bExit == TRUE) {
            break;
        }
    }
    kPrintf("\n");
    kFreeMemory(pcBuffer);
}

// 하드 디스크 연결
static void kMountHDD(const char *pcParameterBuffer) {
    if(kMount() == FALSE) {
        kPrintf("HDD Mount Fail\n");
        return;
    }
    kPrintf("HDD Mount Success\n");
}

// 하드 디스크에 파일 시스템을 생성(포맷)
static void kFormatHDD(const char *pcParameterBuffer) {
    if(kFormat() == FALSE) {
        kPrintf("HDD Format Fail\n");
        return;
    }
    kPrintf("HDD Format Success\n");
}

// 파일 시스템 정보 표시
static void kShowFileSystemInformation(const char *pcParameterBuffer) {
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation(&stManager);

    kPrintf("============ File System Information ============\n");
    kPrintf("Mounted:\t\t\t\t %d\n", stManager.bMounted);
    kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
    kPrintf("Cluster Link Table Start Address:\t %d Sector\n", stManager.dwClusterLinkAreaStartAddress);
    kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
    kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
    kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

// 루트 디렉터리에 빈 파일 생성
static void kCreateFileInRootDirectory(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE *pstFile;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    pstFile = fopen(vcFileName, "w");
    if(pstFile == NULL) {
        kPrintf("File Create Fail\n");
        return;
    }
    fclose(pstFile);
    kPrintf("File Create Success\n");
}

// 루트 디렉터리 파일 삭제
static void kDeleteFileInRootDirectory(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';

    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    if(remove(vcFileName) != 0) {
        kPrintf("File Not Found or File Opened");
        return;
    }

    kPrintf("File Delete Success\n");
}

// 루트 디렉터리의 파일 목록을 표시
static void kShowRootDirectory(const char *pcParameterBuffer) {
    DIR *pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent *pstEntry;
    char vcBuffer[400];
    char vcTempValue[50];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;

    // 파일 시스템 정보 엄음
    kGetFileSystemInformation(&stManager);

    // 루트 디렉터리 열음
    pstDirectory = opendir("/");
    if(pstDirectory == NULL) {
        kPrintf("Root Directory Open Fail\n");
        return;
    }

    // 루프 돌면서 디렉터리에 있는 파일 개수와 전체 파일이 사용한 크기 계산
    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;
    while (1) {
        // 디렉터리에서 엔트리 하나 읽음
        pstEntry = readdir(pstDirectory);

        // 더이상 파일이 없으면 나감
        if(pstEntry == NULL) {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // 실제로 사용된 클러스터 개수 계산
        if(pstEntry->dwFileSize == 0) {
            // 크기가 0이라도 클러스터 1개는 할당됨
            dwUsedClusterCount++;
        }
        else {
            // 클러스터 개수를 올림하여 더함
            dwUsedClusterCount += (pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE;
        }
    }
    
    // 실제 파일의 내용을 표시하는 루프
    rewinddir(pstDirectory);
    iCount = 0;
    while(1) {
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir(pstDirectory);

        // 더이상 파일이 없으면 나감
        if(pstEntry == NULL) {
            break;
        }

        // 전부 공백으로 초기화한 후 각 위치에 값을 대입
        kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
        vcBuffer[sizeof(vcBuffer) - 1] = '\0';

        // 파일 이름 삽입
        kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

        // 파일 길이 삽입
        kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
        kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

        // 파일의 시작 클러스터 삽입
        kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
        kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
        kPrintf("    %s\n", vcBuffer);

        if((iCount != 0) && ((iCount % 20) == 0)) {
            kPrintf("Press any key to continue... ('q' is exit) : ");
            if(kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
        }
        iCount++;
    }

    // 총 파일 개수와 파일의 총 크기 출력
    kPrintf("\t\tTotal File Count: %d\n", iTotalCount);
    kPrintf("\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte / 1024, dwUsedClusterCount);

    // 남은 클러스터 수를 이용해서 여유 공간 출력
    kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", (stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount);

    // 디렉터리 닫음
    closedir(pstDirectory);
}

// 파일을 생성하여 키보드로 입력된 데이터를 씀
static void kWriteDataToFile(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    FILE *fp;
    int iEnterCount;
    BYTE bKey;

    // 파라미터 리스트를 초기화하여 파일 이름 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    // 파일 생성
    fp = fopen(vcFileName, "w");
    if(fp == NULL) {
        kPrintf("%s File Open Fail\n", vcFileName);
        return;
    }

    // 엔터 키가 연속 3번 눌러질 때까지 내용을 파일에 씀
    iEnterCount = 0;
    while (1) {
        bKey = kGetCh();
        if(bKey == KEY_ENTER) {
            iEnterCount++;
            if(iEnterCount >= 3) {
                break;
            }
        }
        // 엔터 키가 아니라면 엔터 키 입력 횟수 초기화
        else {
            iEnterCount = 0;
        }

        kPrintf("%c", bKey);
        if(fwrite(&bKey, 1, 1, fp) != 1) {
            kPrintf("File Write Fail\n");
            break;
        }
    }
    kPrintf("File Create Success\n");
    fclose(fp);
}

// 파일을 열어서 데이터를 읽음
static void kReadDataFromFile(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    FILE *fp;
    int iEnterCount;
    BYTE bKey;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iLength] = '\0';
    if((iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    // 파일 생성
    fp = fopen(vcFileName, "r");
    if(fp == NULL) {
        kPrintf("%s File Open Fail\n", vcFileName);
        return;
    }

    // 파일의 끝까지 출력하는 것을 반복
    iEnterCount = 0;
    while(1) {
        if(fread(&bKey, 1, 1, fp) != 1) {
            break;
        }
        kPrintf("%c", bKey);

        // 만약 엔터 키면 엔터 키 횟수를 증가시키고 20라인까지 출력했다면 더 출력할지 물어봄
        if(bKey == KEY_ENTER) {
            iEnterCount++;

            if((iEnterCount != 0) && ((iEnterCount % 20) == 0)) {
                kPrintf("Press any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q') {
                    kPrintf("\n");
                    break;
                }
                kPrintf("\n");
                iEnterCount = 0;
            }
        }
    }

    fclose(fp);
}

// 파일 I/O에 관련된 기능 테스트
static void kTestFileIO(const char *pcParameterBuffer) {
    FILE *pstFile;
    BYTE *pbBuffer;
    int i, j;
    DWORD dwRandomOffset;
    DWORD dwByteCount;
    BYTE vbTempBuffer[1024];
    DWORD dwMaxFileSize;

    kPrintf("================== File I/O Function Test ==================\n");

    // 4MB 버퍼 할당
    dwMaxFileSize = 4 * 1024 * 1024;
    pbBuffer = kAllocateMemory(dwMaxFileSize);
    if(pbBuffer == NULL) {
        kPrintf("Memory Allocation Fail\n");
        return;
    }

    // 테스트용 파일 삭제
    remove("testfileio.bin");

    // 파일 열기 테스트
    kPrintf("1. File Open Fail Test...");
    // r 옵션은 파일을 생성하지 않으므로 테스트 파일이 없으면 NULL이 되어야 함
    pstFile = fopen("testfileio.bin", "r");
    if(pstFile == NULL) {
        kPrintf("[Pass]\n");
    }
    else {
        kPrintf("[Fail]\n");
        fclose(pstFile);
    }

    // 파일 생성 테스트
    kPrintf("2. File Create Test...");
    // w 옵션은 파일을 생성하므로 정상적으로 핸들이 반환되어야 함
    pstFile = fopen("testfileio.bin", "w");
    if(pstFile != NULL) {
        kPrintf("[Pass]\n");
        kPrintf("    File Handle [0x%Q]\n", pstFile);
    }
    else {
        kPrintf("[Fail]\n");
    }

    // 순차적인 영역 쓰기 테스트
    kPrintf("3. Sequential Write Test(Cluster Size)...");
    
    // 열린 핸들로 쓰기 수행
    for(i = 0; i < 100; i++) {
        kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);
        if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("[Fail]\n");
            kPrintf("    %d Cluster Error\n", i);
            break;
        }
    }
    if(i >= 100) {
        kPrintf("[Pass]\n");
    }

    // 순차적인 영역 읽기 테스트
    kPrintf("4. Sequential Read And Verify Test(Cluster Size)...");
    // 파일의 처음으로 이동
    fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

    // 열린 핸들로 읽기 수행 후 데이터 검증
    for(i = 0; i < 100; i++) {
        // 파일 읽음
        if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("[Fail]\n");
            return;
        }

        // 데이터 검사
        for(j = 0; j < FILESYSTEM_CLUSTERSIZE; j++) {
            if(pbBuffer[j] != (BYTE)i) {
                kPrintf("[Fail]\n");
                kPrintf("    %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[j], (BYTE)i);
                break;
            }
        }
    }
    if(i >= 100) {
        kPrintf("[Pass]\n");
    }

    // 임의의 영역 쓰기 테스트
    kPrintf("5. Random Write Test...\n");

    // 버퍼를 모두 0으로 채움
    kMemSet(pbBuffer, 0, dwMaxFileSize);

    // 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
    // 파일의 내용을 읽어서 버퍼로 복사
    fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
    fread(pbBuffer, 1, dwMaxFileSize, pstFile);

    // 임의의 위치로 옮기면서 데이터를 파일과 버퍼에 동시에 씀
    for(i = 0; i < 100; i++) {
        dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);

        kPrintf("    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

        // 파일 포인터를 이동
        fseek(pstFile, dwRandomOffset, SEEK_SET);
        kMemSet(vbTempBuffer, i, dwByteCount);

        // 데이터를 씀
        if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount) {
            kPrintf("[Fail]\n");
            break;
        }
        else {
            kPrintf("[Pass]\n");
        }

        kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
    }
    // 맨 마지막으로 이동하여 1바이트를 써서 파일의 크기를 4MB로 만듦
    fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
    fwrite(&i, 1, 1, pstFile);
    pbBuffer[dwMaxFileSize - 1] = (BYTE)i;

    // 임의의 영역 읽기 테스트
    kPrintf("6. Random Read And Verify Test...\n");
    // 임의의 위치로 옮기면서 파일에서 데이터를 읽어 버퍼의 내용과 비교
    for(i = 0; i < 100; i++) {
        dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        dwRandomOffset = kRandom() % ((dwMaxFileSize) - dwByteCount);

        kPrintf("    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

        // 파일 포인터를 이동
        fseek(pstFile, dwRandomOffset, SEEK_SET);

        // 데이터를 읽음
        if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount) {
            kPrintf("[Fail]\n");
            kPrintf("    Read Fail\n", dwRandomOffset);
            break;
        }

        // 버퍼와 비교
        if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0) {
            kPrintf("[Fail]\n");
            kPrintf("    Compare Fail\n", dwRandomOffset);
            break;
        }

        kPrintf("[Pass]\n");
    }

    // 다시 순차적인 영역 읽기 테스트
    kPrintf("7. Sequential Write, Read And Verify Test(1024 Byte)...\n");
    // 파일의 처음으로 이동
    fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

    // 열린 핸들로 쓰기 수행. 앞부분에서 2MB만 씀
    for(i = 0; i < (2 * 1024 * 1024 / 1024); i++) {
        kPrintf("    [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024);

        // 1024바이트씩 파일을 씀
        if(fwrite(pbBuffer + (i * 1024), 1, 1024, pstFile) != 1024) {
            kPrintf("[Fail]\n");
            // return; // 계속 에러나서 우선 패스
        }
        else {
            kPrintf("[Pass]\n");
        }
    }

    // 파일의 처음으로 이동
    fseek(pstFile, -dwMaxFileSize, SEEK_SET);

    // 열린 핸들로 읽기 수행 수 데이터 검증
    for(i = 0; i < (dwMaxFileSize / 1024); i++) {
        // 데이터 검사
        kPrintf("    [%d] Offset [%d] Byte [%d] Read And Verify...", i, i * 1024, 1024);

        // 1024바이트씩 파일을 읽음
        if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024) {
            kPrintf("[Fail]\n");
            return;
        }

        if(kMemCmp(pbBuffer + (i * 1024), vbTempBuffer, 1024) != 0) {
            kPrintf("[Fail]\n");
            break;
        }
        else {
            kPrintf("[Pass]\n");
        }
    }

    // 파일 삭제 실패 테스트
    kPrintf("8. File Delete Fail Test...");
    // 파일이 열려 있는 상태이므로 파일을 지우려고 하면 실패해야 함
    if(remove("testfileio.bin") != 0) {
        kPrintf("[Pass]\n");
    }
    else {
        kPrintf("[Fail]\n");
    }

    // 파일 닫기 테스트
    kPrintf("9. File Close Test...");
    // 파일이 정상적으로 닫혀야 함
    if(fclose(pstFile) == 0) {
        kPrintf("[Pass]\n");
    }
    else {
        kPrintf("[Fail]\n");
    }

    // 파일 삭제 테스트
    kPrintf("10. File Delete Test...");
    // 파일이 닫혀있으므로 정상적으로 지워져야 함
    if(remove("testfileio.bin") == 0) {
        kPrintf("[Pass]\n");
    }
    else {
        kPrintf("[Fail]\n");
    }

    // // 임시코드
    // switch (remove("testfileio.bin")){
    // case 0:
    //     kPrintf("[Pass]\n");
    //     break;
    // case -1:
    //     kPrintf("[detected]\n");
    //     break;
    // case 2:
    //     kPrintf("[handle pool search]\n");
    //     break;
    // case 3:
    //     kPrintf("[cluster untilend]\n");
    //     break;
    // case 4:
    //     kPrintf("[memset]\n");
    //     break;
    
    // default:
    //     break;
    // }

    kFreeMemory(pbBuffer);
}

static void kTestPerformance(const char *pcParameterBuffer) {
    FILE *pstFile;
    DWORD dwClusterTestFileSize;
    DWORD dwOneByteTestFileSize;
    QWORD qwLastTickCount;
    DWORD i;
    BYTE *pbBuffer;

    // 클러스터는 1MB까지 파일을 테스트
    dwClusterTestFileSize = 1024 * 1024;
    // 1바이트씩 읽고 쓰는 테스트는 시간이 많이 걸리므로 16KB만 테스트
    dwOneByteTestFileSize = 16 * 1024;

    // 테스트 버퍼 메모리 할당
    pbBuffer = kAllocateMemory(dwClusterTestFileSize);
    if(pbBuffer == NULL) {
        kPrintf("Memory Allocate Fail\n");
        return;
    }

    // 버퍼 초기화
    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

    kPrintf("================== File I/O Performance Test ==================\n");

    // 클러스터 단위로 파일을 순차적으로 쓰는 테스트
    kPrintf("1.Sequential Read/Write Test(Cluster Size)\n");
    
    // 기존의 테스트 파일 제거 후 새로 만듬
    remove("performance.txt");
    pstFile = fopen("performance.txt", "w");
    if(pstFile == NULL) {
        kPrintf("File Open Fail\n");
        kFreeMemory(pbBuffer);
        return;
    }

    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 쓰는 테스트
    for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++) {
        if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("Write Fail\n");

            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }
    
    // 시간 출력
    kPrintf("    Sequential Write(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);

    // 클러스터 단위로 파일을 순차적으로 읽는 테스트
    // 파일의 처음으로 이동
    fseek(pstFile, 0, SEEK_SET);

    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 읽는 테스트
    for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++) {
        if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE) {
            kPrintf("Read Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }

    kPrintf("    Sequential Read(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);
    
    // 1바이트 단위로 파일을 순차적으로 쓰는 테스트
    kPrintf("2.Sequential Read/Write Test(1 Byte)\n");

    // 기존의 테스트 파일을 제거하고 새로 만듬
    remove("performance.txt");
    pstFile = fopen("performance.txt", "w");
    if(pstFile == NULL) {
        kPrintf("File Open Fail\n");
        kFreeMemory(pbBuffer);
        return;
    }

    qwLastTickCount = kGetTickCount();
    
    // 1바이트 단위로 쓰는 테스트
    for(i = 0; i < dwOneByteTestFileSize; i++) {
        if(fwrite(pbBuffer, 1, 1, pstFile) != 1) {
            kPrintf("Write Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }

    kPrintf("    Sequential Write(1 Byte): %d ms\n", kGetTickCount() - qwLastTickCount);

    // 1바이트 단위로 파일을 순차적으로 읽는 테스트
    fseek(pstFile, 0, SEEK_SET);

    qwLastTickCount = kGetTickCount();

    // 1바이트 단위로 읽는 테스트
    for(i = 0;i < dwOneByteTestFileSize; i++) {
        if(fread(pbBuffer, 1, 1, pstFile) != 1) {
            kPrintf("Read Fail\n");
            fclose(pstFile);
            kFreeMemory(pbBuffer);
            return;
        }
    }

    // 시간 출력
    kPrintf("    Sequential Read(1 Byte): %d ms\n", kGetTickCount() - qwLastTickCount);
    fclose(pstFile);
    kFreeMemory(pbBuffer);
}

// 파일 시스템의 캐시 버퍼에 있는 데이터를 모두 하드 디스크에 씀
static void kFlushCache(const char *pcParameterBuffer) {
    QWORD qwTickCount;

    qwTickCount = kGetTickCount();
    kPrintf("Cache Flust... ");
    if(kFlushFileSystemCache() == TRUE) {
        kPrintf("Pass\n");
    }
    else {
        kPrintf("Fail\n");
    }
    
    kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}

// 시리얼 포트로부터 데이터를 수신하여 파일로 저장
static void kDownloadFile(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcFileName[50];
    int iFileNameLength;
    DWORD dwDataLength;
    FILE *fp;
    DWORD dwReceivedSize;
    DWORD dwTempSize;
    BYTE vbDataBuffer[SERIAL_FIFOMAXSIZE];
    QWORD qwLastReceivedTickCount;

    // 파라미터 리스트 초기화 및 파일 이름 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    iFileNameLength = kGetNextParameter(&stList, vcFileName);
    vcFileName[iFileNameLength] = '\0';
    if((iFileNameLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iFileNameLength == 0)) {
        kPrintf("Too Long or Too Short File name\n");
        kPrintf("ex)download a.txt\n");
        return;
    }

    // 시리얼 포트의 FIFO 초기화
    kClearSerialFIFO();

    // 데이터 길이가 수신될 때까지 기다린다는 메시지 출력 및 4바이트 수신 뒤 ACK 전송
    kPrintf("Waitinhg For Data Length.....");
    dwReceivedSize = 0;
    qwLastReceivedTickCount = kGetTickCount();
    while(dwReceivedSize < 4) {
        // 남은 수만큼 데이터 수신
        dwTempSize = kReceiveSerialData(((BYTE*)&dwDataLength) + dwReceivedSize, 4 - dwReceivedSize);
        dwReceivedSize += dwTempSize;

        // 수신된 데이터가 없다면 대기
        if(dwTempSize == 0) {
            kSleep(0);

            // 대기 시간이 30초 이상이면 Time Out
            if((kGetTickCount() - qwLastReceivedTickCount) > 30000) {
                kPrintf("Time Out Occur\n");
                return;
            }
        }
        else {
            // 마지막으로 데이터를 수신한 시간 갱신
            qwLastReceivedTickCount = kGetTickCount();
        }
    }
    
    kPrintf("[%d] Byte\n", dwDataLength);

    // 정상적으로 데이터 길이를 수신했으므로 ACK 송신
    kSendSerialData("A", 1);

    // 파일 생성 후 시리얼로부터 수신한 데이터 파일에 저장
    // 파일 생성
    fp = fopen(vcFileName, "w");
    if(fp == NULL) {
        kPrintf("%s File Open Fail\n", vcFileName);
        return;
    }

    // 데이터 수신
    kPrintf("Data Receive Start: ");
    dwReceivedSize = 0;
    qwLastReceivedTickCount = kGetTickCount();
    while(dwReceivedSize < dwDataLength) {
        // 버퍼에 담아 데이터를 씀
        dwTempSize = kReceiveSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
        dwReceivedSize += dwTempSize;

        // 이번에 데이터가 수신된 것이 있다면 ACK 또는 파일 쓰기 수행
        if(dwTempSize != 0) {
            // 수신하는 쪽은 데이터의 마지막까지 수신했거나 FIFO의 크기인 16바이트마다 한 번씩 ACK 전송
            if(((dwReceivedSize % SERIAL_FIFOMAXSIZE) == 0) || ((dwReceivedSize == dwDataLength))) {
                kSendSerialData("A", 1);
                kPrintf("#");
            }

            // 쓰기 중에 문제가 생기면 바로 종료
            if(fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize) {
                kPrintf("File Write Error Occur\n");
                break;
            }

            // 마지막으로 데이터를 수신한 시간 갱신
            qwLastReceivedTickCount = kGetTickCount();
        }
        // 수신된 데이터가 없다면 대기
        else {
            kSleep(0);

            // 대기한 시간이 10초 이상이면 TIme Out
            if((kGetTickCount() - qwLastReceivedTickCount) > 10000) {
                kPrintf("Time Out Occur\n");
                break;
            }
        }
    }

    // 전체 데이터 크기와 실제로 수신받은 데이터 크기 비교를 통해 성공 여부 출력
    // 이후 파일 닫고 파일 시스템 캐시 모두 비움

    // 수신된 길이를 비교해서 문제가 발생했는지 표시
    if(dwReceivedSize != dwDataLength) {
        kPrintf("\nError Occur. Total Size [%d] Received Size [%d]\n", dwReceivedSize, dwDataLength);
    }
    else {
        kPrintf("\nReceive Complete. Total Size [%d] Byte\n", dwReceivedSize);
    }

    fclose(fp);
    kFlushFileSystemCache();
}

// MP 설정 테이블 정보를 출력
static void kShowMPConfigurationTable(const char *pcParameterBuffer) {
    kPrintMPConfigurationTable();
}

// AP 시작
static void kStartApplicationProcessor(const char *pcParameterBuffer) {
    // AP를 꺠움
    if(kStartUpApplicationProcessor() == FALSE) {
        kPrintf("Application Processor Start Fail\n");
        return;
    }
    kPrintf("Application Processor Start Success\n");

    // BSP APIC ID 출력
    kPrintf("Bootstrap Processor[APIC ID: %d] Start Application Processor\n", kGetAPICID());
}

// 대칭 I/O 모드로 전환
static void kStartSymmetricIOMode(const char *pcParameterBuffer) {
    MPCONFIGRUATIONMANAGER *pstMPManager;
    BOOL bInterruptFlag;

    // MP 설정 테이블 분석
    if(kAnalysisMPConfigurationTable() == FALSE) {
        kPrintf("Analyze MP Configuration Table Fail\n");
        return;
    }

    // MP 설정 매니저를 찾아서 PIC 모드인가 확인
    pstMPManager = kGetMPConfigurationManager();
    if(pstMPManager->bUsePICMode == TRUE) {
        // PIC 모드이면 I/O 포트 어드레스 0x22에 0x70을 먼저 전송하고
        // I/O 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근하여 PIC 모드 비활성화
        kOutPortByte(0x22, 0x70);
        kOutPortByte(0x23, 0x01);
    }

    // PIC 컨트롤러의 인터럽트를 모두 마스크하여 인터럽트가 발생할 수 없도록 함
    kPrintf("Mask All PIC Controller Interrupt\n");
    kMaskPICInterrupt(0xFFFF);

    // 프로세서 전체의 로컬 APIC 활성화
    kPrintf("Enable Global Local APIC\n");
    kEnableGlobalLocalAPIC();

    // 현재 코어의 로컬 APIC를 활성화
    kPrintf("Enable Software Local APIC\n");
    kEnableSoftwareLocalAPIC();

    // 인터럽트 불가로 설정
    kPrintf("Disable CPU Interrupt Flag\n");
    bInterruptFlag = kSetInterruptFlag(FALSE);

    // 모든 인터럽트 수신할 수 있도록 태스크 우선순위 레지스터를 0으로 설정
    kSetTaskPriority(0);

    // 로컬 APIC의 로컬 벡터 테이블 초기화
    kInitializeLocalVectorTable();

    // 대칭 I/O 모드로 변경되었음을 설정
    kSetSymmetricIOMode(TRUE);

    // I/O APIC 초기화
    kPrintf("Initialize IO Redirection Table\n");
    kInitializeIORedirectionTable();

    // 이전 인터럽트 플래그 복원
    kPrintf("Restore CPU Interrupt Flag\n");
    kSetInterruptFlag(bInterruptFlag);

    kPrintf("Change Symmetric I/O Mode Complete\n");
}

// IRQ와 I/O APIC 인터럽트 입력 핀의 관계를 저장한 테이블을 표시
static void kShowIRQINTINMappingTable(const char *pcParameterBuffer) {
    // I/O APIC를 관리하는 자료구조에 있는 출력 함수를 호출
    kPrintIRQToINTINMap();
}

// 코어별로 인터럽트를 처맇나 횟수 출력
static void kShowInterruptProcessingCount(const char *pcParameterBuffer) {
    INTERRUPTMANAGER *pstInterruptManager;
    int i, j;
    int iProcessCount;
    char vcBuffer[20];
    int iRemainLength;
    int iLineCount;

    kPrintf("====================== Interrupt Count =======================\n");

    // MP 설정 테이블에 저장된 코어의 개수를 읽음
    iProcessCount = kGetProcessorCount();
    
    // kPrintf("kGetProcessorCount : %d\n", kGetProcessorCount()); 테스트용 코드
    
    // Column 출력
    // 프로세서의 수만큼 칼럼을 출력
    // 한 줄에 코어 4개씩 출력하고 한 칼럼당 15칸을 할당
    for(i = 0; i < iProcessCount; i++) {
        if(i == 0) {
            kPrintf("IRQ Num\t\t");
        }
        else if((i % 4) == 0) {
            kPrintf("\n     \t\t");
        }
        kSPrintf(vcBuffer, "Core %d", i);
        kPrintf(vcBuffer);

        // 출력하고 남은 공간을 모두 스페이스로 채움
        iRemainLength = 15 - kStrLen(vcBuffer);
        kMemSet(vcBuffer, ' ', iRemainLength);
        vcBuffer[iRemainLength] = '\0';
        kPrintf(vcBuffer);
    }
    kPrintf("\n");

    // Row 인터럽트 처리 횟수 출력
    // 총 인터럽트 횟수와 코어별 인터럽트 처리 횟수 출력
    iLineCount = 0;
    pstInterruptManager = kGetInterruptManager();
    for(i = 0; i < INTERRUPT_MAXVECTORCOUNT; i++) {
        for(j = 0; j < iProcessCount; j++) {
            // Row 출력, 한 줄에 코어 4개씩 출력하고 한 칼럼당 15칸을 할당
            if(j == 0) {
                // 20라인마다 화면 정지
                if((iLineCount != 0) && (iLineCount > 10)) {
                    kPrintf("\nPress any key to continue... ('q' is exit) : ");
                    if(kGetCh() == 'q') {
                        kPrintf("\n");
                        return;
                    }
                    iLineCount = 0;
                    kPrintf("\n");
                }
                kPrintf("-------------------------------------------------------\n");
                kPrintf("IRQ %d\t\t", i);
                iLineCount += 2;
            }
            else if((j % 4) == 0) {
                kPrintf("\n    \t\t");
                iLineCount++;
            }

            kSPrintf(vcBuffer, "0x%Q", pstInterruptManager->vvqwCoreInterruptCount[j][i]);
            kPrintf(vcBuffer);
            iRemainLength = 15 - kStrLen(vcBuffer);
            kMemSet(vcBuffer, ' ', iRemainLength);
            vcBuffer[iRemainLength] = '\0';
            kPrintf(vcBuffer);
        }
        kPrintf("\n");
    }
}

// 인터럽트 부하 분산 기능 시작
static void kStartInterruptLoadBalancing(const char *pcParameterBuffer) {
    kPrintf("Start Interrupt Load Balancing\n");
    kSetInterruptLoadBalancing(TRUE);
}

// 태스크 부하 분산 기능 시작
static void kStartTaskLoadBalancing(const char *pcParameterBuffer) {
    int i;

    kPrintf("Start Task Load Balancing\n");

    for(i = 0; i < MAXPROCESSORCOUNT; i++) {
        kSetTaskLoadBalancing(i, TRUE);
    }
}

// 태스크의 프로세서 친화도를 변경
static void kChangeTaskAffinity(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcID[30];
    char vcAffinity[30];
    QWORD qwID;
    BYTE bAffinity;

    // 파라미터 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcAffinity);

    // 태스크 ID 필드 추출
    if(kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    }
    else {
        qwID = kAToI(vcID, 10);
    }

    // 프로세서 친화도 추출
    if(kMemCmp(vcID, "0x", 2) == 0) {
        bAffinity = kAToI(vcAffinity + 2, 16);
    }
    else {
        bAffinity = kAToI(vcAffinity, 10);
    }
    
    kPrintf("Change Task Affinity ID [0x%q] Affinity[0x%x] ", qwID, bAffinity);
    if(kChangeProcessorAffinity(qwID, bAffinity) == TRUE) {
        kPrintf("Success\n");
    }
    else {
        kPrintf("Fail\n");
    }

}