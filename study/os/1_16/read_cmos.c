#include "../MINT64/02.Kernel64/Source/Types.h"
#include "../MINT64/02.Kernel64/Source/AssemblyUtility.h"

#define RTC_CMOSADDRESS     0x70
#define RTC_CMOSDATA        0x71

#define RTC_ADDRESS_SECOND  0x00
#define RTC_ADDRESS_MINUTE  0X02
#define RTC_ADDRESS_HOUR    0X04
#define RTC_ADDRESS_DAYOFWEEK   0X06
#define RTC_ADDRESS_DAYOFMONTH  0X07
#define RTC_ADDRESS_MONTH   0X08
#define RTC_ADDRESS_YEAR    0X09

#define RTC_BCDTOBINARY(x)  ((((x) >> 4) * 10) + ((x) & 0x0F))

void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond) {
    BYTE bData;
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);

    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);
}

void kReadRTCData(WORD *pwYear, BYTE *pbMonth, BYTE *pbDayOfMonth, BYTE *pbDayOfWeek) {
    BYTE bData;
}