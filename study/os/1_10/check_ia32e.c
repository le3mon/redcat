#include "../MINT64/01.Kernel32/Source/Types.h"

int main() {
    DWORD dwEAX, dwEBX, dwECX, dwEDX;

    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    if (dwEDX & (1<<29))
        kPrintString(0, 0, "PASS");    

    else
        kPrintString(0, 0, "FAIL");
}