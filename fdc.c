
#include "fdc.h"

#include "z80emu/z80emu.h"

static uint8_t opReg = 0;
static uint8_t contReg = 0;

static enum
{
    COM_NULL,
    COM_INVALID,
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
    STATE_COMM_PRE_OPCODE_DELAY,
    STATE_COMM_WAIT_FOR_OPCODE,
    STATE_COMM_PRE_ARGUMENT_DELAY,
    STATE_COMM_WAIT_FOR_ARGUMENT,
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

static uint8_t status0 = 0;
static uint8_t status1 = 0;
static uint8_t status2 = 0;

static uint8_t resultBytes[16];
static uint8_t resultBytesAvailable = 0;
static uint8_t resultBytesRead = 0;

static uint8_t currentTrack = 0;
static uint8_t targetTrack = 0;

static uint8_t irqLine = 0;



static void enterResultPhase()
{
    resultBytesRead = 0;
    state = STATE_RESU_WAIT_FOR_READ;

    switch(command)
    {
    case COM_SENSE_INTERRUPT_STATUS:
        resultBytes[0] = status0;
        resultBytes[1] = currentTrack;
        resultBytesAvailable = 2;
        break;

    case COM_READ:
    case COM_WRITE:
    case COM_FORMAT:
        resultBytes[0] = status0;
        resultBytes[1] = status1;
        resultBytes[2] = status2;
        resultBytes[3] = currentTrack;
        resultBytes[4] = selHead;
        resultBytes[5] = selSector;
        resultBytes[6] = bytesPerSector;
        resultBytesAvailable = 7;
        irqLine = 1;
        break;

    case COM_INVALID:
        status0 = 0x80;
        resultBytes[0] = status0;
        resultBytesAvailable = 1;
        break;

    default:  // command has no result phase. go to command phase instead
        state = STATE_COMM_PRE_OPCODE_DELAY;
        usDelay = 12;
        break;
    }
}

static void execTick()
{
    switch(command)
    {
    case COM_SEEK:
        usDelay = 1000; // depends on step rate setting. assume 1ms for now
        if(currentTrack > targetTrack) { currentTrack--; }
        else if(currentTrack < targetTrack) { currentTrack++; }
        else // if(currentTrack == targetTrack)
        {
            status0 = (1 << BIT_FDC_SEEK_END) | (arguments[0] & 0x07);
            state = STATE_COMM_PRE_OPCODE_DELAY;
            irqLine = 1;
            usDelay = 12;
        }
        break;

    case COM_READ:
        state = STATE_EXEC_WAIT_FOR_READ;
        usDelay = 27;
        irqLine = 1; // only in non-dma mode
        break;

    case COM_WRITE:
        state = STATE_EXEC_WAIT_FOR_WRITE;
        usDelay = 27;
        irqLine = 1; // only in non-dma mode
        break;

    case COM_FORMAT:
        enterResultPhase();
        break;

    default:
        break;
    }
}

static void enterExecPhase()
{
    switch(command)
    {
    case COM_READ:
    case COM_WRITE:
        state = STATE_EXEC_PROCESSING_DELAY;
        usDelay = 300; // assume long delay since sector is yet to be found on track
        break;
        
    case COM_FORMAT:
        state = STATE_EXEC_PROCESSING_DELAY;
        usDelay = 1000; // takes ~one revolution of spindle. adjust later
        break;

    case COM_SPECIFY:
        // step rate time; head unload time
        // head load time; no dma
        // invert no dma bit, store it in dmaen bit of op
        opReg = (opReg & ~(1 << BIT_FDC_DMA_ENABLE)) | (((arguments[1] & 1)^1) << BIT_FDC_DMA_ENABLE);
        enterResultPhase(); // no exec phase
        break;

    default: // command has no exec phase. go to result phase instead
        enterResultPhase();
        break;
    }
}

static void decodeOpcode(uint8_t op)
{
    if(op == 0x03) // specify
    {
        command = COM_SPECIFY;
        state = STATE_COMM_PRE_ARGUMENT_DELAY;
        usDelay = 12;
        argumentsExpected = 2;

    }else // invalid command
    {
        command = COM_INVALID;
        enterResultPhase();
    }
}






uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    // the datasheet is not clear on whether any r/w will clear irq or just specific ones
    //  depending on interrupt cause. just assume first case
    if(irqLine)
    {
        irqLine = 0;
    }

    if(address == 0) // MSR
    {
        switch(state)
        {
        case STATE_COMM_WAIT_FOR_OPCODE:
        case STATE_COMM_WAIT_FOR_ARGUMENT:
            return (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_WAIT_FOR_READ:
            return (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_WAIT_FOR_WRITE:
            return (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_EXEC_PROCESSING_DELAY:
            return (1 << BIT_FDC_EXEC_MODE) | (1 << BIT_FDC_BUSY);

        case STATE_RESU_WAIT_FOR_READ:
            return (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_REQUEST_FOR_MASTER);

        case STATE_COMM_PRE_OPCODE_DELAY:
        case STATE_COMM_PRE_ARGUMENT_DELAY:
        case STATE_COMM_INTER_ARGUMENT_DELAY:
        case STATE_RESU_INTER_READ_DELAY:
        default:
            return 0;
        }

    }else if(address == 1) // Data
    {
        switch(state)
        {
        case STATE_EXEC_WAIT_FOR_READ:
            state = STATE_EXEC_PROCESSING_DELAY;
            usDelay = (usDelay > 27) ? (27 - usDelay) : 0; // don't need to delay cycles spent waiting for service
            return 42; // return data byte here
            //TODO: atm read and write never terminates. 

        case STATE_RESU_WAIT_FOR_READ:
            if(resultBytesRead+1 >= resultBytesAvailable)
            {
                // all result bytes read

                state = STATE_COMM_PRE_OPCODE_DELAY;
                command = COM_NULL;
               
            }else
            {
                state = STATE_RESU_INTER_READ_DELAY;
            }
            usDelay = 12;
            return resultBytes[resultBytesRead];

        default:
            return 0; // probably an error if we end up here
        }
    }

    return 0;
}

void fdc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    // the datasheet is not clear on whether any r/w will clear irq or just specific ones
    //  depending on interrupt cause. just assume first case
    if(irqLine)
    {
        irqLine = 0;
    }

    if(address == 0) // MSR1 (powerdown; only on C version)
    {
        

    }else if(address == 1) // Data
    {
        switch(state)
        {
        case STATE_COMM_WAIT_FOR_OPCODE:    
            decodeOpcode(data);
            break;
        
        case STATE_COMM_WAIT_FOR_ARGUMENT:
            arguments[argumentCount++] = data;
            if(argumentCount >= argumentsExpected)
            {
                enterExecPhase();
            }
            break;
        
        case STATE_EXEC_WAIT_FOR_WRITE:
            state = STATE_EXEC_PROCESSING_DELAY;
            usDelay = (usDelay > 27) ? (27 - usDelay) : 0; // don't need to delay cycles spent waiting for service
            // write data byte here
            break;

        default:
            break; // probably an error if we end up here
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
        usDelay = 0; // fix negative delay values

        switch(state)
        {
        case STATE_COMM_PRE_OPCODE_DELAY:
            state = STATE_COMM_WAIT_FOR_OPCODE;
            break;

        case STATE_COMM_PRE_ARGUMENT_DELAY:
        case STATE_COMM_INTER_ARGUMENT_DELAY:
            state = STATE_COMM_WAIT_FOR_ARGUMENT;
            break;

        case STATE_EXEC_PROCESSING_DELAY:
            execTick();
            break;

        case STATE_EXEC_WAIT_FOR_READ:  // waiting time over without servicing. we are overrun
        case STATE_EXEC_WAIT_FOR_WRITE:
            status1 = (1 << BIT_FDC_OVERRUN);
            enterResultPhase();
            break;

        case STATE_RESU_INTER_READ_DELAY:
            state = STATE_RESU_WAIT_FOR_READ;
            break;

        case STATE_RESETTING:
            opReg &= ~(1 << BIT_FDC_SOFT_RESET);
            state = STATE_COMM_WAIT_FOR_OPCODE;
            command = COM_NULL;
            irqLine = 1;
            break;

        default:
            break;
        }
    }

    if(irqLine)
    {
        additionalTicks += Z80Interrupt(&(context->state), context->ivr_fdc, context);
    }

    return additionalTicks;
}





