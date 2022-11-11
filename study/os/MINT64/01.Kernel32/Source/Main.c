#include "Types.h"
#include "Page.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kInitalizeKernel64Area(void);
BOOL kIsMemoryEnough(void);

void Main(void) {
    DWORD i;

    kPrintString(0, 3, "C Language Kernel Start.....................[Pass]");

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