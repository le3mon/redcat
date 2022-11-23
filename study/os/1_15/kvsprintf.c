#include "../MINT64/02.Kernel64/Source/Types.h"
#include "../MINT64/02.Kernel64/Source/Utility.h"

int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap) {
    QWORD i, j;
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char *pcCopyString;
    QWORD qwValue;
    int iValue;

    iFormatLength = kStrLen(pcFormatString);
    for(i = 0; i < iFormatLength; i++) {
        if(pcFormatString[i] == '%') {
            i++;
            switch (pcFormatString[i]) {
            case 'S':
                // 가변 인자에 들어 있는 파라미터를 문자열 타입으로 변환
                pcCopyString = (char*)(va_arg(ap, char*));
                iCopyLength = kStrLen(pcCopyString);
                // 문자열 길이만큼 출력 버퍼로 복사하고 출력한 길이만큼 버퍼의 인덱스 이동
                kMemCpy(pcBuffer+iBufferIndex, pcCopyString, iCopyLength);
                iBufferIndex += iCopyLength;
                break;
            
            case 'C':
                // 가변 인자에 들어 있는 파라미터를 문자 타입으로 변환하여
                // 출력 버퍼에 복사하고 버퍼의 인덱스를 1만큼 이동
                pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
                iBufferIndex++;
                break;
            
            case 'd':
            case 'i':
                // 가변 인자에 들어 있는 파라미터를 정수 타입으로 변환
                iValue = (int)(va_arg(ap, int));
                iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
                break;
            
            case 'x':
            case 'X':
                // 가변 인자에 들어 있는 파라미터를 DWORD 타입으로 변환
                qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;
            
            case 'q':
            case 'Q':
            case 'p':
                qwValue = (QWORD)(va_arg(ap, QWORD));
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;

            default:
                pcBuffer[iBufferIndex] = pcFormatString[i];
                iBufferIndex++;
                break;
            }
        }
        else {
            pcBuffer[iBufferIndex] = pcFormatString[i];
            iBufferIndex++;
        }
    }
    // NULL을 추가하여 완전한 문자열로 만들고 출력한 문자의 길이를 반환
    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}