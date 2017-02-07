#ifndef FDC_H_INCLUDED
#define FDC_H_INCLUDED

#include "kimp1emu.h"

// master status register bits
#define BIT_FDC_FDD0_BUSY           0
#define BIT_FDC_FDD1_BUSY           1
#define BIT_FDC_FDD2_BUSY           2
#define BIT_FDC_FDD3_BUSY           3
#define BIT_FDC_BUSY                4
#define BIT_FDC_EXEC_MODE           5
#define BIT_FDC_DATA_INPUT          6
#define BIT_FDC_REQUEST_FOR_MASTER  7

// operations register bits
#define BIT_FDC_DRIVE_SELECT    0
#define BIT_FDC_SOFT_RESET      2 // active low
#define BIT_FDC_DMA_ENABLE      3
#define BIT_FDC_MOTOR_ENABLE_1  4
#define BIT_FDC_MOTOR_ENABLE_2  5
#define BIT_FDC_MODE_SELECT     7


extern uint8_t fdc_ioRead(uint16_t address, KIMP_CONTEXT *context);
extern void fdc_ioWrite(uint16_t address, uint8_t data, KIMP_CONTEXT *context);
extern void fdc_writeOpReg(uint8_t data, KIMP_CONTEXT *context);
extern void fdc_writeContReg(uint8_t data, KIMP_CONTEXT *context);

extern uint32_t fdc_tick(double usElapsed, KIMP_CONTEXT *context);

#endif