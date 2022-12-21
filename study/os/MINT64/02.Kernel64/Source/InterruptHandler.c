#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = {0, };

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;
    kPrintStringXY( 0, 0, "==================================================" );
    kPrintStringXY( 0, 1, " Exception Occur~!!!! " );
    kPrintStringXY( 0, 2, " Vector: " );
    kPrintStringXY( 27, 2, vcBuffer );
    kPrintStringXY( 0, 3, "==================================================" );
    
    while(1);
}

void kCommonInterruptHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    // 키보드 컨트롤러에서 데이터를 읽어서 아스키로 변환하여 큐에 삽입
    if(kIsOutputBufferFull() == TRUE) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kTimerHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iTimerInterruptCount;

    g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    // EOI 전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

    // 타이머 발생 횟수 증가
    g_qwTickCount++;

    // 태스크가 사용한 프로세서의 시간을 줄임
    kDecreaseProcessorTime();

    // 프로세서가 사용할 수 있는 시간을 다 썼다면 태스크 전환 수행
    if(kIsProcessorTimeExpired() == TRUE)
        kScheduleInInterrupt();
}

void kDeviceNotAvailableHandler(int iVectorNumber) {
    TCB *pstFPUTask, *pstCurrentTask;
    QWORD qwLastFPUTaskID;

    char vcBuffer[] = "[EXC: , ]";
    static int g_iFPUInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    // CR0 컨트롤 레지스터의 TS 비트 0으로 설정
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인하여 FPU 상태 태스크에 저장
    qwLastFPUTaskID = kGetLastFPUUsedTaskID();
    pstCurrentTask = kGetRunningTask();

    // 이전에 FPU를 사용한 것이 자신이면 리턴
    if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID) {
        return ;
    }

    // FPU를 사용한 태스크가 있으면 FPU 상태 저장
    else if(qwLastFPUTaskID != TASK_INVALIDID) {
        pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
        if((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)) {
            kSaveFPUContext(pstFPUTask->vqwFPUContext);
        }
    }

    // 현재 태스크가 FPU를 사용한 적이 없으면 초기화, 있으면 저장된 FPU 복원
    if(pstCurrentTask->bFPUUsed == FALSE) {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    }
    else {
        kLoadFPUContext(pstCurrentTask->vqwFPUContext);
    }

    kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iHDDInterruptCount = 0;
    BYTE bTemp;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;

    // 왼쪽 위에 있는 메시지와 겹치지 않도록 10, 0 출력
    kPrintStringXY(10, 0, vcBuffer);

    // 첫 번쨰 PATA 포트의 인터럽트 벡터 처리
    if(iVectorNumber - PIC_IRQSTARTVECTOR == 14) {
        // 첫 번째 PATA 포트의 인터럽트 발생 여부를 TRUE 설정
        kSetHDDInterruptFlag(TRUE, TRUE);
    }
    else {
        // 두 번째 PATA 포트의 인터럽트 발생 여부를 TRUE 설정
        kSetHDDInterruptFlag(FALSE, FALSE);
    }

    // EOI 전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}