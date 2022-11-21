#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"

#define PIC_MASTER_PORT1    0x20
#define PIC_SLAVE_PORT1     0xA0

void kSendEOIToPIC(int iIRQNumber) {
    kOutPortByte(PIC_MASTER_PORT1, 0x20);
    if(iIRQNumber > 8)
        kOutPortByte(PIC_SLAVE_PORT1, 0x20);
}