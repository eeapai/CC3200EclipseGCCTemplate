#include <stdlib.h>
#include <string.h>
#include "simplelink.h"
#include "defs.h"
#include "netcfg.h"
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "utils.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "uart.h"

#include <stdio.h>
int main();

typedef void (*pfnVect)();
extern pfnVect vectors[];
#define EXC_VECTOR(f) f,

extern unsigned long _estack;
extern unsigned long _sidata;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long RAM_USED;
extern unsigned long RAM_SIZE;
extern unsigned long RAM_USAGE;


void ccSysInit()
{
  unsigned long dwRAMSize = (unsigned long)&RAM_SIZE;
  unsigned long dwRAMUsed = (unsigned long)&RAM_USED;
  unsigned long dwRAMUsage_percent = (100 * dwRAMUsed) / dwRAMSize;
  MAP_IntVTableBaseSet((unsigned long)vectors);

  //
  // Enable Processor
  //
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);

  PRCMCC3200MCUInit();
#ifdef UART_TRACE
  MAP_PRCMPeripheralClkEnable(PRCM_UART_TRACE, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralClkEnable(PRCM_GPIO_UART_TRACE, PRCM_RUN_MODE_CLK);
  MAP_PinTypeUART(PIN_UART_TRACE_TX, PIN_UART_TRACE_TX_MODE);
  MAP_UARTConfigSetExpClk(UART_TRACE,
                          MAP_PRCMPeripheralClockGet(PRCM_UART_TRACE),
                          115200,
                          (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
#endif

  //
  // Configure PIN_x for SWO
  //
//  PinModeSet(PIN_53, PIN_MODE_13);



  TRACE("\33[2J\n\r");    // clear screen command
  TRACE("\33[3J\n\r");    // clear scroll back
  TRACE("\n\rHello!\n\r");
  TRACE("RAM size: % 7dB\n\r", dwRAMSize);
  TRACE("RAM used: % 7dB (%d%%) (without stack and heap)\n\r", dwRAMUsed, dwRAMUsage_percent);
}

#define CPACR *(unsigned long *)0xE000ED88

__attribute__( ( naked ) )
void Reset_Handler()
{
  register unsigned long *pulSP __asm("sp") = &_estack;
  register unsigned long *pulSrc = &_sidata;
  register unsigned long *pulDest = &_sdata;

  pulSP = pulSP;
  while( pulDest < &_edata )
  {
    *pulDest++ = *pulSrc++;
  }

  pulDest = &_sbss;
  while ( pulDest < &_ebss )
  {
    *pulDest++ = 0;
  }

//  CPACR |= 0x0F000000;

  ccSysInit();
  main();

  while( 1 );
}

typedef struct
{
  unsigned long m_dwExR0;
  unsigned long m_dwExR1;
  unsigned long m_dwExR2;
  unsigned long m_dwExR3;
  unsigned long m_dwExR12;
  unsigned long m_dwExLR;
  unsigned long m_dwExPC;
  unsigned long m_dwExPSR;
}SCortexM3ExContext;

__attribute__( ( naked ) )
void NMIHandler()
{
  asm(" BKPT");
  while( 1 );
}

__attribute__( ( naked ) )
void HardFaultHandler()
{
  register SCortexM3ExContext * psCortexM3ExContext __asm("r0");
  register unsigned long dwLR __asm("lr");

  psCortexM3ExContext = psCortexM3ExContext;
  if ( dwLR & 4 )
    asm(" mrs r0, psp");
  else
    asm(" mrs r0, msp");

  asm(" BKPT");
  while ( 1 );
}

__attribute__( ( naked ) )
void MemManageHandler()
{
  asm(" BKPT");
  while( 1 );
}

__attribute__( ( naked ) )
void BusFaultHandler()
{
  asm(" BKPT");
  while( 1 );
}

__attribute__( ( naked ) )
void UsageFaultHandler()
{
  asm(" BKPT");
  while( 1 );
}

__attribute__( ( naked ) )
void defaultHandler()
{
  asm(" BKPT");
  while( 1 );
}

__attribute__ ((section(".isr_vector")))
pfnVect vectors[NUM_INTERRUPTS] =
{
  (pfnVect)&_estack,
  EXC_VECTOR(Reset_Handler)
  EXC_VECTOR(NMIHandler)
  EXC_VECTOR(HardFaultHandler)
  EXC_VECTOR(MemManageHandler)
  EXC_VECTOR(BusFaultHandler)
  EXC_VECTOR(UsageFaultHandler)
  EXC_VECTOR(defaultHandler)
  EXC_VECTOR(defaultHandler)
};

#define DHCSR *(unsigned long *) 0xE000EDF0
#define C_DEBUGEN (1 << 0)
#define DEMCR *(unsigned long *) 0xE000EDFC
#define TRCENA (1 << 24)
#define ITM_TCR *(unsigned long *) 0xE0000E80
#define ITMENA (1 << 0)
#define ITM_STIM0_ADDR 0xE0000000
int PutChar(int ch)
{
  char flITMEnabled = 0 != (DHCSR & C_DEBUGEN);
#ifdef UART_TRACE
  UARTCharPut(UART_TRACE, ch);
#endif


  if ( flITMEnabled )
  {
    flITMEnabled = 0 != (DEMCR & TRCENA);
  }
  if ( flITMEnabled )
  {
    flITMEnabled = 0 != (ITM_TCR & ITMENA);
  }
  if ( flITMEnabled )
  {
    while ( !*(unsigned long *)ITM_STIM0_ADDR );
    *(unsigned char *)ITM_STIM0_ADDR = (unsigned char)ch;
  }
  return ch;
}

#include <sys/stat.h>

int _close(int file)
{
  return -1;
}

int _fstat(int file, struct stat *st)
{
  st->st_mode =  S_IFSOCK;//S_IFIFO;// S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  return 0;
}

int _read(int file, char *ptr, int len)
{
  return 0;
}

caddr_t _sbrk(int incr)
{
  extern char _end;
  static char *heap_end;
  char *prev_heap_end;
  if (heap_end == 0)
  {
    heap_end = &_end;
  }
  prev_heap_end = heap_end;
  heap_end += incr;
  return (caddr_t) prev_heap_end;
}

int _write(int file, char *ptr, int len)
{
  int i;
  for ( i=0; i < len; i++ )
  {
    PutChar(*ptr++);
  }
  return len;
}
