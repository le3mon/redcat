#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT  300
#define CONSOLESHELL_PROMPTMESSAGE          "MINT64>"


// // 한 섹터 바잍 ㅡ수
// #define BYTESOFESECTOR      512

// 파캐지 시그니처
#define PACKAGESINGNATURE   "MINT64OSPACKAGE"

// 파일 이름 최대 길이
#define MAXFILENAMELENGTH   24

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void (*CommandFunction)(const char *pcParameter);

#pragma pack(push, 1)
// 셸의 커맨드를 저장하는 자료구조
typedef struct kShellCommandEntryStruct {
    // 커맨드 문자열
    char *pcCommand;

    // 커맨드의 도움말
    char *pcHelp;

    // 커맨드를 수행하는 함수의 포인터
    CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

// 파라미터를 처리하기 위해 정보를 저장하는 자료구조
typedef struct kParameterListStruct {
    // 파라미터 버퍼의 어드레스
    const char *pcBuffer;

    // 파라미터의 길이
    int iLength;

    // 현재 처리할 파라미터가 시작하는 위치
    int iCurrentPosition;
} PARAMETERLIST;

// 패키지 헤더 내부 각 파일 정보를 구성하는 자료구조
typedef struct PackageItemStruct {
    // 파일 이름
    char vcFileName[MAXFILENAMELENGTH];

    // 파일 크기
    DWORD dwFileLength;
} PACKAGEITEM;


// 패키지 헤더 자료구조
typedef struct PackageHeaderStrcut {
    // MINT64 OS 패키지 파일 시그니처
    char vcSignature[16];

    // 패키지 헤더 전체 크기
    DWORD dwHeaderSize;

    // 패키지 아이템 시작 위치
    PACKAGEITEM vstItem[0];
} PACKAGEHEADER;

#pragma pack(pop)

void kStartConsoleShell(void);
void kExecuteCommand(const char *pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter);
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter);

static void kHelp(const char *pcParameterBuffer);
static void kCls(const char *pcParameterBuffer);
static void kShowTotalRAMSize(const char *pcParameterBuffer);
static void kStringToDecimalHexTest(const char *pcParameterBuffer);
static void kShutdown(const char *pcParameterBuffer);

static void kSetTimer(const char *pcParameterBuffer);
static void kWaitUsingPIT(const char *pcParameterBuffer);
static void kReadTimeStampCounter(const char *pcParameterBuffer);
static void kMeasureProcessorSpeed(const char *pcParameterBuffer);
static void kShowDateAndTime(const char *pcParameterBuffer);

static void kCreateTestTask(const char *pcParameterBuffer);

static void kChangeTaskPriority(const char *pcParameterBuffer);
static void kShowTaskList(const char *pcParameterBuffer);
static void kKillTask(const char *pcParameterBuffer);
static void kCPULoad(const char *pcParameterBuffer);
static void kTestMutex(const char *pcParameterBuffer);

static void kCreateThreadTask(void);
static void kTestThread(const char *pcParameterBuffer);
static void kShowMatrix(const char *pcParameterBuffer);

static void kTestPIE(const char *pcParameterBuffer);

static void kShowDynamicMemoryInformation(const char *pcParameterBuffer);
static void kTestSequentialAllocation(const char *pcParameterBuffer);
static void kTestRandomAllocation(const char *pcParameterBuffer);
static void kRandomAllocationTask(void);

static void kShowHDDInformation(const char *pcParameterBuffer);
static void kReadSector(const char *pcParameterBuffer);
static void kWriteSector(const char *pcParameterBuffer);

static void kMountHDD(const char *pcParameterBuffer);
static void kFormatHDD(const char *pcParameterBuffer);
static void kShowFileSystemInformation(const char *pcParameterBuffer);
static void kCreateFileInRootDirectory(const char *pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char *pcParameterBuffer);
static void kShowRootDirectory(const char *pcParameterBuffer);

static void kWriteDataToFile(const char *pcParameterBuffer);
static void kReadDataFromFile(const char *pcParameterBuffer);
static void kTestFileIO(const char *pcParameterBuffer);

static void kFlushCache(const char *pcParameterBuffer);
static void kTestPerformance(const char *pcParameterBuffer);

static void kDownloadFile(const char *pcParameterBuffer);

static void kShowMPConfigurationTable(const char *pcParameterBuffer);

static void kStartApplicationProcessor(const char *pcParameterBuffer);

static void kStartSymmetricIOMode(const char *pcParameterBuffer);
static void kShowIRQINTINMappingTable(const char *pcParameterBuffer);

static void kShowInterruptProcessingCount(const char *pcParameterBuffer);
static void kStartInterruptLoadBalancing(const char *pcParameterBuffer);

static void kStartTaskLoadBalancing(const char *pcParameterBuffer);
static void kChangeTaskAffinity(const char *pcParameterBuffer);

static void kShowVBEModeInfo(const char *pcParameterBuffer);

static void kTestSystemCall(const char *pcParameterBuffer);

static void kExecuteApplicationProgram(const char *pcParameterBuffer);

static void kInstallPackage(const char *pcParameterBuffer);
#endif