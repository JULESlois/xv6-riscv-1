//
// Console input and output, to the uart.
// Basic console driver for Bootloader task.
//

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100

void
consputc(int c)
{
  if(c == BACKSPACE){
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  uint r;
  uint w;
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
} cons;

void
consoleinit(void)
{
  initlock(&cons.lock, "console");
  cons.r = 0;
  cons.w = 0;
  uartinit();
}