
#ifndef BUS_H_INCLUDED
#define BUS_H_INCLUDED

#include "kimp1emu.h"


extern uint8_t readByte(uint16_t address, KIMP_CONTEXT *context);

extern uint16_t readWord(uint16_t address, KIMP_CONTEXT *context);

extern void writeByte(uint16_t address, uint8_t data, KIMP_CONTEXT *context);

extern void writeWord(uint16_t address, uint16_t data, KIMP_CONTEXT *context);

extern uint8_t ioRead(uint16_t address, KIMP_CONTEXT *context);

extern void ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);


#endif