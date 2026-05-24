struct context;
struct proc;
struct spinlock;

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);

// console.c
void            consoleinit(void);
void            consputc(int);

// printf.c
int             printf(char*, ...) __attribute__ ((format (printf, 1, 2)));
void            panic(char*) __attribute__((noreturn));
void            printfinit(void);

// proc.c
int             cpuid(void);
int             kfork(void);
int             growproc(int);
struct cpu*     mycpu(void);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             kwait(uint64);
void            wakeup(void*);
void            yield(void);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c
void            argint(int, int*);
int             argstr(int, char*, int);
void            argaddr(int, uint64 *);
int             fetchstr(uint64, char*, int);
int             fetchaddr(uint64, uint64*);
void            syscall();

// trap.c
extern uint     ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            prepare_return(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartwrite(char [], int);
void            uartputc_sync(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);
int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
uint64          uvmalloc(pagetable_t, uint64, uint64, int);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
pte_t *         walk(pagetable_t, uint64, int);
uint64          walkaddr(pagetable_t, uint64);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))