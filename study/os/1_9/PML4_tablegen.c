#include "Table.h"

void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}

void kInitializePageTables(void) {
    PML4ENTRY *pstPML4Entry;
    int i;

    pstPML4Entry = (PML4ENTRY*) 0x100000;
    kSetPageEntryData(&(pstPML4Entry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
    for(i = 1; i < 512, i++)
        kSetPageEntryData(&(pstPML4Entry[i]), 0, 0, 0, 0);
}