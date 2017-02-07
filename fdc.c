
#include "fdc.h"

#include "z80emu/z80emu.h"

static uint8_t opReg = 0;
static uint8_t contReg = 0;

static enum
{
    COM_NULL,
    COM_SPECIFY,
    COM_SENSE_INTERRUPT_STATUS,
    COM_RECALIBRATE,
    COM_SEEK,
    COM_READ,
    COM_WRITE,
    COM_FORMAT
} command = COM_NULL;

static enum
{
    STATE_COMM_WAIT_FOR_OPCODE,
    STATE_COMM_PRE_ARGUMENT_DELAY,
    STATE_COMM_WAIT_FOR_ARGUMENTS,
    STATE_COMM_INTER_ARGUMENT_DELAY,
    STATE_EXEC_WAIT_FOR_READ,
    STATE_EXEC_WAIT_FOR_WRITE,
    STATE_EXEC_PROCESSING_DELAY,
    STATE_RESU_WAIT_FOR_READ,
    STATE_RESU_INTER_READ_DELAY,
    STATE_RESETTING

} state = STATE_COMM_WAIT_FOR_OPCODE;

static uint8_t argumentsExpected = 0;
static uint8_t argumentCount = 0;
static uint8_t arguments[16];

static double usDelay = 0;



static void decodeOpcode(uint8_t op)
{
    if(op == 0x03) // specify
    {
        command = COM_SPECIFY;
        argumentsExpected = 2;
    }
}

static void advanceCommandState()
{
    switch(command)
    {

    }
}


uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    if(address == 0) // MSR
    {
        switch(state)
        {
        case STATE_COMM_WAIT_FOR_OPCODE:
        case STATE_COMM_WAIT_FOR_ARGUEMENTS:
            return (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_WAIT_FOR_READ:
            return (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_WAIT_FOR_WRITE:
            return (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY) | (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_PROCESSING_DELAY:
            return (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY);

        case STATE_RESU_WAIT_FOR_READ:
            return (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_COMM_PRE_ARGUMENT_DELAY:
        case STATE_COMM_INTER_ARGUMENT_DELAY:
        case STATE_RESU_INTER_READ_DELAY:
        default:
            return 0;
        }

    }else if(address == 1) // Data
    {
        
    }

    return 0;
}

void fdc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    if(address == 0) // MSR1 (powerdown; only on C version)
    {
        

    }else if(address == 1) // Data
    {
        switch(state)
        {
        case STATE_COMM_WAIT_FOR_OPCODE:    
            decodeOpcode(data);
            if(argumentsExpected)
            {
                state = STATE_COMM_PRE_ARGUMENT_DELAY;
                usDelay = 12;

            }else
            {
                advanceCommandState();
            }
            break;
        
        case STATE_COMM_WAIT_FOR_ARGUMENT:
            arguments[argumentCount++] = data;
            if(argumentCount >= argumentsExpected)
            {
                
            }
            break;
        
        }
    }
}

void fdc_writeOpReg(uint8_t data, KIMP_CONTEXT *context)
{
    opReg = data;

    if(opReg & (1 << BIT_FDC_SOFT_RESET))
    {
        usDelay = 12;
        state = STATE_RESETTING;
    }
}

void fdc_writeContReg(uint8_t data, KIMP_CONTEXT *context)
{
    contReg = data;
}

uint32_t fdc_tick(double usElapsed, KIMP_CONTEXT *context)
{
    uint32_t additionalTicks = 0;

    if(usDelay > 0)
    {
        usDelay -= usElapsed;  
 
    }else
    {
        switch(state)
        {
        case STATE_COMM_PRE_ARGUMENT_DELAY:
        case STATE_COMM_INTER_ARGUMENT_DELAY:
            state = STATE_COMM_WAIT_FOR_ARGUMENT;
            break;

        case STATE_RESU_INTER_READ_DELAY:
            state = STATE_RESU_WAIT_FOR_READ;
            break;

        case STATE_RESETTING:
            opReg &= ~(1 << BIT_FDC_SOFT_RESET);
            state = STATE_COMM_WAITING_FOR_OPCODE;
            command = COM_NULL;
            additionalTicks += Z80Interrupt(&(context->state), context->ivr_fdc, context);
            break;

        default:
            break;
        }
    }

    return additionalTicks;
}





