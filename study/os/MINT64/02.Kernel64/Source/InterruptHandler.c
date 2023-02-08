#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "LocalAPIC.h"
#include "MPConfigurationTable.h"
#include "Mouse.h"

// 인터럽트 핸들러 자료구조
static INTERRUPTMANAGER gs_stInterruptManager;

void kInitializeHandler(void) {
    kMemSet(&gs_stInterruptManager, 0, sizeof(gs_stInterruptManager));
}

// 인터럽트 처리 모드 설정
void kSetSymmetricIOMode(BOOL bSymmetricIOMode) {
    gs_stInterruptManager.bSymmetricIOMode = bSymmetricIOMode;
}

// 인터럽트 부하 분산 기능을 사용할지 여부를 설정
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing) {
    gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}

// 코어별 인터럽트 처리 횟수 증가
void kIncreaseInterruptCount(int iIRQ) {
    // 코어 인터럽트 카운트 증가
    gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}

// 현재 인터럽트 모드에 맞추어 EOI 전송
void kSendEOI(int iIRQ) {
    // 대칭 I/O 모드가 아니면 PIC 모드이므로, PIC 컨트롤러로 EOI 전송해야 함
    if(gs_stInterruptManager.bSymmetricIOMode == FALSE) {
        kSendEOIToPIC(iIRQ);
    }
    // 대칭 I/O 모드이면 로컬 APIC로 EOI를 전송해야 함
    else {
        kSendEOIToLocalAPIC();
    }
}

// 인터럽트 핸들러 자료구조 반환
INTERRUPTMANAGER *kGetInterruptManager(void) {
    return &gs_stInterruptManager;
}

// 인터럽트 부하 분산 처리
void kProcessLoadBalancing(int iIRQ) {
    QWORD qwMinCount = 0xFFFFFFFFFFFFFFFF;
    int iMinCountCoreIndex;
    int iCoreCount;
    int i;
    BOOL bResetCount = FALSE;
    BYTE bAPICID;

    bAPICID = kGetAPICID();

    // 부하 분산 기능이 꺼져 있거나, 부하 분산을 처리할 시점이 아니면 종료
    if((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0) ||
    ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % 
    INTERRUPT_LOADBALANCINGDIVIDOR) != 0) || 
    (gs_stInterruptManager.bUseLoadBalancing == FALSE)) {
        return;
    }

    // 코어 개수를 구해서 루프를 수행하며 인터럽트 처리 횟수가 가장 작은 코어 선택
    iMinCountCoreIndex = 0;
    iCoreCount = kGetProcessorCount();
    for(i = 0; i < iCoreCount; i++) {
        if((gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] < qwMinCount)) {
            qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];
            iMinCountCoreIndex = i;
        }

        // 전체 카운트가 거의 최댓값에 근접했다면 나중에 카운트를 모두 0으로 초기화
        else if(gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xFFFFFFFFFFFFFFFE) {
            bResetCount = TRUE;
        }
    }

    // I/O 리다이렉션 테이블을 변경하거나 가장 인터럽트 처리 횟수가 작은 로컬 APIC로 전달
    kRoutingIRQToAPICID(iIRQ, iMinCountCoreIndex);

    // 처리한 코어의 카운트가 최댓값에 근접했다면 0으로 초기화
    if(bResetCount == TRUE) {
        for(i = 0; i < iCoreCount; i++) {
            gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] = 0;
        }
    }
}

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = {0, };

    kPrintStringXY( 0, 0, "==================================================");
    kPrintStringXY( 0, 1, "                 Exception Occur~!!!!             ");
    kPrintStringXY( 0, 2, "              Vector:           Core ID:          ");
    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;
    kPrintStringXY( 27, 2, vcBuffer);
    kSPrintf(vcBuffer, "0x%X", kGetAPICID());
    kPrintStringXY( 40, 2, vcBuffer);
    kPrintStringXY( 0, 3, "==================================================");
    
    while(1);
}

void kCommonInterruptHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    // 인터럽트 벡터에서 IRQ 번호 추출
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    // EOI 전송
    kSendEOI(iIRQ);

    // 인터럽트 발생 횟수 업데이트
    kIncreaseInterruptCount(iIRQ);

    // 부하 분산 처리
    kProcessLoadBalancing(iIRQ);
}

void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    // 키보드 컨트롤러에서 데이터를 읽어서 아스키로 변환하여 큐에 삽입
    if(kIsOutputBufferFull() == TRUE) {
        // 마우스 데이터가 아니면 키 큐에 삽입
        if(kIsMouseDataInOutputBuffer() == FALSE) {
            // 출력 버퍼에서 키 스캔 코드를 읽는 용도의 함수지만 키보드와 마우스 데이터는
            // 출력 버퍼를 공통으로 사용하므로 마우스 데이터를 읽는데도 사용 가능
            bTemp = kGetKeyboardScanCode();

            // 키 큐에 삽입
            kConvertScanCodeAndPutQueue(bTemp);
        }
        else {
            bTemp = kGetKeyboardScanCode();
            kConvertScanCodeAndPutQueue(bTemp);
        }
    }
    // 마우스 데이터면 마우스 큐에 삽입
    else {
        bTemp = kGetKeyboardScanCode();

        // 마우스 큐에 삽입
        kAccumulateMouseDataAndPutQueue(bTemp);
    }
    
    // 인터럽트 벡터에서 IRQ 번호 추출
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    // EOI 전송
    kSendEOI(iIRQ);

    // 인터럽트 발생 횟수 업데이트
    kIncreaseInterruptCount(iIRQ);

    // 부하 분산 처리
    kProcessLoadBalancing(iIRQ);
}

void kTimerHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;
    int iIRQ;
    BYTE bCurrentAPICID;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iTimerInterruptCount;

    g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    // 인터럽트 벡터에서 IRQ 번호 추출
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    // EOI 전송
    kSendEOI(iIRQ);

    // 인터럽트 발생 횟수 업데이트
    kIncreaseInterruptCount(iIRQ);

    // IRQ 0 인터럽트 처리는 Bootstrap Processor만 처리
    bCurrentAPICID = kGetAPICID();
    if(bCurrentAPICID == 0) {
        // 타이머 발생 횟수 증가
        g_qwTickCount++;
    }


    // 태스크가 사용한 프로세서의 시간을 줄임
    kDecreaseProcessorTime(bCurrentAPICID);

    // 프로세서가 사용할 수 있는 시간을 다 썼다면 태스크 전환 수행
    if(kIsProcessorTimeExpired(bCurrentAPICID) == TRUE) {
        kScheduleInInterrupt();
    }
}

void kDeviceNotAvailableHandler(int iVectorNumber) {
    TCB *pstFPUTask, *pstCurrentTask;
    QWORD qwLastFPUTaskID;
    BYTE bCurrentAPICID;

    char vcBuffer[] = "[EXC: , ]";
    static int g_iFPUInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    // 현재 코어의 로컬 APIC ID 확인
    bCurrentAPICID = kGetAPICID();

    // CR0 컨트롤 레지스터의 TS 비트 0으로 설정
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인하여 FPU 상태 태스크에 저장
    qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
    pstCurrentTask = kGetRunningTask(bCurrentAPICID);

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

    kSetLastFPUUsedTaskID(bCurrentAPICID, pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iHDDInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;

    // 왼쪽 위에 있는 메시지와 겹치지 않도록 10, 0 출력
    kPrintStringXY(10, 0, vcBuffer);

    // 첫 번째 PATA 포트의 인터럽트 발생 여부를 TRUE 설정
    kSetHDDInterruptFlag(TRUE, TRUE);

    // 인터럽트 벡터에서 IRQ 번호 추출
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    // EOI 전송
    kSendEOI(iIRQ);

    // 인터럽트 발생 횟수 업데이트
    kIncreaseInterruptCount(iIRQ);

    // 부하 분산 처리
    kProcessLoadBalancing(iIRQ);
}

void kMouseHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iMouseInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iMouseInterruptCount;
    g_iMouseInterruptCount = (g_iMouseInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    // 출력 버퍼에 수신된 데이터가 있는지 여부를 확인하여 읽은 데이터를
    // 키 큐 또는 마우스 큐에 삽입
    if(kIsOutputBufferFull() == TRUE) {
        // 마우스 데이터가 아니면 키 큐에 삽입
        if(kIsMouseDataInOutputBuffer() == FALSE) {
            bTemp = kGetKeyboardScanCode();
            
            // 키 큐에 삽입
            kConvertScanCodeAndPutQueue(bTemp);
        }
        // 마우스 데이터면 마우스 큐에 삽입
        else {
            bTemp = kGetKeyboardScanCode();

            // 마우스 큐에 삽입
            kAccumulateMouseDataAndPutQueue(bTemp);
        }
    }

    // 인터럽트 벡터에서 IRQ 번호 추출
    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    // eoi 전송
    kSendEOI(iIRQ);

    // 인터럽트 발생 횟수를 업데이트
    kIncreaseInterruptCount(iIRQ);

    // 부하 분산 처리
    kProcessLoadBalancing(iIRQ);
}