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
#include "VBE.h"
#include "2DGraphics.h"
#include "MPConfigurationTable.h"
#include "Mouse.h"
#include "InterruptHandler.h"
#include "IOAPIC.h"
#include "WindowManagerTask.h"
#include "SystemCall.h"

// AP를 위한 Main 함수
void MainForApplicationProcessor(void);

// 멀티코어 프로세서 또는 멀티프로세서 모드로 전환하는 함수
BOOL kChangeToMultiCoreMode(void);

// 그래픽 모드 테스트 함수
void kStartGraphicModeTest();

void Main(void) {
    int iCursorX, iCursorY;
    BYTE bButton;
    int iX, iY;

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

    kPrintf("Mouse Activate And Queue Initialize.........[    ]\n");
    // 마우스를 활성화
    if(kInitializeMouse() == TRUE) {
        kEnableMouseInterrupt();
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
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

    // 멀티코어 프로세서 모드로 전환
    // AP 활성화, I/O 모드 활성화, 인터럽트와 태스크 부하 분산 기능 활성화
    kPrintf("Change To MultiCore Processor Mode..........[    ]");
    if(kChangeToMultiCoreMode() == TRUE) {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
    }

    // 시스템 콜에 관련된 MSR 초기화
    kPrintf("System Call MSR Initialize..................[PASS]\n");
    iCursorY++;
    kInitializeSystemCall();

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM |
        TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

    // 그래픽 모드가 아니면 콘솔 셸 실행
    if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
        kStartConsoleShell();    
    }
    // 그래픽 모드면 그래픽 모드 테스트 함수 실행
    else {
        kStartWindowManager();
    }
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

    // 스케줄러 초기화
    kInitializeScheduler();

    // 현재 코어의 로컬 APIC를 활성화
    kEnableSoftwareLocalAPIC();

    // 모든 인터럽트를 수신할 수 있도록 태스크 우선 순위 레지스터를 0으로 설정
    kSetTaskPriority(0);

    // 로컬 APIC의 로컬 벡터 테이블 초기화
    kInitializeLocalVectorTable();

    // 인터럽트 활성화
    kEnableInterrupt();

    // 시스템 콜에 관련된 MSR 초기화
    kInitializeSystemCall();

    // 대칭 I/O 모드 테스트를 위해 AP가 시작한 후 한번만 출력
    // kPrintf("Application Processor[APIC ID: %d] Is Activated\n", kGetAPICID());

    // 유휴 태스크 실행
    kIdleTask();
}

// 멀티코어 프로세서 또는 멀티프로세서 모드로 전환하는 함수
BOOL kChangeToMultiCoreMode(void) {
    MPCONFIGRUATIONMANAGER *pstMPManager;
    BOOL bInterruptFlag;
    int i;

    // AP 활성화
    if(kStartUpApplicationProcessor() == FALSE) {
        return FALSE;
    }

    // MP 설정 매니저를 찾아서 PIC 모드인지 확인
    pstMPManager = kGetMPConfigurationManager();
    if(pstMPManager->bUsePICMode == TRUE) {
        // PIC 모드이면 I/O 포트 어드레스 0x22에 0x70을 먼저 전송하고 I/O 포트 어드레스 0x23에
        // 0x01을 전송하는 방법으로 IMCR 레지스터에 접근하여 PIC 모드 비활성화
        kOutPortByte(0x22, 0x70);
        kOutPortByte(0x23, 0x01);
    }

    // PIC 컨트롤러의 인터럽트를 모두 마스크하여 인터럽트가 발생할 수 없도록 함
    kMaskPICInterrupt(0xFFFF);

    // 프로세서 전체의 로컬 APIC를 활성화
    kEnableGlobalLocalAPIC();

    // 현재 코어의 로컬 APIC 활성화
    kEnableSoftwareLocalAPIC();

    // 인터럽트 불가로 설정
    bInterruptFlag = kSetInterruptFlag(FALSE);

    // 모든 인터럽트를 수신할 수 있도록 태스크 우선순위 레지스터를 0으로 설정
    kSetTaskPriority(0);

    // 로컬 APIC의 로컬 벡터 테이블 초기화
    kInitializeLocalVectorTable();

    // 대칭 I/O 모드로 변경되었음을 설정
    kSetSymmetricIOMode(TRUE);

    // I/O APIC 초기화
    kInitializeIORedirectionTable();

    // 이전 인터럽트 플래그 복원
    kSetInterruptFlag(bInterruptFlag);

    // 인터럽트 부하 분산 기능 활성화
    kSetInterruptLoadBalancing(TRUE);

    // 태스크 부하 분산 기능 활성화
    for(i = 0; i < MAXPROCESSORCOUNT; i++) {
        kSetTaskLoadBalancing(i, TRUE);
    }

    return TRUE;
}