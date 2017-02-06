#ifndef FDC_H_INCLUDED
#define FDC_H_INCLUDED

#include "kimp1emu.h"


extern uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void fdc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);


#endif