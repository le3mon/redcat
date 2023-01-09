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
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "Utility.h"
#include "LocalAPIC.h"

// AP를 위한 Main 함수
void MainForApplicationProcessor(void);

void kPrintString(int iX, int iY, const char* pcString);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0;
    KEYDATA stData;
    int iCursorX, iCursorY;

    // 부트 로더에 있는 BSP 플래그를 읽어서 AP이면 해당 코어용 초기화 함수로 이동
    if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
        MainForApplicationProcessor();
    }

    // Bootstrap Processor가 부팅을 완료했으므로, 0x7c09에 있는 Bootstrap Processor를 나타내는 플래그를
    // 0으로 설정하여 AP용으로 코드 실행 경로 변경
    *((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

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

    // 시리얼 포트 초기화
    kPrintf("Serial Port Initialize......................[Pass]\n");
    iCursorY++;
    kInitializeSerialPort();

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

void MainForApplicationProcessor(void) {
    QWORD qwTickCount;

    // GDT 테이블을 설정
    kLoadGDTR(GDTR_STARTADDRESS);

    // TSS 디스크립터를 설정. TSS 세그먼트와 디스크립터를 AP 수만큼 생성했으므로
    // APIC ID를 이용하여 TSS 디스크립터를 할당
    kLoadTR(GDT_TSSSEGMENT + (kGetAPICID() * sizeof(GDTENTRY16)));

    // IDT 테이블 설정
    kLoadIDTR(IDTR_STARTADDRESS);

    // 현재 코어의 로컬 APIC를 활성화
    kEnableSoftwareLocalAPIC();

    // 모든 인터럽트를 수신할 수 있도록 태스크 우선 순위 레지스터를 0으로 설정
    kSetTaskPriority(0);

    // 로컬 APIC의 로컬 벡터 테이블 초기화
    kInitializeLocalVectorTable();

    // 인터럽트 활성화
    kEnableInterrupt();

    // 대칭 I/O 모드 테스트를 위해 AP가 시작한 후 한번만 출력
    kPrintf("Application Processor[APIC ID: %d] Is Activated\n", kGetAPICID());

    // 1초마다 한 번씩 메시지 출력
    qwTickCount = kGetTickCount();
    
    while(1) {
        if(kGetTickCount() - qwTickCount > 1000) {
            qwTickCount = kGetTickCount();

            // kPrintf("Application Processor[APIC ID: %d] Is Activated\n", kGetAPICID());
        }
    }
}


void kPrintString(int iX, int iY, const char* pcString) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    int i;

    pstScreen += (iY*80) + iX;
    for(i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}