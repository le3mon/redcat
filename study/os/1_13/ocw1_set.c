#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"
#include "../MINT64/02.Kernel64/Source/Types.h"

#define PIC_MASTER_PORT2    0x21
#define PIC_SLAVE_PORT2     0xA1

void kMaskPICInterrupt(WORD wIRQBitmask) {
    kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}