#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/io.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// 매크로
// 한 섹터 바잍 ㅡ수
#define BYTESOFESECTOR      512

// 파캐지 시그니처
#define PACKAGESINGNATURE   "MINT64OSPACKAGE"

// 파일 이름 최대 길이
#define MAXFILENAMELENGTH   24

// DWORD 타입
#define DWORD               unsigned int

// 자료구조
#pragma pack(push, 1)

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

// 함수 선언
int AdjustInSectorSize(int iFd, int iSourceSize);
int CopyFile(int iSourceFd, int iTargetFd);

// 메인
int main(int argc, char *argv[]) {
    int iSourceFd, iTargetFd, iSourceSize, i;
    struct stat stFileData;
    PACKAGEHEADER stHeader;
    PACKAGEITEM stItem;

    // 커맨드 라인 옵션 검사
    if(argc < 2) {
        fprintf(stderr, "[ERROR] PackageMaker.exe app1.elf app2.elf data.txt ...\n");
        exit(-1);
    }

    // Package.img 파일 생성
    if((iTargetFd = open("Package.img", O_RDWR | O_CREAT | O_TRUNC, __S_IREAD | __S_IWRITE)) == -1) {
        fprintf(stderr, "[ERROR] Package.img open fail\n");
        exit(-1);
    }

    printf("[INFO] Create package header...\n");

    // 시그니처 복사 및 헤더 크기 계산
    memcpy(stHeader.vcSignature, PACKAGESINGNATURE, sizeof(stHeader.vcSignature));
    stHeader.dwHeaderSize = sizeof(PACKAGEHEADER) + (argc - 1) * sizeof(PACKAGEITEM);

    // 파일에 저장
    if(write(iTargetFd, &stHeader, sizeof(stHeader)) != sizeof(stHeader)) {
        fprintf(stderr, "[ERROR] Data write fail\n");
        exit(-1);
    }

    // 인자를 돌면서 패키지 헤더 정보 채워 넣음
    for(i = 1; i < argc; i++) {
        // 파일 정보 확인
        if(stat(argv[i], &stFileData) != 0) {
            fprintf(stderr, "[ERROR] %s file open fail\n");
            exit(-1);
        }

        // 파일 이름과 길이 저장
        memset(stItem.vcFileName, 0, sizeof(stItem.vcFileName));
        strncpy(stItem.vcFileName, argv[i], sizeof(stItem.vcFileName));
        stItem.vcFileName[sizeof(stItem.vcFileName) - 1] = '\0';
        stItem.dwFileLength = stFileData.st_size;

        // 파일에 씀
        if(write(iTargetFd, &stItem, sizeof(stItem)) != sizeof(stItem)) {
            fprintf(stderr, "[ERROR] Data write fail\n");
            exit(-1);
        }

        printf("[%d] file: %s, size: %d Byte\n", i, argv[i], stFileData.st_size);
    }

    printf("[INFO] Create complete\n");

    // 생성된 패키지 헤더 뒤에 파일 내용 복사
    printf("[INFO] Copy data file to package...\n");

    // 인자를 돌면서 파일 채워 넣음
    iSourceSize = 0;
    for(i = 1; i < argc; i++) {
        // 데이터 파일 열기
        if((iSourceFd = open(argv[i], O_RDONLY)) == -1) {
            fprintf(stderr, "[ERROR] %s file open fail\n", argv[1]);
            exit(-1);
        }

        // 파일의 내용을 패키지 파일에 쓴 뒤 파일 닫음
        iSourceSize += CopyFile(iSourceFd, iTargetFd);
        close(iSourceFd);
    }

    // 파일 크기 섹터 크기인 512바이트로 맞추기 위해 나머지 부분 0x00으로 채움
    AdjustInSectorSize(iTargetFd, iSourceSize + stHeader.dwHeaderSize);

    // 성공 메시지 출력
    printf("[INFO] Total %d Byte copy complete\n", iSourceSize);
    printf("[INFO] Package file create complete\n");

    close(iTargetFd);
    return 0;
}

// 현재 위치부터 512 배수 위치까지 0x00으로 채움
int AdjustInSectorSize(int iFd, int iSourceSize) {
    int i, iAdjustSizeToSector, iSectorCount;
    char cCh;

    iAdjustSizeToSector = iSourceSize % BYTESOFESECTOR;
    cCh = 0x00;

    if(iAdjustSizeToSector != 0) {
        iAdjustSizeToSector = 512 - iAdjustSizeToSector;
        for(i = 0; i < iAdjustSizeToSector; i++) {
            write(iFd, &cCh, 1);
        }
    }
    else {
        printf("[INFO] File size is aligned 512 bytes\n");
    }

    // 섹터 수 돌려줌
    iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTESOFESECTOR;

    return iSectorCount;
}

// 소스 파일의 내용을 목표 파일에 복사하고 크기 돌려줌
int CopyFile(int iSourceFd, int iTargetFd) {
    int iSourceFileSize, iRead, iWrite;
    char vcBuffer[BYTESOFESECTOR];

    iSourceFileSize = 0;
    while(1) {
        iRead = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
        iWrite = write(iTargetFd, vcBuffer, iRead);

        if(iRead != iWrite) {
            fprintf(stderr, "[ERROR] iRead != iWrite\n");
            exit(-1);
        }
        iSourceFileSize += iRead;

        if(iRead != sizeof(vcBuffer)) {
            break;
        }
    }

    return iSourceFileSize;
}
