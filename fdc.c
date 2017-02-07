
#include "fdc.h"

#include "z80emu/z80emu.h"

static uint8_t _fdc_opReg = 0;
static uint8_t _fdc_contReg = 0;

static enum
{
    PHASE_COMMAND,
    PHASE_EXEC,
    PHASE_RESULT
} _fdc_phase = PHASE_COMMAND;

static uint8_t _fdc_command = 0;

static double _fdc_resetTime = 0;

uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    if(address == 0) // MSR
    {
        if(_fdc_phase == PHASE_EXEC)
        {
            return (1 << BIT_FDC_BUSY) | (1 << BIT_FDC_EXEC_MODE);

        }else if(_fdc_phase == PHASE_COMMAND)
        {
            return (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_REQUEST_FOR_MASTER);
        }

    }else if(address == 1) // Data
    {
        
    }

    return 0;
}

void fdc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{

}

void fdc_writeOpReg(uint8_t data, KIMP_CONTEXT *context)
{
    _fdc_opReg = data;

    if(_fdc_opReg & (1 << BIT_FDC_SOFT_RESET))
    {
        _fdc_resetTime = 0;
    }
}

void fdc_writeContReg(uint8_t data, KIMP_CONTEXT *context)
{
    _fdc_contReg = data;
}

void fdc_tick(double usElapsed, KIMP_CONTEXT *context)
{
    if(_fdc_opReg & (1 << BIT_FDC_SOFT_RESET))
    {
        _fdc_resetTime += usElapsed;

        if(_fdc_resetTime >= 16000)
        {
            _fdc_opReg &= ~(1 << BIT_FDC_SOFT_RESET);
            _fdc_resetTime = 0;
            _fdc_phase = PHASE_COMMAND;
            _fdc_command = 0;

            Z80Interrupt(&(context->state), context->ivr_fdc, context);
        }
    }
}





