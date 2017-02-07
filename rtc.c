
#include "rtc.h"

static double _rtc_usTime = 0;
static uint8_t _rtc_div64 = 0;
static uint8_t _rtc_s1   = 0;
static uint8_t _rtc_s10  = 0;
static uint8_t _rtc_mi1  = 0;
static uint8_t _rtc_mi10 = 0;

static uint8_t _rtc_irqFlag = 0;
static uint8_t _rtc_ce = 0;

uint8_t rtc_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    switch(address & 0x0f)
    {
    case 0:
        return _rtc_s1;
    case 1:
        return _rtc_s10;
    case 2:
        return _rtc_mi1;
    case 3:
        return _rtc_mi10;
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
        if(!(data & (1 << BIT_RTC_IRQ_FLAG))) _rtc_irqFlag = 0;
        break;

    case 0x0e:
        _rtc_ce = data;
        break;
    }
}

uint32_t rtc_tick(double usElapsed, KIMP_CONTEXT *context)
{
    uint32_t additionalTicks = 0;

    _rtc_usTime += usElapsed;

    if(_rtc_usTime >= 7812.5 && !(_rtc_ce & (1 << BIT_RTC_ITRPT_STND))) // standard pulse selected?
    {
        _rtc_irqFlag = 0;
    }

    if(_rtc_usTime >= 15625) // 64 Hz carry
    {
        _rtc_usTime = 0;

        if((_rtc_ce & 0x0D) == 0x00) // mask bit 0, t0=t1=0 -> 64 Hz interrupt
        {
            _rtc_irqFlag = 1;
        } 

        if(++_rtc_div64 >= 64)
        {
            _rtc_div64 = 0;
            
            if(++_rtc_s1 >= 10)
            {
                _rtc_s1 = 0;

                if(++_rtc_s10 >= 6)
                {
                    _rtc_s10 = 0;
                }
            }
        }
    }

    if(_rtc_irqFlag)
    {
        additionalTicks += Z80Interrupt(&(context->state), context->ivr_rtc, &context);
    }

    return additionalTicks;
}



