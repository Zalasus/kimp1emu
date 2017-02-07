#ifndef USART_H_INCLUDED
#define USART_H_INCLUDED

#include "kimp1emu.h"

#define BIT_UART_TXRDY       0
#define BIT_UART_RXRDY       1
#define BIT_UART_TXEMPTY     2
#define BIT_UART_PARITY_ERR  3
#define BIT_UART_OVERRUN_ERR 4
#define BIT_UART_FRAMING_ERR 5
#define BIT_UART_SYNDET      6
#define BIT_UART_DSR         7 // 1=pin low, tied to button (1=button pressed)

extern uint8_t usart_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void usart_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);

extern uint32_t usart_tick(double usElapsed, KIMP_CONTEXT *context);

extern void usart_rxChar(char c);
extern int usart_hasTxChar();
extern char usart_getTxChar();

#endif