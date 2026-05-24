// test_boot.c - Demo for Bootloader + Basic Kernel
// Task 2: Bootloader + Basic Kernel

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"

int main(void) {
    printf("\n=== Task 2: Bootloader + Basic Kernel Demo ===\n\n");

    printf("1. Console Output Test:\n");
    printf("   - kernel/console.c:34-42: consputc() outputs char via UART register\n");
    printf("   - kernel/printf.c: printf formatted output\n\n");

    printf("2. Fork System Call Test:\n");
    int pid = fork();
    if (pid == 0) {
        printf("   - Child: fork return=0, pid=%d\n", getpid());
        printf("   - kernel/sysproc.c:26-28: sys_fork() calls kfork()\n");
        printf("   - kernel/proc.c: create new proc, copy pagetable and trapframe\n");
        exit(0);
    } else {
        printf("   - Parent: fork return=%d (child pid)\n", pid);
        wait((int*)0);
    }

    printf("3. Getpid System Call Test:\n");
    printf("   - My process ID: %d\n", getpid());
    printf("   - kernel/sysproc.c:20-23: sys_getpid() returns myproc()->pid\n\n");

    printf("4. Sbrk Memory Allocation Test:\n");
    char* old_brk = sbrk(4096);
    if (old_brk != (char*)-1) {
        printf("   - Old brk: %p\n", old_brk);
        printf("   - New brk: %p\n", sbrk(0));
        printf("   - kernel/sysproc.c:40-65: sys_sbrk() calls growproc()\n");
        sbrk(-4096);
    }

    printf("\n=== Demo Complete ===\n");
    printf("\nControl Flow:\n");
    printf("  user code -> usys.S (syscall) -> trampoline.S -> trap.c\n");
    printf("  -> syscall.c (dispatch) -> sysproc.c (syscall impl) -> usertrapret\n\n");

    exit(0);
}