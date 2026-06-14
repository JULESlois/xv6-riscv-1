#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sem_init(void)
{
  int id;                               // User-provided semaphore table index.
  int value;                            // User-provided initial permit count.
  argint(0, &id);                       // Read syscall argument 0 from trapframe.
  argint(1, &value);                    // Read syscall argument 1 from trapframe.
  return sem_init(id, value);           // Initialize the kernel semaphore slot.
}

uint64
sys_sem_p(void)
{
  int id;                               // User-provided semaphore table index.
  argint(0, &id);                       // Read syscall argument 0 from trapframe.
  return sem_p(id);                     // Run the blocking P operation.
}

uint64
sys_sem_v(void)
{
  int id;                               // User-provided semaphore table index.
  argint(0, &id);                       // Read syscall argument 0 from trapframe.
  return sem_v(id);                     // Run the V operation.
}

uint64
sys_sem_value(void)
{
  int id;                               // User-provided semaphore table index.
  argint(0, &id);                       // Read syscall argument 0 from trapframe.
  return sem_value(id);                 // Return the current permit count.
}

uint64
sys_sem_try_p(void)
{
  int id;                               // User-provided semaphore table index.
  argint(0, &id);                       // Read syscall argument 0 from trapframe.
  return sem_try_p(id);                 // Try to acquire without sleeping.
}

uint64
sys_sync_counter_reset(void)
{
  int value;                            // User-provided new counter value.
  argint(0, &value);                    // Read syscall argument 0 from trapframe.
  return sync_counter_reset(value);     // Reset the shared kernel counter.
}

uint64
sys_sync_counter_add(void)
{
  int delta;                            // User-provided increment amount.
  argint(0, &delta);                    // Read syscall argument 0 from trapframe.
  return sync_counter_add(delta);       // Add to the shared kernel counter.
}

uint64
sys_sync_counter_get(void)
{
  return sync_counter_get();            // Return the shared kernel counter.
}
