int ERROR = -1;
int BIOSReadOneSector(int sector_num, int head_num, int track_num) {
    // 임시 함수
    return 0;
}
void HandleDiskError() {
    printf("DISK ERROR");
    while( 1 );
}

int main(int argc, char *argv[]) {
    int iTotalSectorCount = 1024;
    int iSectorNumber = 2;
    int iHeadNumber = 0;
    int iTrackNumber = 0;

    char *pcTargetAddress = (char*)0x10000;

    while(1) {
        if(iTotalSectorCount == 0)
            break;
        
        iTotalSectorCount = iTotalSectorCount - 1;

        if(BIOSReadOneSector(iSectorNumber, iHeadNumber, iTrackNumber) == ERROR) // BIOS의 섹터 읽기 기능을 호출하는 임의 함수
            HandleDiskError();
        
        pcTargetAddress = pcTargetAddress + 0x200; // 1 섹터 = 512(0x200)

        iSectorNumber += 1;
        if(iSectorNumber < 19) // 섹터 1 ~ 18까지 반복해서 읽어옴
            continue;
        
        iHeadNumber = iHeadNumber ^ 0x01; //헤드의 번호는 0과 1이 반복되므로 이를 편리하기 처리하기 위해 xor 연산 사용
        iSectorNumber = 1;

        if(iHeadNumber != 0) // 헤드 값이 1이면 아직 1번 헤드의 섹터를 다 안돌은 것이기 때문에 반복문 처음으로 돌아간다
            continue;
        
        iTrackNumber += 1; // 헤드 0~1의 모든 섹터를 돌았으므로 트랙 값을 1 늘려 다음 트랙을 처리한다.
    }
    return 0;
}