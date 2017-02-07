
#include "usart.h"

static char _usart_txChar = 0;
static char _usart_rxChar = 0;
static uint8_t _usart_status = 0;

uint8_t usart_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        char c = _usart_rxChar;
        _usart_rxChar = 0;
        return c;

    }else if(address == 1) // status
    { 
        uint8_t status = 0;
        if(_usart_txChar)
            status |= (1 << BIT_UART_TXRDY) | (1 << BIT_UART_TXEMPTY);
        if(_usart_rxChar)
            status |= (1 << BIT_UART_RXRDY);

        return status;
    }

    return 0;
}

void usart_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        _usart_txChar = data;

    }else if(address == 1) // command/mode
    {
        
    }
}

void usart_rxChar(char c)
{
    if(_usart_hasRxChar)
    {
        // overrun
    }
    
    _usart_rxChar = c;
}

int usart_hasTxChar()
{
    return _usart_txChar != 0;
}

char usart_getTxChar()
{
    return _usart_txChar;
}
