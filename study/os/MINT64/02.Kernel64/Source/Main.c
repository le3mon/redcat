#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0;
    KEYDATA stData;

    kPrintString(0, 10, "Switch To IA-32e Mode Success");
    kPrintString(0, 11, "IA-32e C Language Kernel Start..............[Pass]");
    kPrintString(0, 12, "GDT Initialize And Switch For IA-32e Mode...[    ]");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kPrintString(45, 12, "Pass");

    kPrintString(0, 13, "TSS Segment Load............................[    ]");
    kLoadTR(GDT_TSSSEGMENT);
    kPrintString(45, 13, "pass");

    kPrintString(0, 14, "IDT Initalize...............................[    ]");
    kInitalizeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kPrintString(45, 14, "pass");

    kPrintString(0, 15, "Keyboard Activate...........................[    ]");
    
    if(kInitializeKeyboard() == TRUE) {
        kPrintString(45, 15, "Pass");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kPrintString(45, 15, "Fail");
        while(1);
    }

    kPrintString(0, 16, "PIC Controller And Interrupt Initalize......[    ]");
    // PIC 컨트롤러 초기화 및 모든 인터럽트 활성화
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEndableInterrupt();
    kPrintString(45, 16, "Pass");

    while (1) {
        // 키 큐에 데이터가 있으면 키를 처리
        if(kGetKeyFromKeyQueue(&stData) == TRUE) {
            if(stData.bFlags & KEY_FLAGS_DOWN) {
                vcTemp[0] = stData.bASCIICode;
                kPrintString(i++, 17, vcTemp);
                
                if(vcTemp[0] == '0') {
                    bTemp = bTemp / 0;
                }
            }
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