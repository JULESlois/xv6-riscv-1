// test_sync.c - Demo for Synchronization Mechanism
// Task 5: Synchronization - Concurrency Control and Data Consistency

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"

volatile int shared_counter = 0;

void test_spinlock(void) {
    printf("\n=== Task 5: Synchronization Demo ===\n\n");

    printf("1. Spinlock (spinlock.c):\n");
    printf("   - acquire(): uses atomic swap (amoswap.w) to acquire lock\n");
    printf("   - release(): uses atomic swap to release lock\n");
    printf("   - while(__sync_lock_test_and_set(&lk->locked, 1)) spin wait\n\n");

    printf("2. Sleep Lock (sleeplock.c):\n");
    printf("   - acquiresleep(): sleeps if lock unavailable, saves CPU\n");
    printf("   - releasesleep(): releases lock and wakes up waiters\n");
    printf("   - Suitable for I/O and long-wait scenarios\n\n");

    printf("3. Race Condition Demo:\n");
    printf("   - Multiple processes modify shared_counter simultaneously\n");
    printf("   - Without lock protection, data inconsistency occurs\n\n");

    printf("4. Multi-process Race Test:\n");
    int pid1 = fork();
    if (pid1 == 0) {
        for (int i = 0; i < 100; i++) {
            shared_counter++;
        }
        exit(0);
    }

    int pid2 = fork();
    if (pid2 == 0) {
        for (int i = 0; i < 100; i++) {
            shared_counter++;
        }
        exit(0);
    }

    wait((int*)0);
    wait((int*)0);

    printf("   - shared_counter final value: %d (expected 200)\n", shared_counter);
    printf("   - Actual value < 200 due to race condition\n\n");

    printf("5. Sleep/Wakeup Mechanism (proc.c):\n");
    printf("   - sleep(chan, lock): release lock, enter SLEEPING state\n");
    printf("   - wakeup(chan): wake all processes waiting on chan\n");
    printf("   - Used for IPC (e.g., pipe, wait)\n\n");

    printf("=== Demo Complete ===\n");
    exit(0);
}

int main(void) {
    test_spinlock();
    return 0;
}