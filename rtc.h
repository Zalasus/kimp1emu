#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

#include "kimp1emu.h"

// CD bits
#define BIT_RTC_HOLD       0
#define BIT_RTC_BUSY       1 // read-only
#define BIT_RTC_IRQ_FLAG   2 // write 0 to reset, write 1 to leave unaffected
#define BIT_RTC_30_SEC_ADJ 3

// CE bits
#define BIT_RTC_MASK        0
#define BIT_RTC_ITRPT_STND  1 // 0=7.8125ms pulse,  1=as long as irq is set
#define BIT_RTC_T0          2
#define BIT_RTC_T1          3

extern uint8_t rtc_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void rtc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);

extern uint32_t rtc_tick(double usElapsed, KIMP_CONTEXT *context);

#endif