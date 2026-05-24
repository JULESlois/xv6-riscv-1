# 《操作系统设计与实现》实验报告

**实验编号/层级**：实验 X —— [例如：层级2：Bootloader + 基础内核]

---

## 一、 实验目的与任务要求

### 1. 内核实验层级

本实验属于课程规划的第 **X** 层级。

### 2. 核心技术目标

* 明确本层级要解决的 OS 核心矛盾
* 预期在 Qemu 模拟器中达到的运行效果

### 3. 交付物清单

* 内核魔改/实现源码
* 本说明文档
* 运行演示视频

---

## 二、 实验环境配置

| 配置项 | 说明 |
|--------|------|
| 宿主机操作系统 | Windows 11 / Ubuntu 22.04 LTS |
| 编译器与构建工具 | GCC 11.X, GNU Make, Perl |
| 硬件模拟器 | Qemu System RISC-V 64 |
| 基础内核蓝本 | MIT xv6-riscv |
| RISC-V 工具链 | riscv64-unknown-elf-gcc |

---

## 三、 源码来源与修改说明

### 1. 基础源码

本实验基于 **MIT xv6-riscv** 操作系统源码，原始代码可从以下地址获取：

```
https://github.com/mit-pdos/xv6-riscv
```

### 2. 使用的核心源码文件

| 源文件 | 功能说明 | 源码来源 |
|--------|----------|----------|
| `kernel/entry.S` | 内核入口点，设置初始栈 | xv6 原始 |
| `kernel/start.c` | M 模式初始化，切换到 S 模式 | xv6 原始 |
| `kernel/main.c` | 内核主函数，初始化各子系统 | **已修改** |
| `kernel/console.c` | 控制台驱动，字符输出 | xv6 原始 |
| `kernel/uart.c` | UART 16550 MMIO 驱动 | xv6 原始 |
| `kernel/printf.c` | 格式化输出 printf | xv6 原始 |
| `kernel/kalloc.c` | 物理内存页分配器 | xv6 原始 |
| `kernel/vm.c` | 虚拟内存/页表管理 | xv6 原始 |
| `kernel/proc.c` | 进程管理，PCB，调度器 | **已修改** |
| `kernel/proc.h` | 进程控制块结构体定义 | **已修改** |
| `kernel/trap.c` | 中断/异常处理 | xv6 原始 |
| `kernel/syscall.c` | 系统调用分发 | xv6 原始 |
| `kernel/sysproc.c` | 进程相关系统调用 | xv6 原始 |
| `kernel/swtch.S` | 上下文切换汇编 | xv6 原始 |
| `kernel/spinlock.c` | 自旋锁实现 | xv6 原始 |
| `kernel/trampoline.S` | 用户/内核态切换 trampoline | xv6 原始 |
| `kernel/riscv.h` | RISC-V CSR 寄存器访问宏 | xv6 原始 |
| `kernel/memlayout.h` | 物理内存布局定义 | xv6 原始 |
| `kernel/param.h` | 内核参数定义 | xv6 原始 |
| `kernel/types.h` | 基本类型定义 | xv6 原始 |
| `kernel/defs.h` | 内核函数声明 | **已修改** |
| `kernel/kernelvec.S` | 内核中断向量 | xv6 原始 |
| `kernel/kernel.ld` | 内核链接脚本 | xv6 原始 |

### 3. 用户程序源码

| 源文件 | 功能说明 |
|--------|----------|
| `user/init.c` | 第一个用户进程 |
| `user/test_*.c` | 各任务测试程序 |
| `user/ulib.c` | 用户库（memcpy, strcmp 等） |
| `user/umalloc.c` | 用户内存分配器 |
| `user/printf.c` | 用户态 printf |

### 4. 主要修改内容

> 记录本实验中**修改或新增**的代码及其作用

| 文件 | 修改类型 | 修改说明 |
|------|----------|----------|
| `kernel/main.c` | 修改 | 精简初始化流程，移除文件系统、virtio 等本任务不需要的模块 |
| `kernel/defs.h` | 修改 | 删除不需要的函数声明，保持与精简后代码一致 |
| `kernel/proc.h` | 修改 | 根据任务需求增删 PCB 字段 |
| `kernel/proc.c` | 修改 | 根据任务需求修改调度逻辑 |

---

## 四、 操作系统内核核心设计与实现原理

### 1. 核心数据结构设计

```c
// 示例：修改后的进程控制块 (PCB)
struct proc {
    uint64 sz;                  // 进程内存大小
    pagetable_t pagetable;     // 用户页表指针
    enum procstate state;       // 进程状态
    int pid;                    // 进程号
    struct trapframe *trapframe;// 中断上下文
    // ...
};
```

### 2. 关键控制流与算法逻辑

**特权级切换流**：
```
用户程序 → ecall → trampoline.S 压栈 → trap.c → syscall.c 分发 → 内核函数 → sret 返回
```

---

## 五、 关键核心源码解读

> 只摘录**亲自编写、魔改或最核心的几十行代码**

```c
// 核心代码注释
void scheduler(void) {
    // ...
}
```

---

## 六、 实验结果运行演示与分析

### 1. Qemu 编译及启动追踪

```bash
$ cd project/ques2
$ make qemu
```

### 2. 测试程序运行结果

展示测试程序的输出结果。

---

## 七、 实验总结与防坑反思

### 1. 遇到的主要 Bug 及调试过程

* Bug 描述与解决方案

### 2. 个人进阶/高端拓展说明（加分项）

* 指出独立编写的高级功能

---

### 💡 冲分小贴士

在写完报告后，**闭上眼睛对着关键源码盲猜三个可能被老师随堂提问的点**，并主动在第六部分写出反思。