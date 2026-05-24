# xv6-riscv 操作系统实验项目

## 项目概述

分析并解构 xv6 RISC-V 源码，利用其源码创建能够完成以下任务的操作系统内核。项目采用增量开发方式，每个任务在上一任务基础上添加新功能。

## 项目结构

```
project/
├── ques2/    # Bootloader + 基础内核
├── ques3/    # 基于 ques2，添加内存管理
├── ques4/    # 基于 ques3，添加调度算法
├── ques5/    # 基于 ques4，添加同步机制
└── ques6/    # 基于 ques5，添加文件系统
```

---

## 任务 2: Bootloader + 基础内核

### 技术目标

1. **Bootloader**: 计算机上电后，负责初始化 CPU 状态，将操作系统内核镜像从"存储外设"加载到"物理内存"中，并跳转到内核入口。

2. **封装中断与 Printf(不依赖外部库)**: 通过直接读写内存映射 I/O（MMIO）的串口寄存器（如 UART16550），实现底层的字符输出。

3. **系统调用与控制流反转**: 利用 RISC-V 的 ecall / sret，实现用户态到内核态的上下文保存与恢复。

### 验收标准

- [ ] 内核可成功启动并进入 main()
- [ ] 实现 UART 驱动，支持字符输出
- [ ] 实现 Printf，支持 %d, %s, %x, %p 格式说明符
- [ ] 实现至少一个系统调用（如 write）
- [ ] 实现用户态到内核态的上下文切换

### 用户测试程序

`user/test_boot.c`:
```c
int main(void) {
    printf("\n=== Task 2: Bootloader + Basic Kernel Demo ===\n\n");

    // 1. Console Output Test
    printf("1. Console Output: UART MMIO register access\n\n");

    // 2. Fork System Call Test
    int pid = fork();
    if (pid == 0) {
        printf("   Child: fork return=0, pid=%d\n", getpid());
        exit(0);
    } else {
        printf("   Parent: fork return=%d\n", pid);
        wait((int*)0);
    }

    // 3. Getpid Test
    printf("3. My process ID: %d\n", getpid());

    printf("\n=== Demo Complete ===\n");
    exit(0);
}
```

---

## 任务 3: 内存管理 - 可变分区分配

### 技术目标

不引入 MMU 页表，实现纯物理内存的动态划分。使用空闲链表（Free List），实现 kalloc/kfree。划分内核区和用户区。

### 验收标准

- [ ] 实现 kinit(), kfree(), kalloc() 函数
- [ ] 使用空闲链表管理物理内存
- [ ] 实现内核区和用户区的内存划分
- [ ] 编写测试用例验证内存分配和释放

### 用户测试程序

`user/test_mem.c`:
```c
int main(void) {
    printf("\n=== Task 3: Memory Management Demo ===\n\n");

    // 1. kalloc Test
    printf("1. Physical Page Allocator (kalloc.c)\n");

    // 2. sbrk Test
    char* brk1 = sbrk(4096);
    printf("   - sbrk(4096) returns: 0x%lx\n", (uint64)brk1);
    printf("   - sbrk(0) current brk: 0x%lx\n", (uint64)sbrk(0));

    char* brk2 = sbrk(8192);
    printf("   - sbrk(8192) returns: 0x%lx\n", (uint64)brk2);

    sbrk(-4096);
    sbrk(-8192);

    printf("\n=== Demo Complete ===\n");
    exit(0);
}
```

---

## 任务 4: 调度算法 - 多任务并发与状态机

### 技术目标

定义 PCB，管理进程生命周期状态（Ready, Running, Blocked）。利用时钟中断实现抢占式调度，RR 轮转策略。

### 验收标准

- [ ] 定义 PCB 结构体，包含 PID、状态、栈指针、context
- [ ] 实现进程状态转换
- [ ] 实现时钟中断触发的抢占式调度
- [ ] 实现时间片轮转（RR）调度策略
- [ ] 编写测试用例验证多任务并发执行

### 用户测试程序

`user/test_sched.c`:
```c
int main(void) {
    printf("\n=== Task 4: Scheduler Demo ===\n\n");

    // 1. Multi-process Scheduling Test
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

    printf("\n=== Demo Complete ===\n");
    exit(0);
}
```

---

## 任务 5: 同步机制 - 并发控制与数据一致性

### 技术目标

利用原子指令（amoswap）实现自旋锁和信号量，保护临界区。

### 验收标准

- [ ] 实现自旋锁（Spinlock）
- [ ] 实现信号量（Semaphore）及 P-V 操作
- [ ] 编写测试用例验证并发修改共享变量的正确性
- [ ] 测试死锁场景

### 用户测试程序

`user/test_sync.c`:
```c
volatile int shared_counter = 0;

int main(void) {
    printf("\n=== Task 5: Synchronization Demo ===\n\n");

    // Race Condition Test
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

    printf("   - shared_counter final: %d (expected 200)\n", shared_counter);
    printf("\n=== Demo Complete ===\n");
    exit(0);
}
```

---

## 任务 6: 文件系统 - 持久化数据的结构化解析

### 技术目标

解析 FAT32 文件系统的 MBR、FAT 表，或 ELF 可执行文件的 Program Headers。

### 验收标准

- [ ] 解析 FAT32 文件系统的 MBR
- [ ] 解析 FAT 表（File Allocation Table）
- [ ] 解析 ELF 文件的 Program Headers
- [ ] 编写测试用例打印解析结果

### ELF 结构说明

```c
// ELF Header (elf.h)
struct elfhdr {
    uint magic;        // 0x464C457F ("\x7FELF")
    uchar elf[12];
    ushort type;
    ushort machine;
    uint version;
    uint64 entry;      // 程序入口地址
    uint64 phoff;      // Program Header 表偏移
    uint64 shoff;      // Section Header 表偏移
    uint flags;
    ushort ehsize;
    ushort phentsize;  // Program Header 大小
    ushort phnum;      // Program Header 数量
    ushort shentsize;
    ushort shnum;
    ushort shstrndx;
};

// Program Header
struct proghdr {
    uint32 type;       // PT_LOAD = 1
    uint32 flags;
    uint64 off;        // 文件中偏移
    uint64 vaddr;      // 虚拟地址
    uint64 paddr;      // 物理地址
    uint64 filesz;     // 文件中大小
    uint64 memsz;      // 内存中大小
    uint64 align;
};
```

### 用户测试程序

`user/test_fs.c`:
```c
#include "kernel/elf.h"

int main(void) {
    printf("\n=== Task 6: File System Demo ===\n\n");

    // ELF Parsing Test
    struct elfhdr elf;
    read(0, &elf, sizeof(elf));

    if (elf.magic != ELF_MAGIC) {
        printf("   Not a valid ELF file\n");
        exit(1);
    }

    printf("   ELF Magic: 0x%x\n", elf.magic);
    printf("   Entry Point: 0x%lx\n", elf.entry);
    printf("   Program Headers: %d\n", elf.phnum);
    printf("   Header Size: %d bytes\n", elf.phentsize);

    // Parse Program Headers
    struct proghdr ph;
    for (int i = 0; i < elf.phnum; i++) {
        lseek(0, elf.phoff + i * elf.phentsize, 0);
        read(0, &ph, sizeof(ph));

        if (ph.type == ELF_PROG_LOAD) {
            printf("   PH[%d]: vaddr=0x%lx, filesz=%ld, memsz=%ld\n",
                   i, ph.vaddr, ph.filesz, ph.memsz);
        }
    }

    printf("\n=== Demo Complete ===\n");
    exit(0);
}
```

---

## 参考实现

xv6-riscv 源码目录结构：

```
xv6-riscv-1/
├── kernel/           # 内核源码
│   ├── entry.S      # 入口点，设置栈
│   ├── start.c      # 初始化代码
│   ├── main.c       # 主函数
│   ├── uart.c       # UART 驱动
│   ├── printf.c     # 格式化输出
│   ├── trap.c       # 中断处理
│   ├── syscall.c    # 系统调用
│   ├── proc.c       # 进程管理
│   ├── proc.h       # PCB 定义
│   ├── swtch.S      # 上下文切换
│   ├── spinlock.c   # 自旋锁
│   ├── kalloc.c     # 内存分配
│   ├── fs.c         # 文件系统
│   ├── elf.h        # ELF 结构定义
│   └── vm.c         # 虚拟内存
└── user/            # 用户程序
    ├── user.h       # 用户头文件
    ├── ulib.c       # 用户库
    └── *_test.c     # 测试程序
```

## 开发流程

1. 从 xv6-riscv 源码开始，创建 `project/ques2` 目录
2. 在 `ques2` 中精简实现任务 2 的功能
3. 复制 `ques2` 到 `project/ques3`，添加内存管理功能
4. 依次类推，每个任务基于上一个任务增量开发
5. 每个任务完成后，运行测试验证功能正确性

### 代码自完备性审查

每个 ques 目录创建或修改后，必须进行自完备性审查：

1. **检查外部依赖**
   - 确保所有 `#include` 的头文件都存在于当前目录
   - 移除未使用的 `#include` 声明
   - 移除已删除文件的引用

2. **常见问题**
   - `sleeplock.h`, `fs.h`, `file.h`, `elf.h` 等文件系统相关头文件
   - 未使用的函数或变量导致的编译警告
   - 删除的函数在 `defs.h` 中仍有声明

3. **检查方法**
   ```bash
   # 在 ques 目录下检查缺失的头文件
   grep -r "^#include" kernel/*.c | sort | uniq

   # 检查 defs.h 中的声明是否存在对应实现
   grep "void.*(" kernel/defs.h
   ```

4. **自完备性标准**
   - 所有 `#include` 都能找到对应的 `.h` 文件
   - `defs.h` 中的函数声明都有对应的实现
   - 没有编译错误或链接错误

### Makefile 精简策略

每个 ques 目录使用精简的 Makefile：
- 只保留必要的编译目标
- 移除不需要的用户程序
- 精简 UPROGS 列表
- 保持与原 xv6 的兼容性

---

## 实验报告要求

每个任务完成后，需要根据 `project模板.md` 编写实验报告。注意编写时不能使用emoji表情。

### 实验报告结构

| 章节 | 内容 |
|------|------|
| 一、实验目的与任务要求 | 实验层级、核心技术目标、交付物清单 |
| 二、实验环境配置 | 宿主机、编译器、QEMU、工具链 |
| 三、源码来源与修改说明 | 使用的 xv6 源码文件清单，标记已修改的文件 |
| 四、核心设计与实现原理 | 核心数据结构、控制流图、算法逻辑 |
| 五、关键核心源码解读 | 亲自编写或魔改的核心代码及注释 |
| 六、实验结果运行演示 | QEMU 运行截图、测试程序输出 |
| 七、实验总结与防坑反思 | Bug 调试过程、进阶拓展说明 |

### 报告编写要点

1. **源码来源说明**
   - 列出所有使用的 xv6 源码文件
   - 用 **已修改** 标注本任务中修改的文件
   - 说明修改原因和内容

2. **核心代码注释**
   - 只粘贴最核心的几十行代码
   - 添加逐行或逐段的中文注释
   - 说明关键设计决策

3. **运行结果截图**
   - `make qemu` 编译启动截图
   - 测试程序输出截图

### 报告模板

详细模板请参考 `project模板.md` 文件。

### 注意事项

- 不要粘贴成百上千行的冗长源码
- 突出显示**亲自编写或魔改**的代码
- 主动写出可能的面试问题及答案（防坑反思）