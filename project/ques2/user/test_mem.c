// test_mem.c - Demo for Memory Management
// Task 3: Memory Management - Variable Partition Allocation

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"

void test_kalloc(void) {
    printf("\n=== Task 3: Memory Management Demo ===\n\n");

    printf("1. Physical Page Allocator (kalloc.c):\n");
    printf("   - kinit(): init freelist, range [end, PHYSTOP]\n");
    printf("   - kalloc(): get page from freelist head (4096 bytes)\n");
    printf("   - kfree(): return page to freelist head\n");
    printf("   - Freelist structure: struct run { struct run *next; }\n\n");

    printf("2. Sbrk Memory Allocation Test:\n");
    char* brk1 = sbrk(4096);
    printf("   - First sbrk(4096) returns: 0x%lx\n", (uint64)brk1);
    char* brk2 = sbrk(0);
    printf("   - sbrk(0) current brk: 0x%lx\n", (uint64)brk2);
    printf("   - Increase: %d bytes\n", (int)(brk2 - brk1));

    char* brk3 = sbrk(8192);
    printf("   - Second sbrk(8192) returns: 0x%lx\n", (uint64)brk3);
    char* brk4 = sbrk(0);
    printf("   - sbrk(0) current brk: 0x%lx\n", (uint64)brk4);
    printf("   - Increase: %d bytes\n", (int)(brk4 - brk3));

    printf("\n3. Virtual Memory Management (vm.c):\n");
    printf("   - kvmmake(): create kernel page table, direct map all physical memory\n");
    printf("   - kvmmap(kpgtbl, va, pa, sz, perm): add page table mapping\n");
    printf("   - walk(pagetable, va, alloc): traverse 3-level page table to find PTE\n");
    printf("   - uvmalloc(pagetable, oldsz, newsz, perm): allocate user memory\n\n");

    printf("4. Process Memory Layout:\n");
    printf("   - TRAPFRAME (0x3ffffff000): trap frame page\n");
    printf("   - TRAMPOLINE (0x3ffffff000): trampoline page\n");
    printf("   - User heap: grows up from end of program\n");
    printf("   - User stack: below TRAPFRAME\n\n");

    sbrk(-4096);
    sbrk(-8192);

    printf("5. Memory Control Flow:\n");
    printf("   - sys_sbrk() -> growproc() -> uvmalloc()/uvmdealloc()\n");
    printf("   - uvmalloc() -> walk() -> mappages() -> kalloc()\n");
    printf("   - Kernel page table direct maps: [KERNBASE, PHYSTOP]\n\n");

    printf("=== Demo Complete ===\n");
    exit(0);
}

int main(void) {
    test_kalloc();
    return 0;
}