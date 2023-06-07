#include "Loader.h"
#include "FileSystem.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Console.h"

// 응용프로그램 실행
QWORD kExecuteProgram(const char *pcFileName, const char *pcArgumentString,
    BYTE bAffinity) {
    
    DIR *pstDirectory;
    struct dirent *pstEntry;
    DWORD dwFileSize;
    BYTE *pbTempFileBuffer;
    FILE *pstFile;
    DWORD dwReadSize;
    QWORD qwEntryPointAddress;
    QWORD qwApplicationMemory;
    QWORD qwMemorySize;
    TCB *pstTask;

    // 루트 디렉터리 열어서 파일 검색
    pstDirectory = opendir("/");
    dwFileSize = 0;

    // 디렉터리에서 파일 검색
    while(1) {
        // 디렉터리에서 엔트리 하나 읽음
        pstEntry = readdir(pstDirectory);

        // 더이상 파일이 없으면 나감
        if(pstEntry == NULL) {
            break;
        }

        // 파일 이름 길이와 내용이 같은 것을 검색
        if((kStrLen(pstEntry->vcFileName) == kStrLen(pcFileName)) &&
            (kMemCmp(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName))) == 0) {
            dwFileSize = pstEntry->dwFileSize;
            break;
        }
    }

    // 디렉터리 핸들을 반환, 핸들을 반환하지 않으면 메모리가 해제되지 않고 남으므로 꼭 해제
    closedir(pstDirectory);

    if(dwFileSize == 0) {
        kPrintf("%s file doesn't exist or size is zero\n", pcFileName);
        return TASK_INVALIDID;
    }

    // 파일 전체를 저장할 수 있는 임시 버퍼를 할당 받아서 파일 내용 모두 저장
    // 메모리 할당
    pbTempFileBuffer = (BYTE*)kAllocateMemory(dwFileSize);
    if(pbTempFileBuffer == NULL) {
        kPrintf("Memory %d byte allocate fail\n", dwFileSize);
        return TASK_INVALIDID;
    }

    // 파일을 열어 모두 읽은 후 메모리에 저장
    pstFile = fopen(pcFileName, "r");

    if((pstFile != NULL) && 
        (fread(pbTempFileBuffer, 1, dwFileSize, pstFile) == dwFileSize)) {
        
        fclose(pstFile);
        kPrintf("%s file read success\n", pcFileName);
    }
    else {
        kPrintf("%s file read fail\n", pcFileName);
        kFreeMemory(pbTempFileBuffer);
        fclose(pstFile);
        return TASK_INVALIDID;
    }

    // 파일의 내용을 분석하여 섹션을 로딩하고 재배치 수행
    if(kLoadProgramAndRelocation(pbTempFileBuffer, &qwApplicationMemory,
        &qwMemorySize, &qwEntryPointAddress) == FALSE) {
        
        kPrintf("%s file is invalid application file or loading fail\n", pcFileName);
        kFreeMemory(pbTempFileBuffer);
        return TASK_INVALIDID;
    }

    // 메모리 해제
    kFreeMemory(pbTempFileBuffer);

    // 태스크 생성 및 스택 인자 문자열 저장
    // 유저 레벨 응용프로그램 태스크 생성
    pstTask = kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_PROCESS,
        (void*)qwApplicationMemory, qwMemorySize, qwEntryPointAddress, bAffinity);
    if(pstTask == NULL) {
        kFreeMemory((void*)qwApplicationMemory);
        return TASK_INVALIDID;
    }

    // 인자 문자열 저장
    kAddArgumentStringToTask(pstTask, pcArgumentString);

    return pstTask->stLink.qwID;
}

// 응용프로그램 섹션 로딩 및 재배치 수행
static BOOL kLoadProgramAndRelocation(BYTE *pbFileBuffer,
    QWORD *pqwApplicationMemoryAddress, QWORD *pqwApplicationMemorySize,
    QWORD *pqwEntryPointAddress) {
    

    Elf64_Ehdr *pstELFHeader;
    Elf64_Shdr *pstSectionHeader;
    Elf64_Shdr *pstSectionNameTableHeader;
    Elf64_Xword qwLastSectionSize;
    Elf64_Addr qwLastSectionAddress;
    int i;
    QWORD qwMemorySize;
    QWORD qwStackAddress;
    BYTE *pbLoadedAddress;

    // ELF 헤더 정보 출력 및 분석에 필요한 정보 저장
    pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
    pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);
    pstSectionNameTableHeader = pstSectionHeader + pstELFHeader->e_shstrndx;

    kPrintf("====================== ELF Header Info ======================\n");
    kPrintf("Magic Number [%c%c%c] Section Header Count [%d]\n",
        pstELFHeader->e_ident[1], pstELFHeader->e_ident[2],
        pstELFHeader->e_ident[3], pstELFHeader->e_shnum);
    kPrintf("File Type [%d]\n", pstELFHeader->e_type);
    kPrintf("Section Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_shoff,
        pstELFHeader->e_shentsize);
    kPrintf("Program Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_phoff,
        pstELFHeader->e_phentsize);
    kPrintf("Section Name String Table Section Index [%d]\n", pstELFHeader->e_shstrndx);

    // ELF의 ID와 클래스, 인코딩, 타입을 확인하여 올바른 응용프로그램인지 확인
    if((pstELFHeader->e_ident[EI_MAG0] != ELFMAG0) ||
        (pstELFHeader->e_ident[EI_MAG1] != ELFMAG1) ||
        (pstELFHeader->e_ident[EI_MAG2] != ELFMAG2) ||
        (pstELFHeader->e_ident[EI_MAG3] != ELFMAG3) ||
        (pstELFHeader->e_ident[EI_CLASS] != ELFCLASS64) ||
        (pstELFHeader->e_ident[EI_DATA] != ELFDATA2LSB) ||
        (pstELFHeader->e_type != ET_REL)) {
        return FALSE;
    }

    // 모든 섹션 헤더의 로딩할 메모리 어드레스를 확인하여 가장 마지막에 있는 섹션을 찾음
    // 섹션 정보도 같이 표시
    qwLastSectionAddress = 0;
    qwLastSectionSize = 0;
    for(i = 0; i < pstELFHeader->e_shnum; i++) {
        // 가장 마지막 센션 확인 <- 이 값으로 프로그램이 사용할 전체 메모리 확인 가능
        if((pstSectionHeader[i].sh_flags & SHF_ALLOC) &&
            (pstSectionHeader[i].sh_addr >= qwLastSectionAddress)) {
            qwLastSectionAddress = pstSectionHeader[i].sh_addr;
            qwLastSectionSize = pstSectionHeader[i].sh_size;
        }
    }

    kPrintf("\n===================== Load & Relocaion ====================\n");
    // 마지막 섹션 위치와 크기 표시
    kPrintf("Last Section Address [0x%q] Size [0x%q]\n", qwLastSectionAddress, qwLastSectionSize);

    // 마지막 센셔의 위치로 최대 메모리 량을 계산, 4kb 단위로 정렬
    qwMemorySize = (qwLastSectionAddress + qwLastSectionSize + 0x1000 - 1) & 0xfffffffffffff000;
    kPrintf("Aligned Memory Size [0x%q]\n", qwMemorySize);

    // 응용프로그램에서 사용할 메모리 할당
    pbLoadedAddress = (char*)kAllocateMemory(qwMemorySize);
    if(pbLoadedAddress == NULL) {
        kPrintf("Memory allocate fail\n");
        return FALSE;
    }
    else {
        kPrintf("Loaded Address [0x%q]\n", pbLoadedAddress);
    }

    // 파일 내용을 메모리에 복사
    for(i = 1; i < pstELFHeader->e_shnum; i++) {
        // 메모리에 올릴 필요가 없는 섹션이나 크기가 0인 섹션이면 복사할 필요 없음
        if(!(pstSectionHeader->sh_flags & SHF_ALLOC) || (pstSectionHeader[i].sh_size == 0)) {
            continue;
        }

        // 섹션 헤더에 로딩할 어드레스 적용
        pstSectionHeader[i].sh_addr += (Elf64_Addr)pbLoadedAddress;

        // .bss와 같이 SHT_NOBITS가 설정된 섹션은 파일에 데이터가 없으므로 0으로 초기화
        if(pstSectionHeader[i].sh_type == SHT_NOBITS) {
            // 응용프로그램에게 할당된 메모리를 0으로 설정
            kMemSet(pstSectionHeader[i].sh_addr, 0, pstSectionHeader->sh_size);
        }
        else {
            // 파일 버퍼의 내용을 응용프로그램에게 할당된 메모리로 복사
            kMemCpy(pstSectionHeader[i].sh_addr,
                pbFileBuffer + pstSectionHeader[i].sh_offset,
                pstSectionHeader[i].sh_size);
        }
        kPrintf("Section [%x] Virtual Address [%q] File Address [%q] Size [%q]\n",
            i, pstSectionHeader[i].sh_addr,
            pbFileBuffer + pstSectionHeader[i].sh_offset, pstSectionHeader[i].sh_size);
    }
    kPrintf("Program load success\n");

    // 재배치 진행
    if(kRelocation(pbFileBuffer) == FALSE) {
        kPrintf("Relocation fail\n");
        return FALSE;
    }
    else {
        kPrintf("Relocation sucess\n");
    }

    // 응용프로그램의 어드레스와 엔트리 포인트의 어드레스 반환
    *pqwApplicationMemoryAddress = (QWORD)pbLoadedAddress;
    *pqwApplicationMemorySize = qwMemorySize;
    *pqwEntryPointAddress = pstELFHeader->e_entry + (QWORD)pbLoadedAddress;

    return TRUE;
}

// 재배치 수행
// 섹션 헤더에는 메모리 어드레스가 할당되어 있어야 함
static BOOL kRelocation(BYTE *pbFileBuffer) {
    Elf64_Ehdr *pstELFHeader;
    Elf64_Shdr *pstSectionHeader;
    int i, j;
    int iSymbolTableIndex;
    int iSectionIndexInSymbol;
    int iSectionIndexToRelocation;
    Elf64_Addr ulOffset;
    Elf64_Xword ulInfo;
    Elf64_Sxword lAddend;
    Elf64_Sxword lResult;
    int iNumberOfBytes;
    Elf64_Rel *pstRel;
    Elf64_Rela *pstRela;
    Elf64_Sym *pstSymbolTable;

    // ELF 헤더와 섹션 헤더 테이블의 첫 번쨰 헤더를 찾음
    pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
    pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);

    // 모든 섹션 헤더를 검색하여 SHT_REL 또는 SHT_RELA 타입을 가진 섹션을 찾아 재배치 수행
    for(i = 1; i < pstELFHeader->e_shnum; i++) {
        if((pstSectionHeader[i].sh_type != SHT_RELA) &&
            ((pstSectionHeader[i].sh_type != SHT_REL))) {
            continue;
        }

        // sh_info 필드에 재배치를 수행할 섹션 헤더의 인덱스가 저장되어 있음
        iSectionIndexToRelocation = pstSectionHeader[i].sh_info;

        // sh_link에는 참고하는 심볼 테이블 섹션 헤더의 인덱스가 저장되어 있음
        iSymbolTableIndex = pstSectionHeader[i].sh_link;

        // 심볼 테이블 섹션 첫 번쨰 엔트리 저장
        pstSymbolTable = (Elf64_Sym*)(pbFileBuffer + pstSectionHeader[iSymbolTableIndex].sh_offset);

        // 재배치 섹션의 엔트리 모두 찾아 재배치 수행
        for(j = 0; j < pstSectionHeader[i].sh_size;) {
            // SHT_REL 타입
            if(pstSectionHeader[i].sh_type == SHT_REL) {
                // SHT_REL 타입은 더해야하는 값이 없으므로 0으로 설정
                pstRel = (Elf64_Rel*)(pbFileBuffer + pstSectionHeader[i].sh_offset + j);
                ulOffset = pstRel->r_offset;
                ulInfo = pstRel->r_info;
                lAddend = 0;

                // SHT_REL 자료구조 크기만큼 이동
                j += sizeof(Elf64_Rel);
            }
            // SHT_RELA 타입
            else {
                pstRela = (Elf64_Rela*)(pbFileBuffer + pstSectionHeader[i].sh_offset + j);
                ulOffset = pstRela->r_offset;
                ulInfo = pstRela->r_info;
                lAddend = pstRela->r_addend;

                // SHT_RELA 자료구조 크기만큼 이동
                j += sizeof(Elf64_Rela);
            }

            // 절대 어드레스 타입은 재배치 무필요
            if(pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_ABS) {
                continue;
            }
            // 공통 타입은 심볼을 지원하지 않으므로 오류를 표시하고 종료
            else if(pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_COMMON) {
                kPrintf("Common symbol is not supported\n");
                return FALSE;
            }

            // 재배치 타입을 구하여 재배치를 수행할 값을 계산
            switch(RELOCATION_LOWER32(ulInfo)) {
            // S + A 계산
            case R_X86_64_64:
            case R_X86_64_32:
            case R_X86_64_32S:
            case R_X86_64_16:
            case R_X86_64_8:
                // 심볼이 존재하는 섹션 헤더의 인덱스
                iSectionIndexInSymbol = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;
                lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
                    pstSectionHeader[iSectionIndexInSymbol].sh_addr) + lAddend;
                break;
            
            // S + A - P로 계산
            case R_X86_64_PC32:
            case R_X86_64_PC16:
            case R_X86_64_PC8:
            case R_X86_64_PC64:
                // 심볼이 존재하는 섹션 헤더의 인덱스
                iSectionIndexInSymbol = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;
                lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
                    pstSectionHeader[iSectionIndexInSymbol].sh_addr) + lAddend;
                break;
            
            // S + A - P로 계산
            case R_X86_64_PC32:
            case R_X86_64_PC16:
            case R_X86_64_PC8:
            case R_X86_64_PC64:
                // 심볼이 존재하는 섹션 헤더의 인덱스
                iSectionIndexInSymbol = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;

                lResult = (pstSymbolTable[RELOCATION_UPPER32])

            }
        }

    }
}