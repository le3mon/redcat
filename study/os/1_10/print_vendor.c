#include "../MINT64/01.Kernel32/Source/Types.h"

int main() {
    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[13] = {0,};

    kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(DWORD*) vcVendorString = dwEBX;
    *((DWORD*) vcVendorString + 1) = dwEBX;
    *((DWORD*) vcVendorString + 2) = dwEDX;

    kPrintString(0, 0, vcVendorString);
}