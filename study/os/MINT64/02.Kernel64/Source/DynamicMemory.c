#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"

static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory(void) {
    QWORD qwDynamicMemorySize;
    int i, j;
    BYTE *pbCurrentBitmapPosition;
    int iBlockCountOfLevel, iMetaBlockCount;

    // 동적 메모리 영역으로 사용할 메모리 크기를 이용하여 블록을 관리하는데
    // 필요한 메모리 크기를 최소 블록 단위로 계산
    qwDynamicMemorySize = kCalculateDynamicMemorySize();
    iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

    // 전체 블록 개수에서 관리에 필요한 메타블록의 개수를 제외한 나머지 영역에
    // 대해서 메타 정보를 구성
    gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

    // 최대 몇 개의 블록 리스트로 구성되는지를 계산
    for(i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++) {
        // nothing
        ;
    }
    gs_stDynamicMemory.iMaxLevelCount = i;

    // 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역 초기화
    gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
    for(i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++) {
        gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
    }

    
}