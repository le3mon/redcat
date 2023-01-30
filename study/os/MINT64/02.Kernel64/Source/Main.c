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
    kPrintf("Change To MultiCore Processor Mode..........[    ]\n");
    if(kChangeToMultiCoreMode() == TRUE) {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
    }

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

// x를 절대값으로 변환하는 매크로
#define ABS(x)      (((x) >= 0) ? (x) : -(x))

// 임의의 x, y 좌표를 반환
void kGetRandomXY(int *piX, int *piY) {
    int iRandom;

    // x좌표를 계산
    iRandom = kRandom();
    *piX = ABS(iRandom) % 1000;

    // y좌표를 계산
    iRandom = kRandom();
    *piY = ABS(iRandom) % 700;
}

// 임의의 색을 반환
COLOR kGetRandomColor(void) {
    int iR, iG, iB;
    int iRandom;

    iRandom = kRandom();
    iR = ABS(iRandom) % 256;

    iRandom = kRandom();
    iG = ABS(iRandom) % 256;

    iRandom = kRandom();
    iB = ABS(iRandom) % 256;

    return RGB(iR, iG, iB);
}

// 윈도우 프레임을 그림
void kDrawWindowFrame_test(int iX, int iY, int iWidth, int iHeight, const char *pcTitle) {
    char *pcTestString = "This is MINT64 OS's window prototype~!!!";
    char *pcTestString2 = "Coming soon~!!";
    VBEMODEINFOBLOCK *pstVBEMode;
    COLOR *pstVideoMemory;
    RECT stScreenArea;

    // VBE 모드 정보 블록 반환
    pstVBEMode = kGetVBEModeInfoBlock();

    // 화면 영역 설정
    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    // 그래픽 메모리 어드레스 설정
    pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

    // 윈도우 프레임의 가장자리를 그림, 2픽셀 두께
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX, iY, iX + iWidth, iY + iHeight, RGB(109, 218, 22), FALSE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + 1, iY + 1, iX + iWidth - 1, iY + iHeight - 1, RGB(109, 218, 22), FALSE);

    // 제목 표시줄을 채움
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX, iY + 3, iX + iWidth - 1, iY + 21, RGB(79, 204, 11), TRUE);

    // 윈도우 제목 표시
    kInternalDrawText(&stScreenArea, pstVideoMemory,
        iX + 6, iY + 3, RGB(255, 255, 255), RGB(79, 204, 11), pcTitle, kStrLen(pcTitle));

    // 제목 표시줄을 입체로 보이게 위쪽의 선을 그림, 2픽셀 두께
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 1, iY + 1, iX + iWidth - 1, iY + 1, RGB(183, 249, 171));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 1, iY + 2, iX + iWidth - 1, iY + 2, RGB(150, 210, 140));

    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 1, iY + 2, iX + 1, iY + 20, RGB(183, 249, 171));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 2, iY + 2, iX + 2, iY + 20, RGB(150, 210, 140));

    // 제목 표시줄의 아래쪽에 선을 그림
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 2, iY + 19, iX + iWidth - 2, iY + 19, RGB(46, 59, 30));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + 2, iY + 20, iX + iWidth - 2, iY + 20, RGB(46, 59, 30));

    // 닫기 버튼을 그림, 오른쪽 상단에 표시
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2, iY + 19, RGB(255, 255, 255), TRUE);

    // 닫기 버튼을 입체로 보이게 선을 그림, 2픽셀 두께로 그림
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2, iY + 1, iX + iWidth - 2, iY + 19 - 1, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 1, iY + 1, iX + iWidth - 2 - 1, iY + 19 - 1, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 1, iY + 19, iX + iWidth - 2, iY + 19, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 1, iY + 19 - 1, iX + iWidth - 2, iY + 19 - 1, RGB(86, 86, 86), TRUE);

    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 1, iY + 1, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18, iY + 1 + 1, iX + iWidth - 2 - 2, iY + 1 + 1, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 18, iY + 19, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 1, iY + 1, iX + iWidth - 2 - 18 + 1, iY + 19 - 1, RGB(229, 229, 229));

    // 대각선 x를 그림, 3픽셀로 그림
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 4, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 4, RGB(71, 199, 21));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 5, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 5, RGB(71, 199, 21));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 4, iY + 1 + 5, iX + iWidth - 2 - 5, iY + 19 - 4, RGB(71, 199, 21));

    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 4, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 4, RGB(71, 199, 21));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 5, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 5, RGB(71, 199, 21));
    kInternalDrawLine(&stScreenArea, pstVideoMemory,
        iX + iWidth - 2 - 18 + 4, iY + 19 - 5, iX + iWidth - 2 - 5, iY + 1 + 4, RGB(71, 199, 21));

    // 내부를 그림
    kInternalDrawRect(&stScreenArea, pstVideoMemory,
        iX + 2, iY + 21, iX + iWidth - 2, iY + iHeight - 2, RGB(255, 255, 255), TRUE);
    
    // 테스트 문자 출력
    kInternalDrawText(&stScreenArea, pstVideoMemory,
        iX + 10, iY + 30, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString, kStrLen(pcTestString));
    kInternalDrawText(&stScreenArea, pstVideoMemory,
        iX + 10, iY + 50, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString2, kStrLen(pcTestString2));
}

// 마우스 커서를 위해 추가
// 마우스의 너비와 높이
#define MOUSE_CURSOR_WIDTH      20
#define MOUSE_CURSOR_HEIGHT     20

// 마우스 커서 이미지 저장 데이터
BYTE gs_vwMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
    0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};

// 커서 이미지 색깔
#define MOUSE_CURSOR_OUTERLINE      RGB(0, 0, 0)
#define MOUSE_CURSOR_OUTER          RGB(79, 204, 11)
#define MOUSE_CURSOR_INNER          RGB(232, 255, 232)

// X, Y 위치에 마우스 커서 출력
void kDrawCursor(RECT *pstArea, COLOR *pstVideoMemory, int iX, int iY) {
    int i, j;
    BYTE *pbCurrentPos;

    // 커서 데이터의 시작 위치 설정
    pbCurrentPos = gs_vwMouseBuffer;

    // 커서 너비와 높이만큼 루프를 돌면서 픽셀을 화면에 출력
    for(j = 0; j < MOUSE_CURSOR_HEIGHT; j++) {
        for(i = 0; i < MOUSE_CURSOR_WIDTH; i++) {
            switch (*pbCurrentPos) {
            // 0은 출력 X
            case 0:
                break;
            
            // 가장 바깥쪽 테두리, 검은색으로 출력
            case 1:
                kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
                    MOUSE_CURSOR_OUTERLINE);
                break;
            
            // 안쪽과 바깥쪽 경계, 어두운 녹새으로 출력
            case 2:
                kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
                    MOUSE_CURSOR_OUTER);
                break;
            
            // 커서의 안, 밝은 색으로 출력
            case 3:
                kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY,
                    MOUSE_CURSOR_INNER);
                break;
            }

            // 커서의 픽셀이 표시됨에 따라 커서 데이터의 위치도 같이 이동
            pbCurrentPos++;
        }
    }
}

// 그래픽 모드를 테스트하는 함수
void kStartGraphicModeTest() {
    VBEMODEINFOBLOCK *pstVBEMode;
    int iX, iY;
    COLOR *pstVideoMemory;
    RECT stScreenArea;
    int iRelativeX, iRelativeY;
    BYTE bButton;
    
    int iX1, iY1, iX2, iY2;
    COLOR stColor1, stColor2;
    int i;
    char *vpcString[] = {"Pixel", "Line", "Rectangle", "Circle", "MINT64 OS~!!!"};
    
    

    // VBE 모드 정보 블록 반환
    pstVBEMode = kGetVBEModeInfoBlock();

    // 화면 영역 설정
    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    // 그래픽 메모리 어드레스 설정
    pstVideoMemory = (COLOR*)pstVBEMode->dwPhysicalBasePointer;

    // 마우스의 초기 위치를 화면 가운데로 설정
    iX = pstVBEMode->wXResolution / 2;
    iY = pstVBEMode->wYResolution / 2;

    // 마우스 커서 출력 및 이동 처리
    // 배경 출력
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 
        stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2,
        stScreenArea.iY2, RGB(232, 255, 232), TRUE);

    // 현재 위치에 마우스 커서 출력
    kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);

    while(1) {
        // 마우스 데이터 수신 대기
        if(kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) == FALSE) {
            kSleep(0);
            continue;
        }

        // 이전에 마우스 커서가 있던 위치에 배경 출력
        kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY,
            iX + MOUSE_CURSOR_WIDTH, iY + MOUSE_CURSOR_HEIGHT,
            RGB(232, 255, 232), TRUE);
        
        // 마우스가 움직인 거리를 이전 커서 위치에 더해서 현재 좌표 계산
        iX += iRelativeX;
        iY += iRelativeY;

        // 마우스 커서가 화면을 벗어나지 못하도록 보정
        if(iX < stScreenArea.iX1) {
            iX = stScreenArea.iX1;
        }
        else if(iX > stScreenArea.iX2) {
            iX = stScreenArea.iX2;
        }

        if(iY < stScreenArea.iY1) {
            iY = stScreenArea.iY1;
        }
        else if(iY > stScreenArea.iY2) {
            iY = stScreenArea.iY2;
        }

        // 왼쪽 버튼이 눌러지면 윈도우 프로토타입 표시
        if(bButton & MOUSE_LBUTTONDOWN) {
            kDrawWindowFrame_tset(iX - 10, iY - 10, 400, 200, "MINT64 OS Test Window");
        }
        // 오른쪽 버튼이 눌러지면 화면 전체를 배경색으로 채움
        else if(bButton & MOUSE_RBUTTONDOWN) {
            kInternalDrawRect(&stScreenArea, pstVideoMemory,
                stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2,
                stScreenArea.iY2, RGB(232, 255, 232), TRUE);
        }

        // 변경된 마우스 위치에 마우스 커서 출력 이미지를 출력
        kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);
    }
}