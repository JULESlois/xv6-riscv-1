# xv6-riscv 操作系统内核项目报告

## 1. 项目目标与验收口径

本项目基于 xv6-riscv 源码完成 `project/ques2` 到 `project/ques6` 五个阶段的操作系统内核实现。最终目标不是写几个内核打印函数，也不是把用户测试文件编译出来但不运行，而是形成一套能在 QEMU RISC-V64 上真实启动、真实装载用户程序、真实执行系统调用的教学操作系统。

本项目的验收口径如下：

- QEMU 加载真实 RISC-V 内核镜像 `kernel/kernel`。
- 内核完成 console、printf、物理页分配、页表、trap、syscall、进程、调度、virtio 磁盘、buffer cache、log、inode、file、exec 等核心模块初始化。
- `mkfs` 生成真实 `fs.img`，并把 `/init` 与用户程序放入文件系统镜像。
- 内核第一个用户进程通过 `kexec("/init")` 进入用户态。
- 用户态 `init` 通过 `fork + exec + wait` 运行任务程序。
- 用户程序通过 `ecall` 进入内核，经过 `trap -> syscall -> sys_*` 调用真实内核服务。
- 文件系统、ELF 装载、进程切换、同步阻塞等行为都由 xv6 内核路径完成。

## 2. xv6 源码拆解方法

为了把项目从“演示内核”改造成“能真实运行用户程序的内核”，我先按控制流拆解 xv6，而不是按文件名机械复制。拆解时把源码分成八条主线：启动、输出、内存、trap/系统调用、进程调度、文件系统、exec 装载、用户态工具链。

### 2.1 启动链路

启动相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/entry.S` | QEMU 进入内核后的最早入口，设置初始栈并跳转到 C 代码 |
| `kernel/start.c` | 配置 RISC-V 特权级、中断委托、timer，准备进入 supervisor mode |
| `kernel/main.c` | 内核主初始化函数，依次初始化各个子系统 |
| `kernel/memlayout.h` | 定义 UART、PLIC、virtio、kernel base、trampoline 等内存布局 |
| `kernel/riscv.h` | 封装 RISC-V CSR 读写、页表项、状态位等底层操作 |

启动链路可以概括为：

```text
QEMU
  -> kernel/entry.S
  -> kernel/start.c
  -> kernel/main.c
  -> userinit()
  -> scheduler()
```

`main()` 是整个内核的装配点。它不是单独完成所有功能，而是按依赖顺序调用各个模块初始化。例如先初始化 `consoleinit()` 和 `printfinit()`，才能可靠输出调试信息；先初始化 `kinit()` 和 `kvminit()`，才能建立内核页表；先初始化 `procinit()`，才能创建进程；先初始化 `virtio_disk_init()` 和文件系统相关模块，才能从 `fs.img` 装载 `/init`。

### 2.2 输出与调试链路

输出相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/uart.c` | 通过 MMIO 操作 UART16550 设备 |
| `kernel/console.c` | 把字符设备抽象成 console，支持输入输出 |
| `kernel/printf.c` | 内核格式化输出 |
| `user/printf.c` | 用户态格式化输出 |

这里的关键点是区分内核输出和用户输出。内核的 `printf` 最终直接调用 console/UART；用户程序的 `printf` 不是直接访问 UART，而是通过 `write` 系统调用写文件描述符 1，再由内核转到 console。这说明用户程序输出本身也验证了系统调用和文件描述符路径。

### 2.3 内存管理链路

内存相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/kalloc.c` | 物理页分配器，维护空闲页链表 |
| `kernel/vm.c` | 页表创建、映射、复制、释放和用户地址转换 |
| `kernel/proc.c` | 进程地址空间创建、复制和释放 |
| `kernel/exec.c` | 装载 ELF 时分配用户地址空间 |

xv6 内存管理分为两层：

1. 物理页管理：`kalloc()` 从空闲链表取出 4096 字节页，`kfree()` 把页归还空闲链表。
2. 虚拟内存管理：`uvmalloc()` 为用户虚拟地址分配物理页并建立页表映射，`uvmdealloc()` 收缩用户地址空间，`uvmcopy()` 在 `fork` 时复制父进程地址空间。

用户态 `sbrk()` 的调用路径是：

```text
user sbrk()
  -> ecall
  -> usertrap()
  -> syscall()
  -> sys_sbrk()
  -> growproc()
  -> uvmalloc()/uvmdealloc()
```

因此 `sbrk()` 不是简单移动一个变量，而是会改变进程的用户地址空间大小，并通过页表让用户程序能够访问新内存。

### 2.4 Trap 与系统调用链路

trap 和系统调用相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/trampoline.S` | 用户态和内核态切换时保存/恢复寄存器 |
| `kernel/trap.c` | 处理系统调用、中断和异常 |
| `kernel/syscall.c` | 根据系统调用号分派到具体 `sys_*` 函数 |
| `kernel/syscall.h` | 定义系统调用号 |
| `kernel/sysproc.c` | 进程、内存、同步等系统调用实现 |
| `kernel/sysfile.c` | 文件系统相关系统调用实现 |
| `user/usys.pl` | 生成用户态 syscall 汇编桩 |
| `user/user.h` | 声明用户态可调用接口 |

用户程序调用系统调用时，并不是直接调用内核函数。以 `fork()` 为例，实际路径是：

```text
user/fork()
  -> user/usys.S 中的 ecall
  -> trampoline.S 保存用户寄存器
  -> trap.c:usertrap()
  -> syscall.c:syscall()
  -> sysproc.c:sys_fork()
  -> proc.c:kfork()
```

这个链路是整个项目最重要的骨架。后续新增 semaphore 系统调用时，也必须同时修改 `syscall.h`、`syscall.c`、`sysproc.c`、`defs.h`、`user.h`、`usys.pl`，否则用户态和内核态无法闭合。

### 2.5 进程与调度链路

进程与调度相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/proc.h` | 定义 `struct proc`、`struct cpu`、进程状态等 |
| `kernel/proc.c` | 进程分配、fork、exit、wait、sleep/wakeup、scheduler |
| `kernel/swtch.S` | 保存/恢复内核上下文 |
| `kernel/trap.c` | 时钟中断到来时触发 `yield()` |

xv6 的进程状态包括：

```text
UNUSED -> USED -> RUNNABLE -> RUNNING
                         |        |
                         v        v
                      SLEEPING  ZOMBIE
```

调度核心路径是：

```text
timer interrupt
  -> usertrap()
  -> yield()
  -> sched()
  -> swtch()
  -> scheduler()
  -> next RUNNABLE process
```

`scheduler()` 常驻每个 CPU 的内核线程中，不断扫描进程表。发现 `RUNNABLE` 进程后，把它切换为 `RUNNING`，再通过 `swtch()` 切换到该进程保存的上下文。进程主动 `yield()`、阻塞 `sleep()` 或退出 `exit()` 时，又通过 `sched()` 切回调度器。

### 2.6 文件系统与磁盘链路

文件系统相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/virtio_disk.c` | virtio block 设备驱动，负责读写 QEMU 磁盘 |
| `kernel/bio.c` | buffer cache，缓存磁盘块 |
| `kernel/log.c` | 文件系统日志，保证元数据更新一致性 |
| `kernel/fs.c` | inode、目录、路径解析、块映射 |
| `kernel/file.c` | 打开文件表与文件读写抽象 |
| `kernel/sysfile.c` | `open/read/write/close/fstat` 等系统调用 |
| `mkfs/mkfs.c` | 在宿主机上生成 xv6 文件系统镜像 `fs.img` |

读文件路径可以概括为：

```text
user read(fd, buf, n)
  -> sys_read()
  -> fileread()
  -> readi()
  -> bmap()
  -> bread()
  -> virtio_disk_rw()
```

这条路径把用户态文件描述符、内核打开文件表、inode、buffer cache 和 virtio 磁盘驱动串在一起。`ques6` 能从 `fs.img` 读取 `test_fs` 和 `README`，说明这条路径是完整工作的。

### 2.7 Exec 与 ELF 装载链路

`exec` 相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `kernel/exec.c` | 读取 ELF 文件，建立新用户地址空间 |
| `kernel/elf.h` | 定义 ELF Header 和 Program Header |
| `kernel/vm.c` | 为 ELF 段分配和映射用户页 |
| `kernel/proc.c` | 保存进程 trapframe，切换到新程序入口 |

`exec("test_fs", argv)` 的核心流程是：

```text
namei("test_fs")
  -> readi() 读取 ELF header
  -> 检查 ELF_MAGIC
  -> 遍历 Program Header
  -> uvmalloc() 分配用户虚拟地址
  -> loadseg() 把文件段读入用户内存
  -> 建立用户栈并复制 argv
  -> 更新 trapframe->epc 为 ELF entry
```

因此，用户程序能被 `exec` 运行，本身就证明了文件系统、ELF 解析、页表、用户栈和 trap 返回路径共同工作。

### 2.8 用户态构建与文件系统镜像

用户程序相关源码主要包括：

| 文件 | 作用 |
|------|------|
| `user/user.ld` | 用户程序链接脚本 |
| `user/ulib.c` | 用户态基础库 |
| `user/printf.c` | 用户态格式化输出 |
| `user/usys.pl` | 生成 syscall 汇编入口 |
| `Makefile` | 编译 kernel、user programs、mkfs，并生成 `fs.img` |

每个用户程序会被编译成形如 `user/_test_fs` 的 ELF 文件。`mkfs/mkfs` 会把这些文件写入 xv6 文件系统镜像。镜像内文件名会去掉前导下划线，所以用户态执行的是 `test_fs`，而宿主机上的文件是 `user/_test_fs`。

## 3. 总体实现

各任务目录都采用完整 xv6-riscv 内核结构，而不是演示桩：

| 目录 | 实现重点 |
|------|----------|
| `project/ques2` | 整合基础 xv6 启动、UART、printf、trap、syscall、进程和 exec 路径 |
| `project/ques3` | 在完整 xv6 基础上验证并保留页分配、用户页表和 `sbrk` 内存增长能力 |
| `project/ques4` | 保留 xv6 抢占式调度、上下文切换、进程状态机和 pipe 进程通信 |
| `project/ques5` | 在系统调用层新增 semaphore，同步语义由内核 `sleep/wakeup` 实现 |
| `project/ques6` | 保留完整文件系统和 ELF 装载链路，并加入交互 shell 便于直接运行用户程序 |

每个目录的 `Makefile` 都会构建 `kernel/kernel`、用户 ELF、`mkfs/mkfs` 和 `fs.img`。`ques2` 到 `ques5` 的 `UPROGS` 主要保留 `_init` 加当前任务程序，便于验收日志聚焦；`ques6` 额外加入 `_sh/_ls/_cat/_echo/_mkdir/_rm` 等常用用户程序，便于在文件系统初始化后进入 shell 手动运行用户程序。

## 4. 分阶段实现过程

### 4.1 Task 2：Bootloader、基础内核与系统调用

Task 2 的目标是让内核不只进入 `main()`，还要能切换到用户态并执行用户程序。为此保留并整理了 xv6 的启动、console、trap、syscall、proc、exec 和文件系统最小闭环。

核心实现点如下：

- `entry.S` 设置启动栈，把控制权交给 `start.c`。
- `start.c` 配置 RISC-V 特权级和中断委托，准备进入 supervisor mode。
- `main.c` 依次初始化 console、printf、物理内存、页表、trap、进程、virtio、文件系统。
- `userinit()` 不再只是构造一个内嵌 initcode，而是通过 `kexec("/init")` 从文件系统执行真实 `/init`。
- `init.c` 打开或创建 `console`，复制标准输入、输出、错误三个文件描述符，然后通过 `fork + exec + wait` 启动任务程序。

Task 2 拆解后的关键认识是：所谓“基础内核”不能只包含 UART 和 printf。如果要真实运行用户程序，至少还需要 trap、syscall、proc、vm、exec、fs、file、virtio 等模块形成闭环。

### 4.2 Task 3：内存管理

Task 3 的核心不是单独写一个数组分配器，而是接入 xv6 原有页式内存管理体系。拆解时把内存管理分成物理页分配、内核页表、用户页表、用户堆增长四层。

实现保留和使用的关键函数包括：

| 函数 | 作用 |
|------|------|
| `kinit()` | 初始化物理空闲页链表 |
| `kalloc()` | 分配一页物理内存 |
| `kfree()` | 释放一页物理内存 |
| `kvminit()` | 建立内核页表 |
| `uvmalloc()` | 为用户虚拟地址分配并映射物理页 |
| `uvmdealloc()` | 收缩用户地址空间 |
| `growproc()` | 修改当前进程用户内存大小 |
| `sys_sbrk()` | 用户态 `sbrk()` 对应的系统调用 |

`sbrk()` 的实现依赖 `myproc()->sz` 记录进程当前用户地址空间大小。增长时调用 `growproc(n)`，最终进入 `uvmalloc()` 分配页并建立映射；收缩时进入 `uvmdealloc()` 解除映射并释放物理页。

这里的实现重点是保持 xv6 的虚拟内存语义：用户程序看到的是虚拟地址，内核负责通过页表把它映射到物理页。这样后续 `fork()`、`exec()`、用户栈、文件读取缓冲区等功能都能共用同一套内存机制。

### 4.3 Task 4：进程调度

Task 4 拆解的是 xv6 的进程生命周期与调度器。实现时没有另写一个独立调度 demo，而是保留真实进程表、真实 `swtch.S`、真实 timer trap 和真实 `scheduler()`。

关键结构是 `struct proc`，其中包括：

| 字段 | 作用 |
|------|------|
| `state` | 进程状态，如 `RUNNABLE/RUNNING/SLEEPING/ZOMBIE` |
| `pid` | 进程号 |
| `parent` | 父进程指针，用于 `wait()` 和 reparent |
| `kstack` | 进程内核栈地址 |
| `pagetable` | 用户页表 |
| `trapframe` | 用户寄存器保存区 |
| `context` | 内核上下文，由 `swtch()` 保存恢复 |
| `chan` | 睡眠等待通道 |
| `xstate` | 退出状态 |

进程创建、运行和退出的路径如下：

```text
fork()
  -> allocproc()
  -> uvmcopy()
  -> 设置 trapframe
  -> RUNNABLE
  -> scheduler()
  -> RUNNING
  -> exit()
  -> ZOMBIE
  -> wait() 回收
```

调度实现依赖两个关键点：

1. `yield()` 把当前进程状态从 `RUNNING` 改回 `RUNNABLE`，然后调用 `sched()`。
2. `sched()` 通过 `swtch(&p->context, &mycpu()->context)` 切回 CPU 的调度器上下文。

时钟中断来自 `trap.c`。当用户进程运行时发生 timer interrupt，内核会在 trap 路径中调用 `yield()`，从而实现抢占式调度。

### 4.4 Task 5：同步机制与信号量系统调用

Task 5 在 xv6 已有自旋锁、sleep/wakeup 和系统调用机制上新增 semaphore。这里没有把信号量写成用户态变量，因为用户进程之间地址空间隔离，普通全局变量不能自然共享。因此信号量表放在内核中，由系统调用访问。

新增文件：

| 文件 | 作用 |
|------|------|
| `project/ques5/kernel/semaphore.h` | 定义 semaphore 表大小和结构 |
| `project/ques5/kernel/semaphore.c` | 实现 `sem_init/sem_p/sem_v/sem_value/sem_try_p` 和共享计数器服务 |

信号量结构为：

```c
struct semaphore {
  struct spinlock lock;
  int used;
  int value;
  char name[16];
};
```

字段含义：

- `lock`：保护该信号量的 `used/value`。
- `used`：标记该槽位是否已经初始化。
- `value`：当前可用资源数。
- `name`：用于初始化 spinlock 时提供稳定名字。

`sem_p()` 的核心语义是：

```text
检查 id 是否有效
  -> 获取 sems[id].lock
  -> 如果 value == 0，则 sleep(&sems[id], &sems[id].lock)
  -> 被唤醒后重新检查 value
  -> value > 0 时 value--
  -> 释放锁并返回
```

这里必须使用 xv6 的 `sleep(chan, lock)`，因为它能在睡眠时原子地释放锁，并在被唤醒后重新获取锁，避免丢失唤醒。

`sem_v()` 的核心语义是：

```text
检查 id 是否有效
  -> 获取 sems[id].lock
  -> value++
  -> wakeup(&sems[id])
  -> 释放锁
```

同时实现了 `sem_try_p()`，用于在资源不足时立即返回 `-1`，避免调用者永久阻塞。

为了把 semaphore 暴露给用户态，修改了完整 syscall 链路：

| 文件 | 修改内容 |
|------|----------|
| `kernel/syscall.h` | 增加 `SYS_sem_init` 等系统调用号 |
| `kernel/syscall.c` | 增加 `extern sys_sem_*` 和 syscall table 映射 |
| `kernel/sysproc.c` | 增加用户参数解析和内核 semaphore 函数调用 |
| `kernel/defs.h` | 声明 semaphore 内核函数 |
| `kernel/main.c` | 调用 `seminit()` 初始化 semaphore 表 |
| `user/user.h` | 声明用户态 semaphore 函数 |
| `user/usys.pl` | 生成用户态 ecall 入口 |
| `Makefile` | 把 `kernel/semaphore.o` 链接进内核 |

这部分实现体现了 xv6 扩展系统调用的标准方法：用户声明、用户汇编桩、系统调用号、系统调用表、`sys_*` 包装函数、内核实际实现必须全部一致。

### 4.5 Task 6：文件系统、ELF 解析与交互用户程序

Task 6 的目标是文件系统和结构化文件解析。实现时保留完整 xv6 文件系统，而不是在内核中解析静态数组。

文件系统路径包括：

```text
open()
  -> sys_open()
  -> namei()
  -> inode
read()
  -> sys_read()
  -> fileread()
  -> readi()
  -> bread()
  -> virtio_disk_rw()
```

ELF 解析选择放在用户程序中完成。这样做有两个好处：

1. 能证明用户程序可以通过 `open/fstat/read/close` 真实访问 xv6 文件系统。
2. 不污染内核文件系统代码，保持 xv6 内核结构清晰。

`user/test_fs.c` 的实现流程是：

```text
open("test_fs")
  -> fstat() 获取完整文件大小
  -> malloc() 分配用户缓冲区
  -> read() 循环读完整文件
  -> 按小端解析 ELF Header
  -> 遍历 Program Header
  -> 打印 PT_LOAD 段
  -> 再读取 README 验证非 ELF 文件会被拒绝
```

解析时没有直接把缓冲区强转为结构体，而是使用 `le16/le32/le64` 从字节数组中读取字段。这样可以避免未对齐访问和大小端假设错误。

此外，`ques6` 的 `init.c` 已扩展为：

```text
启动 console
  -> 自动运行 test_fs
  -> 进入 shell_loop()
  -> exec("sh")
  -> shell 退出后再次拉起 shell
```

`Makefile` 中也加入了常用用户程序：

```text
_sh
_ls
_cat
_echo
_mkdir
_rm
_test_fs
```

这样 `ques6` 启动后不仅能完成文件系统验证，还能停留在真实 xv6 shell 中，直接运行用户程序。

## 5. 关键实现细节

### 5.1 为什么采用完整 xv6 基线

最初如果只围绕每个题目写单独 demo，会出现两个问题：

1. 很多功能无法真实验证。例如没有 `exec` 和文件系统时，用户程序并没有真正从 `fs.img` 被装载。
2. 后续任务依赖前序任务。例如文件系统测试依赖内存、进程、系统调用和磁盘驱动；同步测试依赖 sleep/wakeup 和调度。

因此最终采用完整 xv6-riscv 结构作为每个任务目录的基础，再针对每个任务缩小 `UPROGS` 和修改用户入口。这样每个任务都能独立启动、独立运行、独立验证，同时保留真实内核链路。

### 5.2 `init.c` 的作用

每个任务目录中的 `user/init.c` 是用户态第一个进程。它负责：

- 打开或创建 `console`。
- 设置文件描述符 0、1、2。
- 使用 `fork()` 创建子进程。
- 在子进程中使用 `exec()` 装载目标用户程序。
- 在父进程中使用 `wait()` 等待程序结束。

这使得每个任务的用户程序都不是由内核硬编码调用，而是通过标准 xv6 用户态流程启动。

### 5.3 `Makefile` 与 `fs.img`

每个任务的 `Makefile` 做三件事：

1. 编译内核对象并链接为 `kernel/kernel`。
2. 编译用户程序为 `user/_xxx` ELF。
3. 调用 `mkfs/mkfs fs.img README $(UPROGS)` 生成文件系统镜像。

只有写入 `UPROGS` 的用户程序才会进入 `fs.img`。例如宿主机上存在 `user/ls.c` 不代表 xv6 里一定能运行 `ls`，必须把 `$U/_ls` 加到 `UPROGS` 中。

### 5.4 新增系统调用的完整步骤

以 `ques5` 的 `sem_p` 为例，新增系统调用需要同时修改以下位置：

```text
user/user.h       声明 int sem_p(int)
user/usys.pl      增加 entry("sem_p")
kernel/syscall.h  分配 SYS_sem_p
kernel/syscall.c  注册 sys_sem_p
kernel/sysproc.c  实现 sys_sem_p 参数解析
kernel/defs.h     声明 sem_p 内核函数
kernel/semaphore.c 实现 sem_p 逻辑
Makefile          链接 semaphore.o
```

少改任何一步都会导致编译失败、链接失败或运行时 unknown syscall。

### 5.5 sleep/wakeup 的同步语义

信号量阻塞不能用普通循环忙等，因为忙等会浪费 CPU，也无法体现内核调度。xv6 的 `sleep(chan, lock)` 解决了两个问题：

- 进程睡眠前原子释放锁，避免其他进程无法进入 `sem_v()`。
- 被 `wakeup(chan)` 唤醒后重新获取锁，避免并发修改 `value`。

因此 semaphore 的阻塞等待必须建立在 xv6 的 `sleep/wakeup` 上，而不是简单 `while(value == 0);`。

### 5.6 ELF Program Header 解析

ELF Header 中最关键的字段包括：

| 字段 | 含义 |
|------|------|
| `magic` | ELF 魔数，用于判断文件类型 |
| `entry` | 程序入口虚拟地址 |
| `phoff` | Program Header 表在文件中的偏移 |
| `phentsize` | 每个 Program Header 表项大小 |
| `phnum` | Program Header 数量 |

Program Header 中最关键的字段包括：

| 字段 | 含义 |
|------|------|
| `type` | 段类型，`ELF_PROG_LOAD` 表示需要装载 |
| `off` | 段在文件中的偏移 |
| `vaddr` | 段装载到用户地址空间的虚拟地址 |
| `filesz` | 文件中实际占用大小 |
| `memsz` | 内存中占用大小 |
| `flags` | 读写执行权限 |

这些字段正是 `exec.c` 装载用户程序时所依赖的信息。`ques6` 在用户态重新解析这些字段，是为了展示 xv6 文件系统和 ELF 格式之间的关系。

## 6. 验证结果

### 6.1 Task 2

运行目录：`project/ques2`

关键输出：

```text
xv6 kernel is booting
init: starting test_boot
=== Task 2: Bootloader + Basic Kernel Demo ===
...
init: test_boot exited with status 0
```

结论：通过。内核真实启动，`init` 从文件系统执行 `test_boot`，基础系统调用可用。

### 6.2 Task 3

运行目录：`project/ques3`

关键输出：

```text
init: starting test_mem
=== Task 3: Memory Management Demo ===
wrote first allocated page: A ... Z
wrote second allocation: B ... Y
final brk: 0x5000
=== Memory Management Test PASSED ===
init: test_mem exited with status 0
```

结论：通过。用户程序真实写入 `sbrk` 扩展出的内存页面。

### 6.3 Task 4

运行目录：`project/ques4`

关键输出：

```text
init: starting test_sched
=== Task 4: Scheduler Demo ===
1. Fork three CPU-bound children without explicit yield
   scheduler progress log: 111113333322222
   child round counts: 5 5 5 (expected 5 5 5)
2. Timer interrupt path preempted CPU-bound user code
   trap -> yield -> sched -> swtch -> scheduler -> next RUNNABLE process
=== Scheduler Test PASSED ===
init: test_sched exited with status 0
```

结论：通过。三个 CPU-bound 用户子进程都获得调度机会，父进程通过 pipe 收集并校验进度。

### 6.4 Task 5

运行目录：`project/ques5`

关键输出：

```text
init: starting test_sync
=== Task 5: Synchronization Demo ===
1. Kernel semaphore P/V protects a shared kernel counter
   - shared_counter final: 200 (expected 200)
2. Counting semaphore state transitions
   - semaphore value after P: 1 (expected 1)
   - semaphore value after V: 2 (expected 2)
3. Blocking wait, wakeup, and deadlock check
   - counter before V: 0 (expected 0)
   - counter after wakeup: 10 (expected 10)
   - try-P on empty semaphore: -1 (expected -1)
=== Synchronization Test PASSED ===
init: test_sync exited with status 0
```

结论：通过。新增 semaphore syscall 通过真实用户态 ecall 进入内核，覆盖互斥、计数信号量、阻塞唤醒和非阻塞 try-P。

### 6.5 Task 6

运行目录：`project/ques6`

关键输出：

```text
init: starting test_fs
=== Task 6: File System / ELF Parsing Demo ===
1. Read a real ELF executable from fs.img and parse Program Headers
   - test_fs ELF entry=0x444 phoff=64 phnum=4
   - PH[1] LOAD off=0x1000 vaddr=0x0 filesz=0x15A0 memsz=0x15A0 flags=0x5
   - PH[2] LOAD off=0x3000 vaddr=0x2000 filesz=0x0 memsz=0x30 flags=0x6
2. Read a real non-ELF file and reject it
   - README rejected as non-ELF, magic=0x20367678
=== File System Parser Test PASSED ===
init: test_fs exited with status 0
init: starting shell
$
```

结论：通过。用户程序通过真实 xv6 文件系统完整读取 ELF 文件并解析 Program Header，随后进入 shell，可继续直接运行文件系统镜像中的用户程序。

## 7. 子 Agent 审核结论

本轮使用三个子 agent 分别审核 `ques4`、`ques5`、`ques6`。

| 审核对象 | 发现 | 处理结果 |
|----------|------|----------|
| `ques4` | 构建和内核链路是真 xv6；测试未检查 wait 状态，调度证明偏弱 | 已增强 `test_sched.c`，加入 pipe 进度校验和子进程状态检查 |
| `ques5` | `semaphore.o` 未链接；`sem_p` 缺少 killed 检查；阻塞唤醒测试不足 | 已修复 Makefile、`sem_p` killed 路径，并增强 `test_sync.c` |
| `ques6` | 链路是真 xv6；测试只读前 4096 字节，负例短读判断不够硬 | 已改为 `fstat + malloc + read loop` 完整读取文件 |

## 8. 验收结论

当前项目已满足“完整操作系统内核、真实运行用户测试程序、系统调用完整链路”的验收方向：

- 不是内核侧 demo 打印。
- 不是静态数组模拟文件。
- 不是跳过 `exec` 的用户程序假运行。
- 不是只编译用户程序但不执行。

`ques2` 到 `ques6` 均能在 QEMU 中启动完整 xv6 内核，挂载 `fs.img`，执行对应用户程序，并打印 PASS 标记与 `init: ... exited with status 0`。其中 `ques6` 进一步进入交互 shell，方便在同一个 xv6 会话中继续执行其他用户程序。

## 9. 运行命令汇总

```bash
cd project/ques2
timeout 25s make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1

cd ../ques3
timeout 25s make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1

cd ../ques4
timeout 25s make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1

cd ../ques5
timeout 25s make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1

cd ../ques6
make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1
```

`ques6` 进入 shell 后，可以输入 `ls`、`cat README`、`echo hello`、`test_fs` 等命令。退出 QEMU 使用 `Ctrl-A` 后按 `X`。

## 10. 后续可改进项

当前 `ques2` 到 `ques5` 的 `init` 在测试完成后保持运行，便于观察 QEMU 日志；`ques6` 在测试完成后进入 shell，便于手动运行用户程序。自动化平台如果要求主动关机，可以后续增加一个测试专用 poweroff syscall 或 QEMU `isa-debug-exit` 设备支持。该项不影响当前真实内核、用户程序、系统调用和文件系统链路的验收。
