

#include "bus.h"

#include "usart.h"
#include "pit.h"
#include "fdc.h"
#include "rtc.h"

#include <stdio.h>

uint8_t floatingBus()
{
    return 0;
}

uint8_t readByte(uint16_t address, KIMP_CONTEXT *context)
{
    if(!(context->tccr & 1) && address < 0x2000)
    {
        return context->rom[address];

    }else
    {
        return context->ram[address];
    }
}

uint16_t readWord(uint16_t address, KIMP_CONTEXT *context)
{
    return readByte(address, context) | readByte(address+1, context) << 8;
}

void writeByte(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    if((context->tccr & 1) || address >= 0x2000)
    {
        context->ram[address] = data;
    }
}

void writeWord(uint16_t address, uint16_t data, KIMP_CONTEXT *context)
{
    writeByte(address, data & 0xff, context);
    writeByte(address+1, data >> 8, context);
}

uint8_t ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    // simulate io decoding
    if(address & 0x80) // bit 7 must be 0
    {
        return floatingBus();
    }

    // first io decoder
    uint8_t cs = (address & 0x70) >> 4;
    uint8_t reg = address & 0x0f;

    // second io decoder
    uint8_t css = reg >> 2;
    
    // third io decoder
    uint8_t cst = reg & 0x03;

    switch(cs)
    {
    case 0:
        return usart_ioRead(reg, context);

    case 1:
        return pit_ioRead(reg, context);

    case 2:
        return context->tccr; // some bits need remapping here

    case 3:
        if(!context->has_extension)
                return floatingBus();

        if(css != 3) // fdc uses select line 0,1 and 2. third io decoder is at line 3
        {
            return fdc_ioRead(reg, context);

        }else 
        {
            if(cst == 2) // ebcr at line 2, line 0,1 are IVRs. line 3 not used atm
            {
                return (context->ebcr ^ 1) | 0x80; // bit 0 reads inverted, bit 7 is (active low) opl irq line. keep high for now
            }

            return floatingBus(); // IVRs are not readable
        }

    case 4:
        if(!context->has_extension)
            return floatingBus();

        return rtc_ioRead(reg, context);


    default:
        return floatingBus();
    }
}

void ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    // simulate io decoding
    if(address & 0x80) // bit 7 must be 0
    {
        return;
    }

    // first io decoder
    uint8_t cs = (address & 0x70) >> 4;
    uint8_t reg = address & 0x0f;

    // second io decoder
    uint8_t css = reg >> 2;
    
    // third io decoder
    uint8_t cst = reg & 0x03;

    switch(cs)
    {
    case 0:
        usart_ioWrite(reg, data, context);
        break;

    case 1:
        pit_ioWrite(reg, data, context);
        break;

    case 2:
        context->tccr = data;
        break;

    case 3:
        if(!context->has_extension)
            break;

        if(css == 0) // fdc uses select line 0,1 and 2. third io decoder is at line 3
        {
            fdc_ioWrite(reg, data, context);

        }else if(css == 1)
        {
            fdc_writeOpReg(data, context);

        }else if(css == 2)
        {
            fdc_writeContReg(data, context);

        }else 
        {
            if(cst == 0) // ebcr at line 2, line 0,1 are IVRs. line 3 not used atm
            {
                kimp_debug("[BUS] Wrote FDC-IVR (%x)", data);
                context->ivr_fdc = data;

            }else if(cst == 1)
            {
                kimp_debug("[BUS] Wrote RTC-IVR (%x)", data);
                context->ivr_rtc = data;

            }else if(cst == 2)
            {
                context->ebcr = (context->ebcr & 0xfe) | (data & 0x01);
            }
        }
        break;

    case 4:
        if(!context->has_extension)
            break;

        rtc_ioWrite(reg, data, context);
        break;


    default:
        break;
    }
}





