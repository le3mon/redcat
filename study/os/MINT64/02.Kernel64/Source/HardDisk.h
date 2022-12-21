#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Synchronization.h"

// 매크로
// 첫 번째 PATA 포트와 두 번쨰 PATA 포트 정보
#define HDD_PORT_PRIMARYBASE            0X1F0
#define HDD_PORT_SECONDARYBASE          0X170

// 포트 인덱스에 관련된 매크로
#define HDD_PORT_INDEX_DATA             0X00
#define HDD_PORT_INDEX_SECTORCOUNT      0X02
#define HDD_PORT_INDEX_SECTORNUMBER     0X03
#define HDD_PORT_INDEX_CYLINDERLSB      0X04
#define HDD_PORT_INDEX_CYLINDERMSB      0X05
#define HDD_PORT_INDEX_DRIVEANDHEAD     0X06
#define HDD_PORT_INDEX_STATUS           0X07
#define HDD_PORT_INDEX_COMMAND          0X07
#define HDD_PORT_INDEX_DIGITALOUTPUT    0X206

// 커맨드 레지스터 관련 매크로
#define HDD_COMMAND_READ                0X20
#define HDD_COMMAND_WRITE               0X30
#define HDD_COMMAND_IDENTIFY            0XEC

// 상태 레지스터 관련 매크로
#define HDD_STATUS_ERROR                0X01
#define HDD_STATUS_INDEX                0X02
#define HDD_STATUS_CORRECTEDDATA        0X04
#define HDD_STATUS_DATAREQUEST          0X08
#define HDD_STATUS_SEEKCOMPLETE         0X10
#define HDD_STATUS_WRITEFAULT           0X20
#define HDD_STATUS_READY                0X40
#define HDD_STATUS_BUSY                 0X80

// 드라이브/헤드 레지스터 관련 매크로
#define HDD_DRIVEANDHEAD_LBA            0XE0
#define HDD_DRIVEANDHEAD_SLAVE          0X10

// 디지털 출력 레지스터 관련 매크로
#define HDD_DIGITALOUTPUT_RESET         0X04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT 0X01

// 하드 디스크 응답 대기 시간
#define HDD_WAITTIME                    500

// 한 번에 HDD를 읽거나 쓸 수 있는 섹터 수
#define HDD_MAXBULKSECTORCOUNT          256

// 구조체
#pragma pack(push, 1)

typedef struct kHDDInformationStruct{
    // 설정 값
    WORD wConfiguation;

    // 실린더 수
    WORD wNumberOfCylinder;
    WORD wReserved1;

    // 헤드 수
    WORD wNumberOfHead;
    WORD wUnformattedBytesPerTrack;
    WORD wUnformattedBytesPerSector;

    // 실린더당 섹터 수
    WORD wNumberOfSectorPerCylinder;
    WORD wInterSectorGap;
    WORD wBytesInPhaseLock;
    WORD wNumberOfVendorUniqueStatusWord;

    // 하드 디스크의 시리얼 넘버
    WORD vwSerialNumber[10];
    WORD wControllerType;
    WORD wBufferSize;
    WORD wNumberOfECCBytes;
    WORD vwFirmwareRevision[4];

    // 하드 디스크의 모델 번호
    WORD vwModelNumber[20];
    WORD vwReserved2[13];

    // 디스크의 총 섹터 수
    DWORD dwTotalSectors;
    WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

// 하드 디스크를 관리하는 구조체
typedef struct kHDDManagerStruct {
    // 하드 디스크를 관리하는 구조체
    BOOL bHDDDetected;
    BOOL bCanWrite;

    // 인터럽트 발생 여부와 동기화 객체
    volatile BOOL bPrimaryInterruptOccur;
    volatile BOOL bSecondaryInterruptOccur;
    MUTEX stMutex;

    // HDD 정보
    HDDINFORMATION stHDDInformation;
} HDDMANAGER;

// 함수
BOOL kInitializeHDD(void);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION *pstHDDInformation);
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char *pcBuffer);
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char *pcBuffer);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);

static void kSwapByteInWord(WORD *pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);
static BOOL kIsHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);

#endif