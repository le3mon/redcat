#include "../MINT64/02.Kernel64/Source/Types.h"

void kStartConsoleShell(void) {
    char vcCommandBuffer[300];
    int iCommandBufferInde = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    kPrintf("MINT64>");
    
    while(1) {
        bKey = kGetCh();
        if(bKey == KEY_BACKSPACE) {
            if(iCommandBufferInde > 0) {
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStrintg(iCursorX - 1, iCursorY, " ");
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferInde--;
            }
        }
        else if(bKey == KEY_ENTER) {
            kPrintf("\n");

            if(iCommandBufferInde > 0) {
                vcCommandBuffer[iCommandBufferInde] = '\0'
                kExecuteCommand(vcCommandBuffer);
            }

            kprintf("%s", "MINT64>");
            kMemSet(vcCommandBuffer, '\0', 300);
            iCommandBufferInde = 0;
        }
        else if((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) || (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLOCK)) {
            ;
        }
        else {
            if(bKey == KEY_TAB) {
                bKey = ' ';
            }

            if(iCommandBufferInde < 300) {
                vcCommandBuffer[iCommandBufferInde++]  = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}