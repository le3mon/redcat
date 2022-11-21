#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0;

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

    if(kActivateKeyboard() == TRUE) {
        kPrintString(45, 15, "Pass");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kPrintString(45, 15, "Fail");
        while(1);
    }

    while (1) {
        if(kIsOutputBufferFull() == TRUE) {
            bTemp = kGetKeyboardScanCode();

            if(kConvertScanCodeToASCIITable(bTemp, &(vcTemp[0]), &bFlags) == TRUE) {
                if(bFlags & KEY_FLAGS_DOWN) {
                    kPrintString(i++, 16, vcTemp);

                    // 0이 입력되면 변수를 0으로 나누어 Divide Error 예외(벡터 0번) 발생
                    if(vcTemp[0] == '0') {
                        bTemp = bTemp / 0;
                    }
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