// test_sched.c - Demo for Scheduler Algorithm
// Task 4: Scheduler Algorithm - Multi-tasking Concurrency and State Machine

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"

void test_scheduler(void) {
    printf("\n=== Task 4: Scheduler Algorithm Demo ===\n\n");

    printf("1. Scheduler Core (proc.c:425-463):\n");
    printf("   - scheduler() runs in infinite loop, selects RUNNABLE process\n");
    printf("   - Uses swtch() to switch context to selected process\n");
    printf("   - Process yields CPU via yield() or clock interrupt\n\n");

    printf("2. Process State Machine:\n");
    printf("   - UNUSED: not in use\n");
    printf("   - USED: allocated\n");
    printf("   - RUNNABLE: ready to be scheduled\n");
    printf("   - RUNNING: currently executing on CPU\n");
    printf("   - SLEEPING: waiting for I/O or event\n");
    printf("   - ZOMBIE: exited but parent hasn't reaped\n\n");

    printf("3. Context Switch (swtch.S):\n");
    printf("   - Save callee-saved registers (s0-s11, sp, ra)\n");
    printf("   - Restore new process's register state\n");
    printf("   - struct context { ra, sp, s0-s11 }\n\n");

    printf("4. Time Slice Round-Robin:\n");
    printf("   - Clock interrupt (1000000 ticks = 0.1s) triggers yield()\n");
    printf("   - trap.c:84-85: if(which_dev == 2) yield();\n\n");

    printf("5. Multi-process Scheduling Test:\n");
    int pid1 = fork();
    if (pid1 == 0) {
        printf("   Child 1 (pid=%d) started\n", getpid());
        exit(0);
    }

    int pid2 = fork();
    if (pid2 == 0) {
        printf("   Child 2 (pid=%d) started\n", getpid());
        exit(0);
    }

    int pid3 = fork();
    if (pid3 == 0) {
        printf("   Child 3 (pid=%d) started\n", getpid());
        exit(0);
    }

    wait((int*)0);
    wait((int*)0);
    wait((int*)0);

    printf("\n6. Scheduler Control Flow:\n");
    printf("   - Clock interrupt -> kerneltrap() -> yield() -> sched()\n");
    printf("   - yield() sets state=RUNNABLE, calls sched()\n");
    printf("   - sched() calls swtch(&p->context, &c->context)\n");
    printf("   - Switch to scheduler context, select next process\n\n");

    printf("=== Demo Complete ===\n");
    exit(0);
}

int main(void) {
    test_scheduler();
    return 0;
}