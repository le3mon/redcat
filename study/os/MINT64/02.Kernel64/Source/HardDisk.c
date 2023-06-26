#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Utility.h"

// 하드 디스크를 관리하는 자료구조
static HDDMANAGER gs_stHDDManager;

// 하드 디스크 디바이스 드라이버를 초기화
BOOL kInitializeHDD(void) {
    // 뮤텍스 초기화
    kInitializeMutex(&(gs_stHDDManager.stMutex));

    // 인터럽트 플래그 초기화
    gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
    gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

    // 첫 번째와 두 번쨰 PATA 포트의 디지털 출력 레지스터에 0을 출력하여
    // 하드 디스크 컨트롤러의 인터럽트 활성화
    kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

    // 하드 디스크 정보 요청
    if(kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation)) == FALSE) {
        gs_stHDDManager.bHDDDetected = FALSE;
        gs_stHDDManager.bCanWrite = FALSE;
        return FALSE;
    }

    // 하드 디스크 검색 시 QEMU에서만 쓸 수 있도록 설정
    gs_stHDDManager.bHDDDetected = TRUE;
    if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0) {
        gs_stHDDManager.bCanWrite = TRUE;
    }
    else {
        gs_stHDDManager.bCanWrite = FALSE;
    }

    return TRUE;
}

// 하드 디스크 상태 반환
static BYTE kReadHDDStatus(BOOL bPrimary) {
    if(bPrimary == TRUE) {
        // 첫 번쨰 PATA 포트 상태 레지스터에서 값 반환
        return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
    }
    // 두 번쨰 PATA 포트 상태 레지스터에서 값 반환
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

// 하드 디스크의 Busy 상태가 해제될 때까지 일정시간 대기
static BOOL kWaitForHDDNoBusy(BOOL bPrimary) {
    QWORD qwStartTickCount;
    BYTE bStatus;

    // 대기를 시작한 시간 저장
    qwStartTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크의 busy가 해제될 때까지 대기
    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
        // HDD 상태 반환
        bStatus = kReadHDDStatus(bPrimary);

        // busy 상태 체크
        if((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) {
            return TRUE;
        }
        kSleep(1);
    }

    return FALSE;
}

// 하드 디스크가 ready될 때까지 대기
static BOOL kWaitForHDDReady(BOOL bPrimary) {
    QWORD qwStartTickCount;
    BYTE bStatus;

    qwStartTickCount = kGetTickCount();

    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
        bStatus = kReadHDDStatus(bPrimary);

        if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY) {
            return TRUE;
        }
        kSleep(1);
    }

    return FALSE;
}

// 인터럽트 발생 여부 설정
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag) {
    if(bPrimary == TRUE) {
        gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
    }
    else {
        gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
    }
}

// 인터럽트 발생 까지 대기
static BOOL kWaitForHDDInterrupt(BOOL bPrimary) {
    QWORD qwTickCount;

    qwTickCount = kGetTickCount();

    while((kGetTickCount() - qwTickCount) <= HDD_WAITTIME) {
        if((bPrimary == TRUE) && (gs_stHDDManager.bPrimaryInterruptOccur == TRUE)) {
            return TRUE;
        }
        else if((bPrimary == TRUE) && (gs_stHDDManager.bSecondaryInterruptOccur == TRUE)) {
            return TRUE;
        }
    }

    return FALSE;
}

// 하드 디스크의 정보 읽어옴
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION *pstHDDInformation) {
    WORD wPortBase;
    QWORD qwLastTickCount;
    BYTE bStatus;
    BYTE bDriveFlag;
    int i;
    WORD wTemp;
    BOOL bWaitResult;

    // PATA 포트에 따라 I/O 포트의 기본 어드레스 설정
    if(bPrimary == TRUE) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 동기화 처리
    kLock(&(gs_stHDDManager.stMutex));

    // 아직 수행 중인 커맨드가 있다면 일정 시간 대기
    if(kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // LBA 어드레스와 드라이브 및 헤드에 관련된 레지스터 설정. 드라이브와 헤드 정보만 있으면 됨
    // 드라이브와 헤드 데이터 설정
    if(bMaster == TRUE) {
        // 마스터면 LBA만 설정
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        // 슬레이브면 슬레이브 플래그도 추가로 설정
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    // 드라이브/헤드 헤지스터에 설정된 값 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

    // 커맨드 전송 후 인터럽트 대기
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // 인터럽트 플래그 초기화
    kSetHDDInterruptFlag(bPrimary, FALSE);

    // 커맨드 레지스터에 드라이브 인식 커맨드 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

    // 처리 완료될 때까지 인터럽트 발생 대기
    bWaitResult = kWaitForHDDInterrupt(bPrimary);
    
    // 에러가 발생하거나 인터럽트 발생하지 않음 -> 문제가 발생한 것이므로 종료
    bStatus = kReadHDDStatus(bPrimary);
    if((bWaitResult == FALSE) || ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // 데이터 수신
    for(i = 0; i < 512 /2; i++) {
        ((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }

    // 문자열은 바이트 순서로 다시 변환
    kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber) / 2);
    kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber) / 2);

    kUnlock(&(gs_stHDDManager.stMutex));
    return TRUE;
}

// word 내의 바이트 순서 바꿈
static void kSwapByteInWord(WORD *pwData, int iWordCount) {
    int i;
    WORD wTemp;

    for(i = 0; i < iWordCount; i++) {
        wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}

// 하드 디스크의 섹터를 읽음
// 최대 256 섹터 읽기 가능, 읽을 섹터 수 반환
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char *pcBuffer) {
    WORD wPortBase;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    // 범위 검사
    if((gs_stHDDManager.bHDDDetected == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || 
    ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)) {
        return 0;
    }

    // PATA 포트에 따라 I/O 포트기본 어드레스 설정
    if(bPrimary == TRUE) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&(gs_stHDDManager.stMutex));

    if(kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // 데이터 레지스터 설정
    // 섹터 수 레지스터에 일긍ㄹ 섹터 수 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    
    // 섹터 번호 레지스터에 읽을 섹터 위치(LBA 0~7 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터에 읽을 섹터 위치(LBA 8~15 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터에 읽을 섹터 위치(LBA 8~15 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

    // 드라이브와 헤드 데이터 설정
    if(bMaster == TRUE) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    // 드라이브/헤드 레지스터에 읽을 섹터의 위치와 설정된 값 같이 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

    // 커맨드 전송
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // 인터럽트 플래그 초기화
    kSetHDDInterruptFlag(bPrimary, FALSE);

    // 커맨드 레지스터에 읽기 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

    // 인터럽트 대기 후 데이터 수신
    for(i = 0; i < iSectorCount; i++) {
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("Error Occur\n");
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터 수신 대기
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            // 처리가 완료될 때까지 일정 시간 동안 인터럽트 대기
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            // 인터럽트가 발생하지 않으면 문제 발생한 것이므로 종료
            if(bWaitResult == FALSE) {
                // kPrintf("Interrupt Not Occur\n");
                // kUnlock(&(gs_stHDDManager.stMutex));
                // return FALSE;
            }
        }

        for(j = 0; j < 512 / 2; j++) {
            ((WORD*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
        }
    }

    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}

// 하드 디스크에 섹터를 씀
// 최대 256개의 섹터를 쓸 수 있음, 실제로 쓴 섹터 수 반환
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char *pcBuffer) {
    WORD wPortBase;
    WORD wTemp;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    if((gs_stHDDManager.bCanWrite == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || 
    ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)) {
        return 0;
    }

    // PATA 포트에 따라 I/O 포트의 기본 어드레스 설정
    if(bPrimary == TRUE) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    if(kWaitForHDDNoBusy(bPrimary) == FALSE) {
        return FALSE;
    }

    kLock(&(gs_stHDDManager.stMutex));
    
    // 데이터 레지스터 설정
    // 섹터 수 레지스터에 쓸 섹터 수 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    
    // 섹터 번호 레지스터에 쓸 섹터 위치(LBA 0~7 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터에 쓸 섹터 위치(LBA 8~15 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터에 쓸 섹터 위치(LBA 8~15 비트) 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

    if(bMaster == TRUE) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    // 드라이브/헤드 레지스터에 쓸 섹터의 위치와 설정된 값 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

    // 커맨드 전송 후 데이터 송신까지 대기
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // 커맨드 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

    while(1) {
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock(&(gs_stHDDManager.stMutex));
            return 0;
        }

        // Data Request 비트가 설정되면 데이터 송신 가능
        if((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST) {
            break;
        }

        kSleep(1);
    }

    // 데이터 송신 후 인터럽트 대기
    for(i = 0; i < iSectorCount; i++) {
        kSetHDDInterruptFlag(bPrimary, FALSE);
        for(j = 0; j < 512 / 2; j++) {
            kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lReadCount++]);
        }

        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않을 시 대기
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            // 처리 완료 까지 인터럽트 대기
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);

            // 인터럽트가 발생하지 않다면 문제 발생한 것이므로 종료
            if(bWaitResult == FALSE) {
                kUnlock(&(gs_stHDDManager.stMutex));
                return FALSE;
            }
        }
    }
    
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}