
#include "usart.h"

static char _usart_txChar = 0;
static char _usart_rxChar = 0;
static uint8_t _usart_status = (1 << BIT_UART_TXRDY) | (1 << BIT_UART_TXEMPTY);
static double _usart_charTime = 0;

uint8_t usart_ioRead(uint16_t address, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        _usart_status &= ~(1 << BIT_UART_RXRDY);
    
        return _usart_rxChar;

    }else if(address == 1) // status
    { 
        return _usart_status;
    }

    return 0;
}

void usart_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context)
{
    if(address == 0) // data
    {
        _usart_txChar = data;
        _usart_status &= ~((1 << BIT_UART_TXRDY) | (1 << BIT_UART_TXEMPTY));

    }else if(address == 1) // command/mode
    {
        
    }
}

uint32_t usart_tick(double usElapsed, KIMP_CONTEXT *context)
{
    _usart_charTime += usElapsed;

    return 0;
}

void usart_rxChar(char c)
{
    if(_usart_rxChar)
    {
        _usart_status |= (1 << BIT_UART_OVERRUN_ERR);
    }
    
    _usart_status |= (1 << BIT_UART_RXRDY);

    _usart_rxChar = c;
}

int usart_hasTxChar()
{
    return !(_usart_status & (1 << BIT_UART_TXEMPTY)) && _usart_charTime >= (10 / USART_BAUD);
}

char usart_getTxChar()
{
    _usart_charTime = 0;
    _usart_status |= (1 << BIT_UART_TXRDY) | (1 << BIT_UART_TXEMPTY);
    return _usart_txChar;
}




