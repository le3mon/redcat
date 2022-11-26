#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

#define RTC_CMOSADDRESS     0X70
#define RTC_CMOSDATA        0X71

#define RTC_ADDRESS_SECOND  0X00
#define RTC_ADDRESS_MINUTE  0X02
#define RTC_ADDRESS_HOUR    0X04
#define RTC_ADDRESS_DAYOFWEEK   0X06
#define RTC_ADDRESS_DAYOFMONTH  0X07
#define RTC_ADDRESS_MONTH       0X08
#define RTC_ADDRESS_YEAR        0X09

#define RTC_BCDTOBINARY(x)      (((x) >> 4) * 10) + ((x) & 0x0F)

void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond);
void kReadRTCDate(WORD *pwYear, BYTE *pbMonth, BYTE *pbDayOfMonth, BYTE *pbDayOfWeek);

#endif