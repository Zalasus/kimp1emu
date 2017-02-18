
#include "rtc.h"

static double  usTime = 0;
static uint8_t div64 = 0;
static uint8_t s1   = 0;
static uint8_t s10  = 0;
static uint8_t mi1  = 0;
static uint8_t mi10 = 0;
static uint8_t h1 = 0;
static uint8_t h10 = 0;

static uint8_t irqFlag = 0;
static uint8_t ce = (1 << BIT_RTC_MASK);
static uint8_t cf = 0;

static void raiseInterrupt()
{
    if(!irqFlag)
    {
        irqFlag = 1;

        kimp_debug("[RTC] Raising interrupt");
    }
}

static void lowerInterrupt()
{
    if(irqFlag)
    {
        irqFlag = 0;

        kimp_debug("[RTC] Lowering interrupt");
    }
}

uint8_t rtc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    switch(address & 0x0f)
    {
    case 0:
        return s1;
    case 1:
        return s10;
    case 2:
        return mi1;
    case 3:
        return mi10;
    case 4:
        return h1;
    case 5:
        return h10;
    
    default:
        return 0;
    }

    return 0;
}

void rtc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    switch(address & 0x0f)
    {
    case 0x0d:
        kimp_debug("[RTC] Wrote CD (%x)", data);
        if(!(data & (1 << BIT_RTC_IRQ_FLAG))) lowerInterrupt();
        break;

    case 0x0e:
        kimp_debug("[RTC] Wrote CE (%x)", data);
        ce = data;
        break;

    case 0x0f:
        kimp_debug("[RTC] Wrote CF (%x)", data);
        cf = data;
        if(cf & (1 << BIT_RTC_REST))
        {
            usTime = 0;
        }
        break;
    
    default:
        break;
    }
}

uint32_t rtc_tick(double usElapsed, KIMP_CONTEXT *context)
{
    uint32_t additionalTicks = 0;

    usTime += usElapsed;

    if(cf & (1 << BIT_RTC_REST))
    {
        usTime = 0;
    }

    if(usTime >= 7812.5 && !(ce & (1 << BIT_RTC_ITRPT_STND))) // standard pulse selected?
    {
        lowerInterrupt();
    }

    if(usTime >= 15625) // 64 Hz carry
    {
        usTime = 0;

        if((ce & ~(1 << BIT_RTC_ITRPT_STND)) == 0x00) // mask bit 0, t0=t1=0 -> 64 Hz interrupt
        {
            raiseInterrupt();
        } 

        if(++div64 >= 64)
        {
            div64 = 0;
            
            if(++s1 >= 10)
            {
                s1 = 0;

                if(++s10 >= 6)
                {
                    s10 = 0;

                    if(++mi1 >= 10)
                    {
                        mi1 = 0;
                        
                        if(++mi10 >= 6)
                        {
                            mi10 = 0;
    
                            h1++;
                        }
                    }
                }
            }
        }
    }

    if(irqFlag)
    {
        additionalTicks += Z80Interrupt(&(context->state), context->ivr_rtc, &context);

        if(additionalTicks > 0)
        {
            kimp_debug("[RTC] CPU accepted interrupt");
        }
    }

    return additionalTicks;
}



