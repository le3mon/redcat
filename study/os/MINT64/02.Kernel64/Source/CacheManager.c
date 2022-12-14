#include "CacheManager.h"
#include "FileSystem.h"
#include "DynamicMemory.h"
#include "Utility.h"

// 파일 시스템 캐시 자료구조
static CACHEMANAGER gs_stCacheManager;

// 파일 시스템 캐시 초기화
BOOL kInitializeCacheManager(void) {
    int i;

    // 자료구조 초기화
    kMemSet(&gs_stCacheManager, 0, sizeof(gs_stCacheManager));

    // 접근 시간 초기화
    gs_stCacheManager.vdwAccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
    gs_stCacheManager.vdwAccessTime[CACHE_DATAAREA] = 0;

    // 캐시 버퍼의 최댓값 설정
    gs_stCacheManager.vdwMaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
    gs_stCacheManager.vdwMaxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;

    // 클러스터 링크 테이블 영역용 메모리 할당, 클러스터 링크 테이블은 512바이트로 관리
    gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] = (BYTE*)kAllocateMemory(CACHE_MAXCLUSTERLINKTABLEAREACOUNT * 512);
    if(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] == NULL) {
        return FALSE;
    }

    // 할당받은 메모리 영역을 나누어서 캐시 버퍼에 등록
    for(i = 0; i < CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++) {
        // 캐시 버퍼에 메모리 공간 할당
        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].pbBuffer = gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);

        // 태그를 유효하지 않은 것으로 설정하여 빈 것으로 만듬
        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].dwTag = CACHE_INVALIDTAG;
    }

    // 데이터 영역용 메모리 할당, 데이터 영역은 클러스터 단위로 관리
    gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] = (BYTE*)kAllocateMemory(CACHE_MAXDATAAREACOUNT * FILESYSTEM_CLUSTERSIZE);
    if(gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] == NULL) {
        kFreeMemory(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA]);
        return FALSE;
    }

    // 할당받은 메모리 영역을 나누어서 캐시 버퍼에 등록
    for(i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].pbBuffer = gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] + (i * FILESYSTEM_CLUSTERSIZE);

        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].dwTag = CACHE_INVALIDTAG;
    }

    return TRUE;
}

// 캐시 버퍼에서 빈 것을 찾아 반환
CACHEBUFFER *kAllocateCacheBuffer(int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    // 캐시 테이블 최대 개수를 넘어서면 실패
    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    // 접근 시간 필드가 최대값까지 가면 전체적으로 접근 시간을 낮춤
    kCutDownAccessTime(iCacheTableIndex);

    // 최대 개수만큼 검색하여 빈 것을 반환
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            // 임시로 캐시 태그를 설정하여 할당된 것으로 만듬
            pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG - 1;

            // 접근 시간 갱신
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

            return &(pstCacheBuffer[i]);
        }
    }

    return NULL;
}

// 캐시 버퍼에서 태그가 일치하는 것을 찾아 반환
CACHEBUFFER *kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    kCutDownAccessTime(iCacheTableIndex);

    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if(pstCacheBuffer[i].dwTag == dwTag) {
            // 접근 시간 갱신
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

            return &(pstCacheBuffer[i]);
        }
    }

    return NULL;
}

// 접근한 시간을 전체적으로 낮춤
static void kCutDownAccessTime(int iCacheTableIndex) {
    CACHEBUFFER stTemp;
    CACHEBUFFER *pstCacheBuffer;
    BOOL bSorted;
    int i, j;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return;
    }

    // 접근 시간이 아직 최대치를 넘지 않았다면 줄일 필요가 없음
    if(gs_stCacheManager.vdwAccessTime[iCacheTableIndex] < 0xfffffffe) {
        return;
    }

    // 캐시 버퍼를 오름차순으로 정렬, 버블 정렬 사용
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(j = 0; j < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1; j++) {
        // 기본은 정렬된 것으로 저장
        bSorted = TRUE;
        for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1 - j; i++) {
            // 인접한 두 데이터를 비교하여 큰 값을 오른쪽에 배치
            if(pstCacheBuffer[i].dwAccessTime > pstCacheBuffer[i + 1].dwAccessTime) {
                // 두 데이터를 교환하므로 정렬되지 않은 것으로 표시
                bSorted = FALSE;

                kMemCpy(&stTemp, &(pstCacheBuffer[i]), sizeof(CACHEBUFFER));
                kMemCpy(&(pstCacheBuffer[i]), &(pstCacheBuffer[i + 1]), sizeof(CACHEBUFFER));
                kMemCpy(&(pstCacheBuffer[i + 1]), &stTemp, sizeof(CACHEBUFFER));
            }
        }
        
        // 다 정렬되면 루프 빠져나옴
        if(bSorted == TRUE) {
            break;
        }
    }

    // 접근 시간을 0부터 순차적으로 설정하여 데이터 갱신
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        pstCacheBuffer[i].dwAccessTime = i;
    }

    // 접근 시간을 파일 시스템 캐시 자료구조에 저장해 다음부터는 변경된 값으로 접근 시간을 설정하게 함
    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = i;
}

// 캐시 버퍼에서 내보낼 것을 찾음
CACHEBUFFER *kGetVictimInCacheBuffer(int iCacheTableIndex) {
    DWORD dwOldTime;
    CACHEBUFFER *pstCacheBuffer;
    int iOldIndex;
    int i;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    // 접근 시간을 최대로 해서 접근 시간이 가장 오래된 캐시 버퍼를 검색
    iOldIndex = -1;
    dwOldTime = 0xFFFFFFFF;

    // 캐시 버퍼에서 가장 마지막에 사용된 것을 찾아 반환
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        // 빈 캐시 버퍼가 있으면 빈 것을 반환
        if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            iOldIndex = i;
            break;
        }

        // 접근 시간이 현재 값보다 오래되었으면 저장
        if(pstCacheBuffer[i].dwAccessTime < dwOldTime) {
            dwOldTime = pstCacheBuffer[i].dwAccessTime;
            iOldIndex = i;
        }
    }

    // 캐시 버퍼를 찾지 못하면 문제 발생한 것
    if(iOldIndex == -1) {
        kPrintf("Cache Buffer Find Error\n");
        return NULL;
    }

    // 선택된 캐시 버퍼의 접근 시간을 갱신
    pstCacheBuffer[iOldIndex].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;
    
    return &(pstCacheBuffer[iOldIndex]);
}

// 캐시 버퍼의 내용을 모두 비움
void kDiscardAllCacheBuffer(int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    // 캐시 버퍼를 모두 빈 것으로 설정
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG;
    }

    // 접근 시간 초기화
    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = 0;
}

// 캐시 버퍼의 포인터와 최대 개수 반환
BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER **ppstCacheBuffer, int *piMaxCount) {
    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
        return FALSE;
    }

    *ppstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    *piMaxCount = gs_stCacheManager.vdwMaxCount[iCacheTableIndex];
    return TRUE;
}