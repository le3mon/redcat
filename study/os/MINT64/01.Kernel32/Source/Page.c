#include "Page.h"

void kInitializePageTables(void) {
    PML4TENTRY* pstPML4TEntry;
    PDPTENTRY* pstPDPTEntry;
    PDENTRY* pstPDEntry;
    DWORD dwMappingAddress;
    int i;
    // PML4 테이블 생성
    // 첫 번째 엔트리 외에 나머지는 모두 0으로 초기화
    pstPML4TEntry = ( PML4TENTRY* ) 0x100000;
    kSetPageEntryData( &( pstPML4TEntry[ 0 ] ), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0 );
    for( i = 1 ; i < PAGE_MAXENTRYCOUNT ; i++ )
        kSetPageEntryData( &( pstPML4TEntry[ i ] ), 0, 0, 0, 0 );

    // 페이지 디렉터리 포인터 테이블 생성
    // 64개 엔트리를 설정하여 64GB까지 매핑
    pstPDPTEntry = ( PDPTENTRY* ) 0x101000;
    for( i = 0 ; i < 64 ; i++ )
        kSetPageEntryData( &( pstPDPTEntry[ i ] ), 0, 0x102000 + ( i * PAGE_TABLESIZE ), PAGE_FLAGS_DEFAULT, 0 );
    
    for( i = 64 ; i < PAGE_MAXENTRYCOUNT ; i++ )
        kSetPageEntryData( &( pstPDPTEntry[ i ] ), 0, 0, 0, 0 );
    
    // 페이지 디렉터리 테이블 생성
    // 하나의 페이지 디렉터리가 1GB까지 매핑 가능
    // 총 페이지 디렉터리의 엔트리 수는 64*512이 됨
    pstPDEntry = ( PDENTRY* ) 0x102000;
    dwMappingAddress = 0;
    for( i = 0 ; i < PAGE_MAXENTRYCOUNT * 64 ; i++ ) {
        // 32비트로 상위 어드레스를 표현할 수 없으므로, MB 단위로 계산한 후 최종 결과를 다시 4KB로 나누어 32비트 이상의 어드레스 계산
        kSetPageEntryData( &( pstPDEntry[ i ] ), ( i * ( PAGE_DEFAULTSIZE >> 20 ) ) >> 12, dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0 );
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = ( dwUpperBaseAddress & 0xFF ) | dwUpperFlags;
}
