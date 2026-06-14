#include "kernel/types.h"   // Use xv6 integer types shared with the kernel.
#include "kernel/stat.h"    // Include stat declarations required by user.h dependencies.
#include "user/user.h"      // Use fork, wait, exit, printf, and synchronization syscalls.

static void
worker(void)
{
  for(int i = 0; i < 100; i++){         // Each worker contributes exactly 100 increments.
    if(sem_p(0) < 0){                   // Acquire semaphore 0 as a binary mutex.
      printf("   worker: sem_p failed\n"); // Report unexpected syscall failure.
      exit(1);                          // Stop this worker with a failing status.
    }
    sync_counter_add(1);                // Update the shared kernel counter while holding mutex.
    sem_v(0);                           // Release the mutex so another worker can enter.
  }
  exit(0);                              // Report successful worker completion to the parent.
}

static void
blocking_worker(void)
{
  if(sem_p(2) < 0){                     // Block on empty semaphore 2 until parent releases it.
    printf("   blocking worker: sem_p failed\n"); // Report an unexpected interrupted wait.
    exit(1);                            // Return failure to the parent.
  }
  sync_counter_add(10);                 // Prove the child ran only after sem_v woke it.
  exit(0);                              // Return success to the parent.
}

int
main(void)
{
  int pid1;                             // First child pid.
  int pid2;                             // Second child pid.
  int pid3;                             // Blocking child pid.
  int status;                           // Status returned by wait.
  int final;                            // Final shared counter value.
  int value;                            // Snapshot of a semaphore value.
  int try_result;                       // Return value from nonblocking try-P.

  printf("\n=== Task 5: Synchronization Demo ===\n\n"); // Visible test banner.
  printf("1. Kernel semaphore P/V protects a shared kernel counter\n"); // Describe the first check.

  if(sem_init(0, 1) < 0){               // Initialize semaphore 0 as a binary mutex.
    printf("   sem_init(0, 1) failed\n"); // Report initialization failure.
    exit(1);                            // Fail the test immediately.
  }
  sync_counter_reset(0);                // Reset the shared kernel counter before forking.

  pid1 = fork();                        // Create the first real user process.
  if(pid1 < 0){                         // Negative pid means fork failed.
    printf("   first fork failed\n");   // Report process creation failure.
    exit(1);                            // Fail the test.
  }
  if(pid1 == 0)                         // Child sees fork return zero.
    worker();                           // Run the first worker body in user mode.

  pid2 = fork();                        // Create the second real user process.
  if(pid2 < 0){                         // Negative pid means fork failed.
    printf("   second fork failed\n");  // Report process creation failure.
    exit(1);                            // Fail the test.
  }
  if(pid2 == 0)                         // Child sees fork return zero.
    worker();                           // Run the second worker body in user mode.

  wait(&status);                        // Reap one worker process.
  if(status != 0){                      // Child status must report success.
    printf("   worker exited with status %d\n", status); // Print failing status.
    exit(1);                            // Fail the test if a worker failed.
  }
  wait(&status);                        // Reap the other worker process.
  if(status != 0){                      // Child status must report success.
    printf("   worker exited with status %d\n", status); // Print failing status.
    exit(1);                            // Fail the test if a worker failed.
  }

  final = sync_counter_get();           // Read the protected shared counter after both workers exit.
  printf("   - shared_counter final: %d (expected 200)\n", final); // Print the deterministic result.
  if(final != 200)                      // Validate the synchronization result.
    exit(1);                            // Return failure if mutual exclusion broke.

  printf("\n2. Counting semaphore state transitions\n"); // Describe the second check.
  if(sem_init(1, 2) < 0)                 // Initialize semaphore 1 with two permits.
    exit(1);                             // Fail if initialization is rejected.
  if(sem_p(1) < 0)                       // Consume one permit.
    exit(1);                             // Fail if P unexpectedly fails.
  value = sem_value(1);                  // Read the value after one P operation.
  printf("   - semaphore value after P: %d (expected 1)\n", value); // Show P result.
  if(value != 1)                         // Validate the value after P.
    exit(1);                             // Fail on an incorrect semaphore value.
  if(sem_v(1) < 0)                       // Return one permit.
    exit(1);                             // Fail if V unexpectedly fails.
  value = sem_value(1);                  // Read the value after one V operation.
  printf("   - semaphore value after V: %d (expected 2)\n", value); // Show V result.
  if(value != 2)                         // Validate the value after V.
    exit(1);                             // Fail on an incorrect semaphore value.

  printf("\n3. Blocking wait, wakeup, and deadlock check\n"); // Describe the blocking check.
  if(sem_init(2, 0) < 0)                 // Initialize semaphore 2 as empty.
    exit(1);                             // Fail if initialization is rejected.
  sync_counter_reset(0);                 // Reset the counter used to prove child progress.
  pid3 = fork();                         // Create a child that should block in sem_p.
  if(pid3 < 0){                          // Negative pid means fork failed.
    printf("   blocking fork failed\n"); // Report process creation failure.
    exit(1);                             // Fail the test.
  }
  if(pid3 == 0)                          // Child sees fork return zero.
    blocking_worker();                   // Run the blocking semaphore test in user mode.
  pause(10);                             // Give the child time to enter sem_p and sleep.
  final = sync_counter_get();            // Read the counter before releasing the semaphore.
  printf("   - counter before V: %d (expected 0)\n", final); // Show that child is blocked.
  if(final != 0)                         // Child must not have passed sem_p yet.
    exit(1);                             // Fail if empty semaphore did not block.
  if(sem_v(2) < 0)                       // Release one permit and wake the child.
    exit(1);                             // Fail if V unexpectedly fails.
  wait(&status);                         // Reap the blocking child.
  if(status != 0){                       // Child status must report success.
    printf("   blocking worker exited with status %d\n", status); // Print failing status.
    exit(1);                             // Fail the test if the child failed.
  }
  final = sync_counter_get();            // Read the counter after the child was woken.
  printf("   - counter after wakeup: %d (expected 10)\n", final); // Show wakeup result.
  if(final != 10)                        // The child must have run after sem_v.
    exit(1);                             // Fail if wakeup did not happen.
  sem_init(3, 0);                        // Initialize another empty semaphore for try-P.
  try_result = sem_try_p(3);             // Try to acquire without blocking.
  printf("   - try-P on empty semaphore: %d (expected -1)\n", try_result); // Nonblocking deadlock check.
  if(try_result != -1)                   // try-P must fail on an empty semaphore.
    exit(1);                             // Fail if try-P incorrectly acquired a permit.

  printf("\n=== Synchronization Test PASSED ===\n"); // Stable pass marker for QEMU logs.
  exit(0);                              // Return success to init.
}
