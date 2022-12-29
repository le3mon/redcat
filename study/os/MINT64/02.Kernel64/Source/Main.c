#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0;
    KEYDATA stData;
    int iCursorX, iCursorY;

    kInitializeConsole(0, 10);
    
    kPrintf("Switch To IA-32e Mode Success\n");
    kPrintf("IA-32e C Language Kernel Start..............[Pass]\n");
    kPrintf("Initialize Console..........................[Pass]\n");
    kGetCursor(&iCursorX, &iCursorY);

    kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]\n");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("TSS Segment Load............................[    ]\n");
    kLoadTR(GDT_TSSSEGMENT);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("IDT Initalize...............................[    ]\n");
    kInitalizeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("Total RAM Size Check........................[    ]\n");
    kCheckTotalRAMSize();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass], Size = %d MB \n", kGetTotalRAMSize());

    kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n");
    iCursorY++;
    kInitializeScheduler();

    // 동적 메모리 초기화
    kPrintf("Dynamic Memory Initialize...................[pass]\n");
    iCursorY++;
    kInitializeDynamicMemory();

    // 1ms당 한 번씩 인터럽트가 발생하도록 설정
    kInitializePIT(MSTOCOUNT(1), 1);

    kPrintf("Keyboard Activate...........................[    ]\n");
    
    if(kInitializeKeyboard() == TRUE) {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
        while(1);
    }

    kPrintf("PIC Controller And Interrupt Initalize......[    ]\n");
    // PIC 컨트롤러 초기화 및 모든 인터럽트 활성화
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    // // 하드 디스크 초기화
    // kPrintf("HDD Initialize..............................[    ]\n");
    // if(kInitializeHDD() == TRUE) {
    //     kSetCursor(45, iCursorY++);
    //     kPrintf("Pass\n");
    // }
    // else {
    //     kSetCursor(45, iCursorY++);
    //     kPrintf("Fail\n");
    // }

    // 파일 시스템 초기화
    kPrintf("File System Initialize......................[    ]\n");
    if(kInitializeFileSystem() == TRUE) {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
    }

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM |TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
    kStartConsoleShell();
    // while (1) {
    //     // 키 큐에 데이터가 있으면 키를 처리
    //     if(kGetKeyFromKeyQueue(&stData) == TRUE) {
    //         if(stData.bFlags & KEY_FLAGS_DOWN) {
    //             vcTemp[0] = stData.bASCIICode;
    //             kPrintString(i++, 17, vcTemp);
                
    //             if(vcTemp[0] == '0') {
    //                 bTemp = bTemp / 0;
    //             }
    //         }
    //     }
    // }
}

void kPrintString(int iX, int iY, const char* pcString) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    int i;

    pstScreen += (iY*80) + iX;
    for(i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}