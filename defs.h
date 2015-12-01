#ifndef __DEFS_H__
#define __DEFS_H__

#ifdef _DEBUG
#define UART_TRACE UARTA0_BASE
#endif

#ifdef UART_TRACE
#define PRCM_UART_TRACE PRCM_UARTA0
#define PRCM_GPIO_UART_TRACE PRCM_GPIOA3
#define PIN_UART_TRACE_TX PIN_53
#define PIN_UART_TRACE_TX_MODE PIN_MODE_9
#endif


#ifdef _DEBUG
#include <stdio.h>
#define TRACE printf
#define ASSERT(c) { if ( !(c) ) __asm__(" BKPT #0"); }
#else
#define TRACE(...)
#define ASSERT(...)
#endif

#define asm __asm__

#endif  //  __DEFS_H__
