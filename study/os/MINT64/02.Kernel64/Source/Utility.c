#include "Utility.h"
#include "AssemblyUtility.h"
#include <stdarg.h>
#include "VBE.h"

volatile QWORD g_qwTickCount = 0;

void kMemSet(void *pvDestination, BYTE bData, int iSize) {
    int i;
    QWORD qwData;
    int iRemainByteStartOffset;

    qwData = 0;
    for(i = 0; i < 8; i++) {
        qwData = (qwData << 8) | bData;
    }

    for(i = 0; i < (iSize / 8); i++) {
        ((QWORD*)pvDestination)[i] = qwData;
    }

    iRemainByteStartOffset = i * 8;
    for(i = 0; i < (iSize % 8); i++) {
        ((char*)pvDestination)[iRemainByteStartOffset++] = bData;
    }
}

int kMemCpy(void *pvDestination, const void *pvSource, int iSize) {
    int i;
    int iRemainByteStartOffset;

    for(i = 0; i < (iSize / 8); i++) {
        ((QWORD*)pvDestination)[i] = ((QWORD*)pvSource)[i];
    }

    iRemainByteStartOffset = i * 8;
    for(i = 0; i < (iSize % 8); i++) {
        ((char*)pvDestination)[iRemainByteStartOffset] = ((char*)pvSource)[iRemainByteStartOffset];
        iRemainByteStartOffset++;
    }

    return iSize;
}

int kMemCmp(const void *pvDestination, const void *pvSource, int iSize) {
    int i, j;
    int iRemainByteStartOffset;
    QWORD qwValue;
    char cValue;

    for(i = 0; i < (iSize / 8); i++) {
        qwValue = ((QWORD*)pvDestination)[i] - ((QWORD*)pvSource)[i];

        if(qwValue != 0) {
            for(i = 0; i < 8; i++) {
                if(((qwValue >> (i * 8)) & 0xFF) != 0) {
                    return (qwValue >> (i * 8)) & 0xFF;
                }
            }
        }
    }

    iRemainByteStartOffset = i * 8;
    for(i = 0; i < (iSize % 8); i++) {
        cValue = ((char*)pvDestination)[iRemainByteStartOffset] - ((char*)pvSource)[iRemainByteStartOffset];
        if(cValue != 0) {
            return cValue;
        }
        iRemainByteStartOffset++;
    }
    
    return 0;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
    QWORD qwRFLAGS;

    qwRFLAGS = kReadRFLAGS();
    if(bEnableInterrupt == TRUE)
        kEnableInterrupt();
    else
        kDisableInterrupt();
    
    if(qwRFLAGS & 0x0200)
        return TRUE;
    
    return FALSE;
}

int kStrLen(const char *pcBuffer) {
    int i;
    for(i = 0; ; i++) {
        if(pcBuffer[i] == '\0')
            break;
    }
    return i;
}

static gs_qwTotalRAMMBSize = 0;

void kCheckTotalRAMSize(void) {
    DWORD *pdwCurrentAddress;
    DWORD dwPreviousValue;

    pdwCurrentAddress = (DWORD*)0x400000;
    while (1) {
        dwPreviousValue = *pdwCurrentAddress;
        *pdwCurrentAddress = 0x12345678;
        if(*pdwCurrentAddress != 0x12345678)
            break;  
        
        *pdwCurrentAddress = dwPreviousValue;
        pdwCurrentAddress += (0x400000 / 4);
    }
    gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;
}

QWORD kGetTotalRAMSize(void) {
    return gs_qwTotalRAMMBSize;
}

long kAToI(const char *pcBuffer, int iRadix) {
    long lReturn;

    switch(iRadix) {
    case 16:
        lReturn = kHexStringToQword(pcBuffer);
        break;
    
    case 10:
    default:
        lReturn = kDecimalStringToLong(pcBuffer);
        break;
    }
    return lReturn;
}

QWORD kHexStringToQword(const char *pcBuffer) {
    QWORD qwValue = 0;
    int i;

    for(i = 0; pcBuffer[i] != '\0'; i++) {
        qwValue *= 16;
        if(('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z')) {
            qwValue += (pcBuffer[i] - 'A') + 10;
        }
        else if(('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z')) {
            qwValue += (pcBuffer[i] - 'a') + 10;
        }
        else {
            qwValue += pcBuffer[i] - '0';
        }
    }
    return qwValue;
}

long kDecimalStringToLong(const char *pcBuffer) {
    long lValue = 0;
    int i;

    if(pcBuffer[0] == '-')
        i = 1;
    else
        i = 0;
    
    for(; pcBuffer[i] != '\0'; i++) {
        lValue *= 10;
        lValue += pcBuffer[i] - '0';
    }

    if(pcBuffer[0] == '-')
        lValue = -lValue;
    
    return lValue;
}

int kIToA(long lValue, char *pcBuffer, int iRadix) {
    int iReturn;

    switch (iRadix) {
    case 16:
        iReturn = kHexToString(lValue, pcBuffer);
        break; 
    
    case 10:
    default:
        iReturn = kDecimalToString(lValue, pcBuffer);
        break;
    }
    
    return iReturn;
}

int kHexToString(QWORD qwValue, char *pcBuffer) {
    QWORD i;
    QWORD qwCurrentValue;

    if(qwValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    for(i = 0; qwValue > 0; i++) {
        qwCurrentValue = qwValue % 16;
        if(qwCurrentValue >= 10)
            pcBuffer[i] = 'A' + (qwCurrentValue - 10);
        else
            pcBuffer[i] = '0' + qwCurrentValue;
        
        qwValue = qwValue / 16;
    }
    pcBuffer[i] = '\0';

    kReverseString(pcBuffer);
    return i;
}

int kDecimalToString(long lValue, char *pcBuffer) {
    long i;

    if(lValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    if(lValue < 0) {
        i = 1;
        pcBuffer[0] = '-';
        lValue = -lValue;
    }
    else
        i = 0;
    
    for(; lValue > 0; i++) {
        pcBuffer[i] = '0' + lValue % 10;
        lValue = lValue / 10;
    }
    pcBuffer[i] = '\0';
    
    if(pcBuffer[0] == '-')
        kReverseString(&(pcBuffer[1]));
    else
        kReverseString(pcBuffer);
    
    return i;
}

void kReverseString(char *pcBuffer) {
    int iLength;
    int i;
    char cTemp;

    iLength = kStrLen(pcBuffer);
    for(i = 0; i < iLength / 2; i++) {
        cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength - 1 - i];
        pcBuffer[iLength - 1 - i] = cTemp;
    }
}

int kSPrintf(char *pcBuffer, const char *pcFormatString, ...) {
    va_list ap;
    int iReturn;

    va_start(ap, pcFormatString);
    iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
    va_end(ap);

    return iReturn;    
}

int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap) {
    QWORD i, j, k;
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char *pcCopyString;
    QWORD qwValue;
    int iValue;
    double dValue;

    iFormatLength = kStrLen(pcFormatString);
    for(i = 0; i < iFormatLength; i++) {
        if(pcFormatString[i] == '%') {
            i++;
            switch (pcFormatString[i]) {
            case 's':
                pcCopyString = (char*)(va_arg(ap, char*));
                iCopyLength = kStrLen(pcCopyString);
                kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
                iBufferIndex += iCopyLength;
                break;
            
            case 'c':
                pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
                iBufferIndex++;
                break;
            
            case 'd':
            case 'i':
                iValue = (int)(va_arg(ap, int));
                iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
                break;
            
            case 'x':
            case 'X':
                qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;
            
            case 'q':
            case 'Q':
            case 'p':
                qwValue = (QWORD)(va_arg(ap, QWORD));
                iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                break;
            
            case 'f':
                dValue = (double)(va_arg(ap, double));
                // 셋째 자리에서 반올림 처리
                dValue += 0.005;
                // 소수점 둘째 자리부터 차례로 지정하여 버퍼를 뒤집음
                pcBuffer[iBufferIndex] = '0' + (QWORD)(dValue * 100) % 10;
                pcBuffer[iBufferIndex + 1] = '0' + (QWORD)(dValue * 10) % 10;
                pcBuffer[iBufferIndex + 2] = '.';
                for(k = 0; ; k++) {
                    if(((QWORD)dValue == 0) && (k != 0)) {
                        break;
                    }
                    pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
                    dValue = dValue / 10;
                }
                pcBuffer[iBufferIndex + 3 + k] = '\0';
                // 값이 저장된 길이만큼 뒤집고 길이를 증가시킴
                kReverseString(pcBuffer + iBufferIndex);
                iBufferIndex += 3 + k;
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
    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}

void kSleep(QWORD qwMillisecond) {
    QWORD qwLastTickCount;

    qwLastTickCount = g_qwTickCount;

    while((g_qwTickCount - qwLastTickCount) <= qwMillisecond) {
        kSchedule();
    }
}

QWORD kGetTickCount(void) {
    return g_qwTickCount;
}

void kMemSetWord(void *pvDestination, WORD wData, int iWordSize) {
    int i;
    QWORD qwData;
    int iRemainWordStartOffset;

    // 8바이트에 WORD 데이터를 채움
    qwData = 0;
    for(i = 0; i < 4; i++) {
        qwData = (qwData << 16) | wData;
    }

    // 8바이트씩 먼저 채움, word 데이터를 4개씩 한꺼번에 채울 수 있음
    for(i = 0; i < (iWordSize / 4); i++) {
        ((QWORD*)pvDestination)[i] = qwData;
    }

    // 8바이트씩 채우고 남은 부분을 마무리
    iRemainWordStartOffset = i * 4;
    for(i = 0; i < (iWordSize % 4); i++) {
        ((WORD*)pvDestination)[iRemainWordStartOffset++] = wData;
    }
}

// 그래픽 모드인지 여부 반환
BOOL kIsGraphicMode(void) {
    // 그래픽 모드 시작 여부가 저장된 어드레스(0x7c0a)
    if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
        return FALSE;
    }

    return TRUE;
}