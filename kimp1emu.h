

#ifndef KIMP1EMU_H_INCLUDED
#define KIMP1EMU_H_INCLUDED

#include <stdint.h>

#include "z80emu/z80emu.h"

#define ROM_SIZE 0x2000
#define RAM_SIZE 0xffff

#define CPU_SPEED 2457600
#define CPU_SPEED_MHZ 2.4576

typedef struct KIMP_CONTEXT
{
    Z80_STATE state;

    uint8_t ram[RAM_SIZE];
    uint8_t rom[ROM_SIZE];

    uint8_t tccr;
    uint8_t ebcr;

    uint8_t ivr_fdc;
    uint8_t ivr_rtc;

    uint8_t has_extension;
    uint8_t stopped;

} KIMP_CONTEXT;


#endif