#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kInitalizeKernel64Area(void);
BOOL kIsMemoryEnough(void);
void kCopyKernel64ImageTo2Mbyte(void);

#define BOOTSTRAPPROCESSOR_FLAGADDRESS 0x7C09

void Main(void) {
    DWORD i;
    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[13] = {0, };

    // Application Processor이면 아래 코드를 생략하고 바로 64비트 모드로 전환
    if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
        kSwitchAndExecute64bitKernel();
        while(1);
    }

    kPrintString(0, 3, "Protected Mode C Language Kernel Start......[Pass]");

    kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");

    if(kIsMemoryEnough() == FALSE) {
        kPrintString(45, 4, "Fail");
        kPrintString(0, 5, "Not Enough Memory~!! MINT64 OS Requires Over 64MB Memory~!!!!");
        while (1);
    }
    else {
        kPrintString(45, 4, "Pass");
    }

    kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
    if(kInitalizeKernel64Area() == FALSE) {
        kPrintString(45, 4, "Fail");
        kPrintString(0, 6, "Kernel Area Initalization Fail~!!");
        while (1);
    }
    kPrintString(45, 5, "Pass");

    // IA-32e 모드 커널을 위한 페이지 테이블 생성
    kPrintString(0, 6, "IA-32e Page Tables Initalize................[    ]");
    kInitializePageTables();
    kPrintString(45, 6, "Pass");

    kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(DWORD*)vcVendorString = dwEBX;
    *((DWORD*)vcVendorString + 1) = dwEDX;
    *((DWORD*)vcVendorString + 2) = dwECX;
    kPrintString(0, 7, "Processor Vendor String.....................[            ]");
    kPrintString(45, 7, vcVendorString);

    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    kPrintString(0, 8, "64bit Mode Support Check....................[    ]");
    if(dwEDX & (1 << 29))
        kPrintString(45, 8, "Pass");
    else {
        kPrintString(45, 8, "Fail");
        kPrintString(0, 9, "This processor does not support 64bit mode~");
        while (1);
    }

    kPrintString(0, 9, "Copy IA-32e Kernel To 2MB Address...........[    ]");
    kCopyKernel64ImageTo2Mbyte();
    kPrintString(45, 9, "Pass");

    kPrintString(0, 10, "Switch To IA-32e Mode");
    kSwitchAndExecute64bitKernel(); // IA-32e 모드 커널이 아직 없으므로 주석처리
    
    while (1);
}

void kPrintString(int iX, int iY, const char* pcString) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    int i;

    pstScreen += (iY*80) + iX;
    for(i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}

BOOL kInitalizeKernel64Area(void) {
    DWORD *pdwCurrentAddress;

    pdwCurrentAddress = (DWORD*)0x100000; // 초기화 시작 어드레스 1MB 설정

    //6MB 까지 루프를 돌면서 4 바이트 씩 0으로 채움
    while((DWORD)pdwCurrentAddress < 0x600000) {  
        *pdwCurrentAddress = 0x00;

        // 0으로 저장 후 다시 확인할 때 0이 아니면 문제가 발생한 것 이므로 종료
        if(*pdwCurrentAddress != 0) 
            return FALSE;

        pdwCurrentAddress++;
    }
    
    return TRUE;
}

BOOL kIsMemoryEnough(void) {
    DWORD *pdwCurrentAddress;

    pdwCurrentAddress = (DWORD*)0x100000; // 1MB 부터 검사 시작

    while((DWORD)pdwCurrentAddress < 0x4000000) {
        *pdwCurrentAddress = 0x12345678;    // 해당 주소에 0x12345678 값 입력

        // 입력한 값과 읽어온 값이 다르면 해당 어드레스 사용의 문제가 있는 것 이므로 FALSE 반환
        if(*pdwCurrentAddress != 0x12345678) 
            return FALSE;
        
        // 1MB 씩 이동
        pdwCurrentAddress += (0x100000 / 4);
    }
    return TRUE;
}

void kCopyKernel64ImageTo2Mbyte() {
    WORD wKernel32SectorCount, wTotalKernelSectorCount;
    DWORD *pdwSourceAddress, *pdwDestinationAddress;
    int i;

    wTotalKernelSectorCount = *((WORD*)0x7C05);
    wKernel32SectorCount = *((WORD*)0x7C07);

    pdwSourceAddress = (DWORD*)(0x10000 + (wKernel32SectorCount*512));
    pdwDestinationAddress = (DWORD*)0x200000;

    for(i = 0; i < 512*(wTotalKernelSectorCount - wKernel32SectorCount) / 4; i++) { // 4를 나눠주는 이유은 4바이트 씩 처리 중이기 때문
        *pdwDestinationAddress = *pdwSourceAddress;
        pdwDestinationAddress++;
        pdwSourceAddress++;
    }
}