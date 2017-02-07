#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

#include "kimp1emu.h"


extern uint8_t rtc_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void rtc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);

extern void rtc_tick(double usElapsed, KIMP_CONTEXT *context);

#endif