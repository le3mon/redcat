#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Task.h"

// 동적 메모리 영역의 시작 어드레스, 1MB 단위로 정렬
#define DYNAMICMEMORY_START_ADDRESS     ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)

// 버디 블록의 최소 크기, 1kb
#define DYNAMICMEMORY_MIN_SIZE  (1 * 1024)

// 비트맵 플래그
#define DYNAMICMEMORY_EXIST     0x01
#define DYNAMICMEMORY_EMPTY     0x00

// 비트맵 관리 자료구조
typedef struct kBitmapStruct {
    BYTE *pbBitmap;
    QWORD qwExistBitCount;
} BITMAP;

typedef struct kDynamicMemoryManagerStruct {
    // 블록 리스트 총 개수, 크기가 가장 작은 블록 수, 할당된 메모리 크기
    int iMaxLevelCount;
    int iBlockCountOfSmallestBlock;
    QWORD qwUsedSize;

    // 블록 풀이 시작 어드레스와 마지막 어드레스
    QWORD qwStartAddress;
    QWORD qwEndAddress;

    // 블록 리스트의 인덱스 어드레스, 비트맵 자료구조 어드레스
    BYTE *pbAllocatedBlockListIndex;
    BITMAP *pstBitmapOfLevel; 
} DYNAMICMEMORY;

void kInitializeDynamicMemory(void);
void *kAllocateMemory(QWORD qwSize);
BOOL kFreeMemory(void *pvAddress);
void kGetDynamicMemoryInformation(QWORD *pqwDynamicMemoryStartAddress, QWORD *pqwDynamicMemoryTotalSize, QWORD *pqwMetaDataSize, QWORD *pqwUsedMemorySize);
DYNAMICMEMORY *kGetDynamicMemoryManager(void);

static QWORD kCalculateDynamicMemorySize(void);
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
static int kAllocationBuddyBlock(QWORD qwAligendSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int kGetBlockListIndexOfMatchSize(QWORD qwAligendSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);

#endif