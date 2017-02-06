#ifndef USART_H_INCLUDED
#define USART_H_INCLUDED

#include "kimp1emu.h"


extern uint8_t usart_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void usart_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);


#endif