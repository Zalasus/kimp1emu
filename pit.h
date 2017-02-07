#ifndef PIT_H_INCLUDED
#define PIT_H_INCLUDED

#include "kimp1emu.h"


extern uint8_t pit_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void pit_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);

extern uint32_t pit_tick(double usElapsed, KIMP_CONTEXT *context);

#endif