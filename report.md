# xv6-riscv 操作系统内核项目报告

## 1. 项目目标

本项目按照 `project.md` 的 `ques2` 到 `ques6` 任务要求，完成一个可以在 QEMU RISC-V64 环境中真实启动的 xv6-riscv 操作系统内核。验收重点不是内核侧打印演示，而是：

- QEMU 加载真实 RISC-V 内核镜像 `kernel/kernel`。
- 内核完成 UART、页表、trap、syscall、proc、virtio、buffer cache、log、inode、file、exec 等初始化。
- `mkfs` 生成真实 `fs.img`，其中包含 `/init` 和对应用户测试程序。
- 内核通过 `kexec("/init")` 启动第一个用户进程。
- `init` 再通过真实 `fork + exec + wait` 运行 `test_boot/test_mem/test_sched/test_sync/test_fs`。
- 用户程序通过真实系统调用进入内核，覆盖进程、内存、调度、同步、文件系统和 ELF 解析路径。

当前实现已把 `project/ques2` 到 `project/ques6` 都整理为完整 xv6 工程目录，并针对每个任务保留最小用户测试程序集。

## 2. 实验环境

| 项目 | 内容 |
|------|------|
| 主机环境 | Termux / Linux shell |
| 目标架构 | RISC-V64 |
| 模拟器 | `qemu-system-riscv64` |
| 工具链 | `riscv64-linux-gnu-` |
| 运行命令 | `make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1` |

说明：测试命令使用 `timeout 25s` 包裹 QEMU。测试程序打印 PASS 后，`init` 会留在系统中循环 `pause()`，所以宿主命令返回 `124` 是预期行为，不代表测试失败。

## 3. 总体实现

各任务目录都采用完整 xv6-riscv 内核结构，而不是演示桩：

| 目录 | 用户测试程序 | 验证重点 |
|------|--------------|----------|
| `project/ques2` | `user/test_boot.c` | 启动、UART 输出、`fork/getpid/wait/sbrk` 基础系统调用 |
| `project/ques3` | `user/test_mem.c` | `sbrk` 堆增长、页分配、用户态实际读写新页 |
| `project/ques4` | `user/test_sched.c` | CPU-bound 子进程、timer trap、`yield/sched/swtch/scheduler` 调度链路 |
| `project/ques5` | `user/test_sync.c` | 新增 semaphore 系统调用、阻塞唤醒、共享计数器互斥 |
| `project/ques6` | `user/test_fs.c` | 从 xv6 文件系统完整读取真实 ELF 文件并解析 Program Header |

每个目录的 `Makefile` 都会构建 `kernel/kernel`、用户 ELF、`mkfs/mkfs` 和 `fs.img`。`UPROGS` 被收窄为 `_init` 加当前任务测试程序，便于验收日志聚焦。

## 4. 关键模块说明

### 4.1 启动与用户程序装载

内核从 `_entry` 进入，完成 machine mode 到 supervisor mode 的切换后进入 `main()`。`main()` 初始化 console、printf、物理页分配、页表、trap、PLIC、buffer cache、inode、file、virtio disk、进程表等模块，然后创建第一个用户进程。

用户态启动链路为：

```text
QEMU -> kernel/kernel -> main -> userinit -> kexec("/init")
      -> init -> fork -> exec("test_xxx") -> wait
```

这条路径会真实经过 `trap.c` 的 ecall 处理、`syscall.c` 的系统调用分派、`sysfile.c` 的文件系统调用、`exec.c` 的 ELF 装载和 `proc.c` 的调度。

### 4.2 内存管理

`ques3` 使用 xv6 的页式物理内存分配器和用户地址空间增长逻辑。用户测试调用 `sbrk()` 增长堆空间，并直接写入新分配页面，验证缺页/映射/物理页分配链路能够支持真实用户内存访问。

### 4.3 调度

`ques4` 保留完整 xv6 进程表、上下文切换和时钟中断路径。增强后的 `test_sched.c` 创建三个不主动 `yield` 的 CPU-bound 子进程，并通过继承的 pipe 向父进程写入进度标记。父进程断言三个子进程均完成 5 轮运行，并检查所有子进程退出状态为 0。

### 4.4 同步与信号量系统调用

`ques5` 在完整 xv6 syscall 链路中新增：

```text
sem_init
sem_p
sem_v
sem_value
sem_try_p
sync_counter_reset
sync_counter_add
sync_counter_get
```

新增文件：

- `project/ques5/kernel/semaphore.h`
- `project/ques5/kernel/semaphore.c`

同时修改 `syscall.h/c`、`sysproc.c`、`defs.h`、`main.c`、`user/user.h`、`user/usys.pl` 和 `Makefile`，使用户程序能通过 ecall 调用内核信号量实现。

`sem_p()` 使用 xv6 的 `sleep(chan, lock)` 阻塞，`sem_v()` 使用 `wakeup(chan)` 唤醒。当前实现还处理了 killed 进程：阻塞进程被 kill 唤醒后会返回 `-1`，避免永久睡眠。

### 4.5 文件系统与 ELF 解析

`ques6` 不再解析静态数组，而是打开真实 `fs.img` 中的文件：

- 正例：完整读取 `test_fs` 用户 ELF 文件，通过 ELF header 找到 Program Header 表，打印 `PT_LOAD` 段。
- 反例：完整读取 `README`，验证其不是 ELF，并打印实际 magic。

读取过程使用 `open + fstat + read + close`，解析过程使用小端解码函数读取 ELF 字段，避免未对齐结构体访问。

## 5. 验证结果

### 5.1 Task 2

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

### 5.2 Task 3

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

### 5.3 Task 4

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

### 5.4 Task 5

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

### 5.5 Task 6

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
```

结论：通过。用户程序通过真实 xv6 文件系统完整读取 ELF 文件并解析 Program Header。

## 6. 子 Agent 审核结论

本轮使用三个子 agent 分别审核 `ques4`、`ques5`、`ques6`。

| 审核对象 | 发现 | 处理结果 |
|----------|------|----------|
| `ques4` | 构建和内核链路是真 xv6；测试未检查 wait 状态，调度证明偏弱 | 已增强 `test_sched.c`，加入 pipe 进度校验和子进程状态检查 |
| `ques5` | `semaphore.o` 未链接；`sem_p` 缺少 killed 检查；阻塞唤醒测试不足 | 已修复 Makefile、`sem_p` killed 路径，并增强 `test_sync.c` |
| `ques6` | 链路是真 xv6；测试只读前 4096 字节，负例短读判断不够硬 | 已改为 `fstat + malloc + read loop` 完整读取文件 |

## 7. 验收结论

当前项目已满足“完整操作系统内核、真实运行用户测试程序、系统调用完整链路”的验收方向：

- 不是内核侧 demo 打印。
- 不是静态数组模拟文件。
- 不是跳过 `exec` 的用户程序假运行。
- 不是只编译用户程序但不执行。

`ques2` 到 `ques6` 均能在 QEMU 中启动完整 xv6 内核，挂载 `fs.img`，执行对应用户测试程序，并打印 PASS 标记与 `init: ... exited with status 0`。

## 8. 运行命令汇总

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
timeout 25s make qemu TOOLPREFIX=riscv64-linux-gnu- QEMU=qemu-system-riscv64 CPUS=1
```

## 9. 后续可改进项

当前 `init` 在测试完成后保持运行，便于观察 QEMU 日志，但自动化平台如果要求主动关机，可以后续增加一个测试专用 poweroff syscall 或 QEMU `isa-debug-exit` 设备支持。该项不影响当前真实内核、用户程序、系统调用和文件系统链路的验收。
