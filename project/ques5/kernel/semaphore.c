#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "semaphore.h"

static struct semaphore sems[NSEM];      // Fixed kernel semaphore table shared by all processes.
static int shared_counter;               // Shared counter protected by user/test_sync with semaphores.

static int
valid_sem(int id)
{
  return id >= 0 && id < NSEM;           // Accept only indexes inside the fixed semaphore table.
}

void
seminit(void)
{
  for(int i = 0; i < NSEM; i++){         // Initialize every semaphore slot exactly once at boot.
    sems[i].name[0] = 's';               // Build a short stable name without libc formatting.
    sems[i].name[1] = 'e';               // Store the second character of "semNN".
    sems[i].name[2] = 'm';               // Store the third character of "semNN".
    sems[i].name[3] = '0' + i / 10;      // Store the tens digit of the slot number.
    sems[i].name[4] = '0' + i % 10;      // Store the ones digit of the slot number.
    sems[i].name[5] = 0;                 // Terminate the lock name string.
    initlock(&sems[i].lock, sems[i].name); // Initialize the per-semaphore spinlock.
    sems[i].used = 0;                    // Mark the slot unused until sem_init is called.
    sems[i].value = 0;                   // Start with no permits.
  }
  shared_counter = 0;                    // Reset the shared counter at boot.
}

int
sem_init(int id, int value)
{
  if(!valid_sem(id) || value < 0)        // Reject invalid slots and negative permit counts.
    return -1;                           // Report bad arguments to user space.
  acquire(&sems[id].lock);               // Serialize initialization with P/V users.
  sems[id].used = 1;                     // Publish the slot as initialized.
  sems[id].value = value;                // Store the requested initial permit count.
  wakeup(&sems[id]);                     // Wake any sleepers that might be waiting on this slot.
  release(&sems[id].lock);               // Release the semaphore metadata lock.
  return 0;                              // Report success.
}

int
sem_p(int id)
{
  if(!valid_sem(id))                     // Reject indexes outside the semaphore table.
    return -1;                           // Report bad arguments to user space.
  acquire(&sems[id].lock);               // Hold the semaphore lock while inspecting permits.
  if(!sems[id].used){                    // An uninitialized slot cannot be acquired.
    release(&sems[id].lock);             // Drop the lock before returning.
    return -1;                           // Report misuse.
  }
  while(sems[id].value == 0){            // Wait until a permit is available.
    if(killed(myproc())){                // Stop waiting if kill woke this process.
      release(&sems[id].lock);           // Drop the semaphore lock before returning.
      return -1;                         // Report interrupted acquisition to user space.
    }
    sleep(&sems[id], &sems[id].lock);    // Atomically sleep on this semaphore and release lock.
  }
  sems[id].value--;                      // Consume one available permit.
  release(&sems[id].lock);               // Publish the decremented value.
  return 0;                              // Report successful acquisition.
}

int
sem_try_p(int id)
{
  if(!valid_sem(id))                     // Reject invalid slots.
    return -1;                           // Report bad arguments.
  acquire(&sems[id].lock);               // Protect the used/value check.
  if(!sems[id].used || sems[id].value == 0){ // Fail if the slot is absent or empty.
    release(&sems[id].lock);             // Release the lock before returning.
    return -1;                           // Tell user space no permit was acquired.
  }
  sems[id].value--;                      // Consume one permit without blocking.
  release(&sems[id].lock);               // Publish the new value.
  return 0;                              // Report success.
}

int
sem_v(int id)
{
  if(!valid_sem(id))                     // Reject invalid slots.
    return -1;                           // Report bad arguments.
  acquire(&sems[id].lock);               // Serialize the permit update.
  if(!sems[id].used){                    // Reject release on an uninitialized slot.
    release(&sems[id].lock);             // Drop the lock before returning.
    return -1;                           // Report misuse.
  }
  sems[id].value++;                      // Release one permit to the semaphore.
  wakeup(&sems[id]);                     // Wake processes blocked in sem_p on this slot.
  release(&sems[id].lock);               // Publish the incremented value.
  return 0;                              // Report success.
}

int
sem_value(int id)
{
  int value;                             // Local copy returned after releasing the lock.
  if(!valid_sem(id))                     // Reject invalid slots.
    return -1;                           // Report bad arguments.
  acquire(&sems[id].lock);               // Protect the value read.
  value = sems[id].used ? sems[id].value : -1; // Expose -1 for uninitialized slots.
  release(&sems[id].lock);               // Release the lock after copying the value.
  return value;                          // Return the stable snapshot.
}

int
sync_counter_reset(int value)
{
  shared_counter = value;                // Store the caller-provided baseline value.
  return 0;                              // Report success.
}

int
sync_counter_add(int delta)
{
  int value;                             // Local copy of the counter after the update.
  shared_counter += delta;               // Apply the caller's increment.
  value = shared_counter;                // Copy the result before releasing the lock.
  return value;                          // Return the new value to user space.
}

int
sync_counter_get(void)
{
  int value;                             // Local copy of the shared counter.
  value = shared_counter;                // Copy the protected value.
  return value;                          // Return the stable snapshot.
}
