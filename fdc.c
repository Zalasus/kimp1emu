
#include "fdc.h"

#include "z80emu/z80emu.h"

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

static uint8_t opReg = (1 << BIT_FDC_SOFT_RESET) | (1 << BIT_FDC_DMA_ENABLE);
static uint8_t dataRate = 0x02;

static uint8_t status0 = 0;
static uint8_t status1 = 0;
static uint8_t status2 = 0;

static uint8_t resultBytes[16];
static uint8_t resultBytesAvailable = 0;
static uint8_t resultBytesRead = 0;

static uint8_t currentDrive = 0;
static uint8_t currentTrack = 0;
static uint8_t targetTrack = 0;
static uint8_t currentHead = 0;
static uint8_t currentSector = 0;

static uint8_t execBytesTransferred = 0;
static uint8_t execBytesExpected = 0;

static double stepTime = 1000;
static double headLoadTime = 2000;
static double headUnloadTime = 16000;
static uint8_t headLoaded = 0;

static uint8_t irqLine = 0;



static void raiseInterrupt()
{
    if(!irqLine && (opReg & (1 << BIT_FDC_DMA_ENABLE)))
    {
        irqLine = 1;
        kimp_debug("[FDC] Raising interrupt");
    }
}

static void lowerInterrupt()
{
    if(irqLine)
    {
        irqLine = 0;
        kimp_debug("[FDC] Lowering interrupt");
    }
}

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
        resultBytes[4] = currentHead;
        resultBytes[5] = currentSector;
        resultBytes[6] = arguments[4];
        resultBytesAvailable = 7;
        raiseInterrupt();
        break;

    case COM_INVALID:
        status0 = 0x80;
        resultBytes[0] = status0;
        resultBytesAvailable = 1;
        break;

    default:  // command has no result phase. go to command phase instead
        state = STATE_COMM_PRE_OPCODE_DELAY;
        usDelay = 12;
        command = COM_NULL;
        break;
    }
}

static void execTick()
{
    switch(command)
    {
    case COM_SEEK:
    case COM_RECALIBRATE:
        usDelay = stepTime;
        if(currentTrack > targetTrack) { currentTrack--; }
        else if(currentTrack < targetTrack) { currentTrack++; }
        else // if(currentTrack == targetTrack)
        {
            status0 = (1 << BIT_FDC_SEEK_END) | (arguments[0] & 0x07);
            state = STATE_COMM_PRE_OPCODE_DELAY;
            raiseInterrupt();
            usDelay = 12;
        }
        break;

    case COM_READ:
        state = STATE_EXEC_WAIT_FOR_READ;
        usDelay = 27;
        raiseInterrupt(); // only in non-dma mode
        break;

    case COM_WRITE:
        state = STATE_EXEC_WAIT_FOR_WRITE;
        usDelay = 27;
        raiseInterrupt(); // only in non-dma mode
        break;

    case COM_FORMAT:
        if(execBytesTransferred >= execBytesExpected)
        {
            enterResultPhase();

        }else
        {
            state = STATE_EXEC_WAIT_FOR_WRITE;
            usDelay = US_PER_REVOLUTION / (arguments[2] * 4); // strech out writes over one rev
            raiseInterrupt();
        }
        break;

    default:
        enterResultPhase();
        break;
    }
}

static void enterExecPhase()
{
    switch(command)
    {
    case COM_READ:
    case COM_WRITE:
        if((arguments[0] & 0x03) != currentDrive) kimp_debug("[FDC] Error: drive in command arg does not match selected one in OpReg");
        if(!(((opReg >> BIT_FDC_MOTOR_ENABLE_1) & 0x03) & (currentDrive + 1))) kimp_debug("[FDC] Error: Motor not on");
        if(arguments[2] != ((arguments[0] >> 2) & 1)) kimp_debug("[FDC] Error: Head number mismatch in command arguments");
        state = STATE_EXEC_PROCESSING_DELAY;
        usDelay = US_PER_REVOLUTION; // worst case for r/w is one revolution to find sector
        break;

    case COM_FORMAT:
        if((arguments[0] & 0x03) != currentDrive) kimp_debug("[FDC] Error: Drive in command arg does not match selected one in OpReg");
        if(!(((opReg >> BIT_FDC_MOTOR_ENABLE_1) & 0x03) & (currentDrive + 1))) kimp_debug("[FDC] Error: Motor not on");
        state = STATE_EXEC_PROCESSING_DELAY;
        usDelay = US_PER_REVOLUTION; // worst case for finding index hole is one revolution
        execBytesTransferred = 0;
        execBytesExpected = arguments[2] * 4; // argument #2 is sectors/track, 4 bytes per sector needed
        break;

    case COM_SPECIFY:
        // step rate time; head unload time
        // head load time; no dma
        // invert no dma bit, store it in dmaen bit of op
        stepTime = (0x10 - (arguments[0] >> 4))*1000.0;
        headUnloadTime = (arguments[0] & 0x0f)*16000.0;
        headLoadTime = (arguments[1] >> 1)*2000.0;
        kimp_debug("[FDC] Specified SRT=%f HUT=%f HLT=%f NODMA=%c", stepTime, headUnloadTime, headLoadTime, (arguments[0] & 1) ? '1' : '0');
        if(!(arguments[0] & 1)) kimp_debug("[FDC] Error: DMA mode unsupported");
        enterResultPhase(); // no exec phase
        break;

    case COM_RECALIBRATE:
        if((arguments[0] & 0x03) != currentDrive) kimp_debug("[FDC] Error: drive in command arg does not match selected one in OpReg");
        state = STATE_EXEC_PROCESSING_DELAY;
        targetTrack = 0;
        usDelay = 0; // start stepping right ahead
        break;

    case COM_SEEK:
        if((arguments[0] & 0x03) != currentDrive) kimp_debug("[FDC] Error: drive in command arg does not match selected one in OpReg");
        state = STATE_EXEC_PROCESSING_DELAY;
        targetTrack = arguments[1];
        usDelay = 0; // start stepping right ahead
        break;

    default: // command has no exec phase. go to result phase instead
        enterResultPhase();
        break;
    }
}

static void decodeOpcode(uint8_t op)
{
    argumentCount = 0;

    if(op == 0x03) // specify
    {
        kimp_debug("[FDC] Issued SPECIFY command...");
        command = COM_SPECIFY;
        state = STATE_COMM_PRE_ARGUMENT_DELAY;
        usDelay = 12;
        argumentsExpected = 2;

    }else if(op == 0x07) // recalibrate
    {
        kimp_debug("[FDC] Issued RECALIBRATE command...");
        command = COM_RECALIBRATE;
        state = STATE_COMM_PRE_ARGUMENT_DELAY;
        usDelay = 12;
        argumentsExpected = 1;

    }else if(op == 0x0f) // seek
    {
        kimp_debug("[FDC] Issued SEEK command...");
        command = COM_SEEK;
        state = STATE_COMM_PRE_ARGUMENT_DELAY;
        usDelay = 12;
        argumentsExpected = 2;

    }else if(op == 0x08) // sense interrupt status
    {
        kimp_debug("[FDC] Issued SENSEI command...");
        lowerInterrupt();
        command = COM_SENSE_INTERRUPT_STATUS;
        enterResultPhase();

    }else if((op & 0xBF) == 0x0D) // format
    {
        kimp_debug("[FDC] Issued FORMAT command...");
        command = COM_FORMAT;
        state = STATE_COMM_PRE_ARGUMENT_DELAY;
        usDelay = 12;
        argumentsExpected = 5;

    }else // invalid command
    {
        kimp_debug("[FDC] Issued invalid command! (%x)", op);
        command = COM_INVALID;
        enterResultPhase();
    }
}





uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
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
            return (1 << BIT_FDC_DATA_INPUT) | (1 << BIT_FDC_REQUEST_FOR_MASTER) | (1 << BIT_FDC_BUSY);

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
            lowerInterrupt(); // servicing will lower irq line
            return 42; // return data byte here
            //TODO: atm read and write never terminates.

        case STATE_RESU_WAIT_FOR_READ:
            kimp_debug("[FDC] Handed over result byte (%x)", resultBytes[resultBytesRead]);
            if(irqLine)
            {
                // reading result bytes will clear interrupt
                lowerInterrupt();
            }
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
            return resultBytes[resultBytesRead++];

        default:
            kimp_debug("[FDC] Error: Illegal read of data register");
            return 0;
        }
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
            break;
        
        case STATE_COMM_WAIT_FOR_ARGUMENT:
            kimp_debug("[FDC] Got argument (%x)", data);
            arguments[argumentCount++] = data;
            state = STATE_COMM_INTER_ARGUMENT_DELAY;
            if(argumentCount >= argumentsExpected)
            {
                enterExecPhase();
            }
            break;
        
        case STATE_EXEC_WAIT_FOR_WRITE:
            state = STATE_EXEC_PROCESSING_DELAY;
            usDelay = (usDelay > 27) ? (27 - usDelay) : 0; // don't need to delay cycles spent waiting for service
            lowerInterrupt(); // servicing will lower irq line
            // write data byte here
            break;

        default:
            kimp_debug("[FDC] Error: Illegal write of data register");
            break; // probably an error if we end up here
        }
    }
}

void fdc_writeOpReg(uint8_t data, KIMP_CONTEXT *context)
{
    kimp_debug("[FDC] Write to OPER (%x)", data);

    opReg = data;

    if(!(opReg & (1 << BIT_FDC_SOFT_RESET)))
    {
        usDelay = 0;
        state = STATE_RESETTING;
        command = COM_NULL;
        execBytesTransferred = 0;
        execBytesExpected = 0;
        resultBytesRead = 0;
        resultBytesAvailable = 0;
        kimp_debug("[FDC] Soft reset...");

    }else if(state == STATE_RESETTING)
    {
        state = STATE_COMM_WAIT_FOR_OPCODE;
        kimp_debug("[FDC] Soft reset lifted!");
        raiseInterrupt();
    }

    currentDrive = (opReg & (1 << BIT_FDC_DRIVE_SELECT)) ? 1 : 0;
}

void fdc_writeContReg(uint8_t data, KIMP_CONTEXT *context)
{
    dataRate = data & 0x03;

    kimp_debug("[FDC] Set data rate (%x)", dataRate);
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
            kimp_debug("[FDC] Error: Overrun during read/write");
            status1 = (1 << BIT_FDC_OVERRUN);
            enterResultPhase();
            break;

        case STATE_RESU_INTER_READ_DELAY:
            state = STATE_RESU_WAIT_FOR_READ;
            break;

        default:
            break;
        }
    }

    if(irqLine)
    {
        additionalTicks += Z80Interrupt(&(context->state), context->ivr_fdc, context);

        if(additionalTicks > 0)
        {
            kimp_debug("[FDC] CPU accepted interrupt");
        }
    }

    return additionalTicks;
}





