#ifndef __MAIN_H__
#define __MAIN_H__

#include "../../UserLibrary/Source/Types.h"

// 매크로
// 최대 라인 수
#define MAXLINECOUNT        (256 * 1024)

// 윈도우 영역과 파일 내용 표시 영역 사이 여유 공간
#define MARGIN              5

// 탭이 차지하는 크기
#define TABSPACE            4

// 구조체
// 텍스트 정보를 저장하는 구조체
typedef struct TextInformationStruct {
    // 파일 버퍼, 파일 크기
    BYTE *pbFileBuffer;
    DWORD dwFileSize;

    // 파일 내용 표시 영역에 출력할 수 있는 라인 수와 라인별 문자 수
    int iColumnCount;
    int iRowCount;

    // 라인 번호에 따른 파일 오프셋 저장 버퍼
    DWORD *pdwFileOffsetOfLine;
    
    // 파일의 최대 라인 수
    int iMaxLineCount;

    // 현재 라인 인덱스
    int iCurrentLineIndex;

    // 파일 이름
    char vcFileName[100];
} TEXTINFO;

// 함수
BOOL ReadFileToBuffer(const char *pcFileName, TEXTINFO *pstInfo);
void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO *pstInfo);
BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO *pstInfo);

#endif