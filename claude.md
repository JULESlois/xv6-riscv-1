# xv6-riscv 开发计划

## 项目概述

本项目基于 xv6 RISC-V 源码，重写并实现一个可启动的操作系统内核。采用增量开发方式，从基础 Bootloader 到完整文件系统，共 6 个任务逐步演进。

**运行环境**: ARM64 主机，QEMU 模拟 RISC-V64
**编译命令**: `make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64`

---

## 项目结构

```
project/
├── ques2/    # Bootloader + 基础内核
├── ques3/    # 基于 ques2，添加内存管理
├── ques4/    # 基于 ques3，添加调度算法
├── ques5/    # 基于 ques4，添加同步机制
└── ques6/    # 基于 ques5，添加文件系统
```

**增量开发原则**: 每个任务基于上一个任务增量开发，直接复制上一个项目目录，然后增加新功能代码。

---

## 任务清单与依赖关系

| 任务 | 目录 | 依赖 | 核心功能 |
|------|------|------|----------|
| Task 2 | ques2 | 无 | Bootloader + UART + printf + 基本系统调用 |
| Task 3 | ques3 | ques2 | 内存管理 - 可变分区分配 (kalloc/kfree) |
| Task 4 | ques4 | ques3 | 调度算法 - 抢占式 RR 调度 |
| Task 5 | ques5 | ques4 | 同步机制 - 自旋锁 + 信号量 |
| Task 6 | ques6 | ques5 | 文件系统 - FAT32/ELF 解析 |

---

## 任务 2: Bootloader + 基础内核

### 验收标准
- [ ] 内核可成功启动并进入 main()
- [ ] 实现 UART 驱动，支持字符输出
- [ ] 实现 Printf，支持 %d, %s, %x, %p 格式说明符
- [ ] 实现至少一个系统调用（如 write）
- [ ] 实现用户态到内核态的上下文切换

### 需要实现/重写的文件

**kernel/ 目录**:
- `entry.S` - 内核入口点，设置初始栈
- `start.c` - 机器模式初始化代码
- `main.c` - 主函数，调用各初始化函数
- `uart.c` - UART16550 MMIO 驱动
- `printf.c` - 格式化输出实现
- `trap.c` - 中断/异常处理框架
- `syscall.c` - 系统调用处理
- `sysproc.c` - 进程相关系统调用 (fork, exit, wait, getpid)
- `proc.c` - 进程管理基础结构
- `proc.h` - PCB 定义
- `swtch.S` - 上下文切换
- `spinlock.c` - 自旋锁基础
- `kalloc.c` - 物理页分配器（简化版）
- `defs.h` - 内核函数声明
- `types.h` - 类型定义
- `param.h` - 内核参数
- `memlayout.h` - 内存布局
- `riscv.h` - RISC-V 相关定义

**user/ 目录**:
- `user.h` - 用户头文件
- `ulib.c` - 用户库 (printf, memset, memcpy, strcmp, strlen)
- `printf.c` - 用户态 printf
- `usys.pl` - 系统调用桩生成
- `test_boot.c` - 测试程序

### 关键实现细节

#### 1. entry.S 入口点
```asm
# 设置初始栈指针，跳转到 start.c
.globl _start
_start:
    la sp, stack
    call start
```

#### 2. UART MMIO 寄存器 (uart.c)
```c
// UART 基地址 (QEMU virt machine)
#define UART0 0x10000000
// 发送寄存器偏移
#define LSR 5
#define LSR_TX_IDLE 0x20
// 轮询方式发送字符
```

#### 3. Printf 实现 (printf.c)
- 支持格式说明符: %d, %s, %x, %p
- 不依赖外部库，自己实现 va_list
- 通过 UART 输出

#### 4. 系统调用约定
- RISC-V ecall 触发系统调用
- a0 = 系统调用号, a1-a6 = 参数
- 返回值通过 a0 传递

#### 5. 上下文切换 (swtch.S)
- 保存/恢复 callee-saved 寄存器
- 用于进程切换和系统调用返回

### 测试运行
```bash
cd project/ques2
make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64
# 期望输出 test_boot.c 的测试结果
```

---

## 任务 3: 内存管理

### 验收标准
- [ ] 实现 kinit(), kfree(), kalloc() 函数
- [ ] 使用空闲链表管理物理内存
- [ ] 实现内核区和用户区的内存划分
- [ ] 编写测试用例验证内存分配和释放

### 在 ques2 基础上新增/修改

**kernel/ 目录**:
- `kalloc.c` - 完整实现空闲链表分配器
- `vm.c` - 简化的虚拟内存管理（可选）

**user/ 目录**:
- `test_mem.c` - 内存管理测试

### 关键实现细节

#### 空闲链表分配器 (kalloc.c)
```c
struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;

// kinit() - 初始化空闲链表
// kalloc() - 从空闲链表取出一页
// kfree() - 将一页放回空闲链表
```

#### 内存布局
- 内核区: 0x80000000 开始
- 用户区: 最高地址向下扩展

#### sbrk 实现
- 增加/减少进程的堆指针
- 按页对齐分配物理内存

### 测试运行
```bash
cd project/ques3
make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64
# 期望输出 test_mem.c 的测试结果
```

---

## 任务 4: 调度算法

### 验收标准
- [ ] 定义 PCB 结构体，包含 PID、状态、栈指针、context
- [ ] 实现进程状态转换
- [ ] 实现时钟中断触发的抢占式调度
- [ ] 实现时间片轮转（RR）调度策略
- [ ] 编写测试用例验证多任务并发执行

### 在 ques3 基础上新增/修改

**kernel/ 目录**:
- `proc.c` - 扩展进程管理，增加调度器
- `proc.h` - 扩展 PCB 定义
- `trap.c` - 增加时钟中断处理
- `sysproc.c` - 增加 sched_yield 系统调用

**user/ 目录**:
- `test_sched.c` - 调度测试

### 关键实现细节

#### PCB 状态机
```
UNUSED -> USED -> RUNNABLE -> RUNNING -> RUNNABLE
                      |           |
                      v           v
                   SLEEPING    ZOMBIE
```

#### 时钟中断
- RISC-V SIE 寄存器使能时钟中断
- 每隔固定时间片触发时钟中断
- 时钟中断处理中调用 scheduler()

#### RR 调度器
```c
void scheduler(void) {
    struct proc *p;
    for(;;) {
        for(p = proc; p < &proc[NPROC]; p++) {
            if(p->state == RUNNABLE) {
                p->state = RUNNING;
                swtch(&c->context, &p->context);
            }
        }
    }
}
```

---

## 任务 5: 同步机制

### 验收标准
- [ ] 实现自旋锁（Spinlock）
- [ ] 实现信号量（Semaphore）及 P-V 操作
- [ ] 编写测试用例验证并发修改共享变量的正确性
- [ ] 测试死锁场景

### 在 ques4 基础上新增/修改

**kernel/ 目录**:
- `spinlock.c` - 完善自旋锁实现
- `spinlock.h` - 自旋锁结构
- `proc.c` - 增加 sleep/wakeup 实现

**user/ 目录**:
- `test_sync.c` - 同步测试

### 关键实现细节

#### 自旋锁实现
```c
// 使用 amoswap 原子指令
void acquire(struct spinlock *lk) {
    while(__sync_lock_test_and_set(&lk->locked, 1))
        ;
    __sync_synchronize();
}

void release(struct spinlock *lk) {
    __sync_synchronize();
    __sync_lock_release(&lk->locked, 0);
}
```

#### 信号量实现
```c
struct semaphore {
    int value;
    struct spinlock lock;
    struct proc *queue;
};

void P(struct semaphore *s) {
    acquire(&s->lock);
    while(s->value <= 0) {
        sleep(s, &s->lock);
    }
    s->value--;
    release(&s->lock);
}

void V(struct semaphore *s) {
    acquire(&s->lock);
    s->value++;
    wakeup(s);
    release(&s->lock);
}
```

---

## 任务 6: 文件系统

### 验收标准
- [ ] 解析 FAT32 文件系统的 MBR
- [ ] 解析 FAT 表（File Allocation Table）
- [ ] 解析 ELF 文件的 Program Headers
- [ ] 编写测试用例打印解析结果

### 在 ques5 基础上新增/修改

**kernel/ 目录**:
- `fs.c` - 文件系统实现
- `elf.h` - ELF 结构定义
- `bio.c` - 块设备缓存

**user/ 目录**:
- `test_fs.c` - 文件系统测试

### 关键实现细节

#### ELF 结构
```c
struct elfhdr {
    uint magic;        // 0x464C457F
    uchar elf[12];
    ushort type;
    ushort machine;
    uint version;
    uint64 entry;
    uint64 phoff;
    uint64 shoff;
    uint flags;
    ushort ehsize;
    ushort phentsize;
    ushort phnum;
    ushort shentsize;
    ushort shnum;
    ushort shstrndx;
};

struct proghdr {
    uint32 type;
    uint32 flags;
    uint64 off;
    uint64 vaddr;
    uint64 paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};
```

---

## 开发流程

### 步骤 1: 创建项目结构
```bash
cd /data/data/com.termux/files/home/xv6-riscv-1
mkdir -p project/ques{2,3,4,5,6}
```

### 步骤 2: 实现任务 2 (基础内核)
```bash
cp -r . project/ques2
# 删除不必要的文件，保留核心框架
# 实现 UART、printf、基本系统调用
```

### 步骤 3: 增量开发
```bash
# 每个任务复制上一个项目
cp -r project/ques2 project/ques3
# 在 ques3 基础上添加内存管理
```

### 步骤 4: 自完备性审查
每个 ques 目录完成后，必须验证：
1. 所有 `#include` 头文件存在
2. `defs.h` 中的声明都有对应实现
3. 没有编译错误或链接错误

```bash
# 检查依赖
grep -r "^#include" kernel/*.c | sort | uniq
# 编译测试
make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64
```

---

## 重要注意事项

1. **代码注释**: 所有代码必须添加详细注释，说明每一行的作用
2. **不依赖外部库**: printf、memcpy、strlen 等必须自己实现
3. **增量开发**: 每个任务基于上一个任务，禁止跳跃式开发
4. **测试验证**: 每个任务完成后必须运行测试验证功能正确性
5. **代码自完备**: 不引入不必要的依赖，确保可独立编译运行

---

## 参考资料

- RISC-V 指令集手册
- xv6-riscv 原始源码 (kernel/, user/)
- project.md - 项目详细需求
