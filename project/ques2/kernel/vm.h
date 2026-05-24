#ifndef XV6_VM_H
#define XV6_VM_H

#define SBRK_EAGER 1
#define SBRK_LAZY  2

typedef uint64 pte_t;
typedef uint64 *pagetable_t;  // 512 PTEs

#define PGSIZE 4096
#define PGROUNDUP(s) (((s) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

#define PTE_V 0x001  // valid
#define PTE_R 0x002  // readable
#define PTE_W 0x004  // writable
#define PTE_X 0x008  // executable
#define PTE_U 0x010  // user accessible

#define PTE_FLAGS(pte) ((pte) & 0x1FF)

#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

#endif