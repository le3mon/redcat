#ifndef __TABLE_H__
#define __TABLE_H__

// 하위 32 비트용 속성 필드
#define PAGE_FLAGS_P 0x00000001 // Present
#define PAGE_FLAGS_RW 0x00000002 // Read/Write
#define PAGE_FLAGS_US 0x00000004 // User/Supervisor(플래그 설정 시 유저 레벨)
#define PAGE_FLAGS_PWT 0x00000008 // Page Level Write-through
#define PAGE_FLAGS_PCD 0x00000010 // Page Level Cache Disable
#define PAGE_FLAGS_A 0x00000020 // Accessed

#define PAGE_FLAGS_D 0x00000040 // Dirty
#define PAGE_FLAGS_PS 0x00000080 // Page Size
#define PAGE_FLAGS_G 0x00000100 // Global
#define PAGE_FLAGS_PAT 0x00001000 // Page Attribute Table Index

// 상위 32 비트용 속성 필드
#define PAGE_FLAGS_EXB 0x80000000 // Execute Disable 비트

#define PAGE_FLAGS_DEFAULT ( PAGE_FLAGS_P | PAGE_FLAGS_RW ) // 편의를 위해 정의

#define DWORD   unsigned int

typedef struct pageTableEntryStruct {
	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4ENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#endif /*__TABLE_H__*/