

#ifndef KIMP1EMU_H_INCLUDED
#define KIMP1EMU_H_INCLUDED

#include "z80emu/z80emu.h"

#define ROM_SIZE 0x2000
#define RAM_SIZE 0xffff

typedef struct KIMP_CONTEXT
{
    Z80_STATE state;

    uint8_t ram[RAM_SIZE];
    uint8_t rom[ROM_SIZE];

    uint8_t tccr;
    uint8_t ebcr;

    uint8_t ivr_fdc;
    uint8_t ivr_rtc;

    uint8_t stopped;

} KIMP_CONTEXT;


#endif