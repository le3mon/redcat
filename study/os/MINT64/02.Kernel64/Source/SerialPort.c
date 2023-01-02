#include "SerialPort.h"
#include "Utility.h"
#include "AssemblyUtility.h"

// 시러얼 포트 담당 자료구조
static SERIALMANAGER gs_stSerialManager;

// 시리얼 포트 초기화
void kInitializeSerialPort(void) {
    WORD wPortBaseAddress;

    // 뮤텍스 초기화
    kInitializeMutex(&(gs_stSerialManager.stLock));

    // COM1 시리얼 포트를 선택하여 초기화
    wPortBaseAddress = SERIAL_PORT_COM1;

    // 인터럽트 활성화 레지스터에 9을 전송하여 모든 인러터럽트 비활성화
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);

    // 통신 속도를 115200으로 설정
    // 라인 제어 레지스터의 DLAB 비트를 1로 설정하여 제수 래치 레지스터 접근
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_DLAB);

    // LSB 제수 래치 레지스터에 제수의 하위 8비트 전송
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHLSB, SERIAL_DIVISORLATCH_115200);

    // MSB 제수 래치 레지스터에 제수의 상위 8비트 전송
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHMSB, SERIAL_DIVISORLATCH_115200 >> 8);

    // 송수신 방법 설정
    // 라인 제어 레지스터에 통신 방법을 8비트, 패러티 없음, 1 Stop 비트로 설정, DLAB 비트 0으로 설정
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY | SERIAL_LINECONTROL_1BITSTOP);

    // FIFO 인터럽트 발생 시점을 14바이트로 설정
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

// 송신 FIFO가 비어 있는지 반환
static BOOL kIsSerialTransmitterBufferEmpty(void) {
    BYTE bData;

    // 라인 상태 레지스터를 읽은 뒤 TBE 비트를 확인하여 송신 FIFO가 비어있는지 확인
    bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
    if((bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) {
        return TRUE;
    }

    return FALSE;
}

// 시리얼 포트로 데이터 송신
void kSendSerialData(BYTE *pbBuffer, int iSize) {
    int iSentByte;
    int iTempSize;
    int j;

    // 동기화
    kLock(&(gs_stSerialManager.stLock));

    // 요청한 바이트 수만큼 보낼 때까지 반복
    iSentByte = 0;
    while(iSentByte < iSize) {
        // 송신 FIFO에 데이터가 남아있다면 다 전송될 때까지 대기
        while(kIsSerialTransmitterBufferEmpty() == FALSE) {
            kSleep(0);
        }

        // 전송할 데이터 중에서 남은 크기와 FIFO의 최대 크기를 비교한 후 작은 것을 선택하여 채움
        iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);
        for(j = 0; j < iTempSize; j++) {
            kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER, pbBuffer[iSentByte + j]);
        }
        iSentByte += iTempSize;
    }

    kUnlock(&(gs_stSerialManager.stLock));
}

// 수신 FIFO에 데이터가 있는지 반환
static BOOL kIsSerialReceiveBufferFull(void) {
    BYTE bData;

    // 라인 상태 레지스터를 읽은 뒤 RxRD 비트를 확인하여 수신 FIFO에 데이터가 있는지 확인
    bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
    if((bData & SERIAL_LINESTATUS_RECEIVEDDATAREADY) == SERIAL_LINESTATUS_RECEIVEDDATAREADY) {
        return TRUE;
    }

    return FALSE;
}

// 시리얼 포트에서 데이터를 읽음
int kReceiveSerialData(BYTE *pbBuffer, int iSize) {
    int i;

    kLock(&(gs_stSerialManager.stLock));

    for(i = 0; i < iSize; i++) {
        // 버퍼에 데이터가 없으면 중지
        if(kIsSerialReceiveBufferFull() == FALSE) {
            break;
        }

        // 수신 버퍼 레지스터에서 한 바이트 읽음
        pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_RECEIVEBUFFER);
    }

    kUnlock(&(gs_stSerialManager.stLock));

    return i;
}

// 시리얼 포트 컨트롤러의 FIFO 초기화
void kClearSerialFIFO(void) {
    kLock(&(gs_stSerialManager.stLock));

    // 송수신 FIFO를 모두 비우고 버퍼에 데이터가 14바이츠 찼을 때 인터럽트가 발생하도록
    // FIFO 제어 레지스터에 설정 값 전송
    kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO | SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);

    kUnlock(&(gs_stSerialManager.stLock));
}