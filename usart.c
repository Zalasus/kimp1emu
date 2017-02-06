
#include "usart.h"

#include <stdio.h>



uint8_t usart_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        return getchar();

    }else if(address == 1) // status
    {
        return feof(stdin) ? 0x05 : 0x07; // TX always ready and empty 
    }

    return 0;
}

void usart_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        putchar(data);

    }else if(address == 1) // command/mode
    {

    }
}
