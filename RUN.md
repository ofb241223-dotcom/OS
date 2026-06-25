# 运行说明

以下按指导书实验顺序给出运行过程。

---

## A1 Windows 进程管理

### 1. hello

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_hello.exe a1_hello.c
cd /d E:\oslab\windows\output
a1_hello.exe
```

### 2. clone

第一次运行：

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_clone.exe a1_clone.c
cd /d E:\oslab\windows\output
a1_clone.exe
```

说明：共创建 6 个控制台窗口。

第二次运行（带参数 3）：

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_clone.exe a1_clone.c
cd /d E:\oslab\windows\output
a1_clone.exe 3
```

说明：共创建 3 个控制台窗口。

### 3. clone_fix1

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_clone_fix1.exe a1_clone_fix1.c
cd /d E:\oslab\windows\output
a1_clone_fix1.exe
```

说明：共创建 6 个控制台窗口。

### 4. clone_fix2

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_clone_fix2.exe a1_clone_fix2.c
cd /d E:\oslab\windows\output
a1_clone_fix2.exe
```

说明：会无限创建新控制台窗口，运行前注意。

### 5. procterm

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_procterm.exe a1_procterm.c
cd /d E:\oslab\windows\output
a1_procterm.exe
```

输入：父窗口按一次 `Enter`
说明：共创建 2 个控制台窗口。

### 6. procterm_fix1

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_procterm_fix1.exe a1_procterm_fix1.c
cd /d E:\oslab\windows\output
a1_procterm_fix1.exe
```

说明：会无限创建新控制台窗口，运行前注意。

### 7. procterm_fix2

```cmd
cd /d E:\oslab\windows\src
gcc -o ..\output\a1_procterm_fix2.exe a1_procterm_fix2.c
cd /d E:\oslab\windows\output
a1_procterm_fix2.exe
```

输入：父窗口按一次 `Enter`
说明：共创建 2 个控制台窗口。

---

## A2 Linux 进程控制

### 1. fork_demo

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/a2_fork_demo a2_fork_demo.c
cd /mnt/e/oslab/linux/output
./a2_fork_demo
```

### 2. exec_demo

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/a2_exec_demo a2_exec_demo.c
cd /mnt/e/oslab/linux/output
./a2_exec_demo
```

---

## A3 Linux 进程间通信

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/a3_msg_queue_demo a3_msg_queue_demo.c
cd /mnt/e/oslab/linux/output
./a3_msg_queue_demo
```

---

## A4 Windows 线程的互斥与同步

### 1. producer_consumer

```cmd
cd /d E:\oslab\windows\src
g++ -o ..\output\a4_producer_consumer.exe a4_producer_consumer.cpp
cd /d E:\oslab\windows\output
a4_producer_consumer.exe
```

输出：控制台会持续出现生产、消费、缓冲区状态信息。
结束：`Ctrl + C`

### 2. producer_consumer_fix1

```cmd
cd /d E:\oslab\windows\src
g++ -o ..\output\a4_producer_consumer_fix1.exe a4_producer_consumer_fix1.cpp
cd /d E:\oslab\windows\output
a4_producer_consumer_fix1.exe
```

输出：控制台会持续出现生产、消费、缓冲区状态信息。
结束：`Ctrl + C`

### 3. producer_consumer_fix2

```cmd
cd /d E:\oslab\windows\src
g++ -o ..\output\a4_producer_consumer_fix2.exe a4_producer_consumer_fix2.cpp
cd /d E:\oslab\windows\output
a4_producer_consumer_fix2.exe
```

输出：控制台会持续出现生产、消费、缓冲区状态信息。
结束：`Ctrl + C`

---

## A5 Windows 内存管理

```cmd
cd /d E:\oslab\windows\src
g++ -o ..\output\a5_vmwalker.exe a5_vmwalker.cpp -lshlwapi
cd /d E:\oslab\windows\output
a5_vmwalker.exe
```

---

## B1 银行家算法

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b1_banker b1_banker.c
cd /mnt/e/oslab/linux/output
./b1_banker
```

输入顺序：

```text
5
3
10 5 7
7 5 3
3 2 2
9 0 2
2 2 2
4 3 3
0 1 0
2 0 0
3 0 2
2 1 1
0 0 2
2
1
1 0 2
2
0
0 2 0
4
```

对应含义：

```text
5                // 进程数
3                // 资源种类数
10 5 7           // 总资源向量
7 5 3            // P0 的 Max
3 2 2            // P1 的 Max
9 0 2            // P2 的 Max
2 2 2            // P3 的 Max
4 3 3            // P4 的 Max
0 1 0            // P0 的 Allocation
2 0 0            // P1 的 Allocation
3 0 2            // P2 的 Allocation
2 1 1            // P3 的 Allocation
0 0 2            // P4 的 Allocation
2                // 菜单里选择 Request resources
1                // 选择 P1
1 0 2            // P1 请求 [1,0,2]，这是成功请求
2                // 再次选择 Request resources
0                // 选择 P0
0 2 0            // P0 请求 [0,2,0]，这是失败请求（会进入不安全状态）
4                // 菜单里选择 Exit
```

输出：先显示当前资源分配表和安全序列，然后会看到一次成功请求和一次失败请求。

---

## B2 页面置换算法

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b2_page_replacement b2_page_replacement.c
cd /mnt/e/oslab/linux/output
./b2_page_replacement
```

---

## B3 磁盘调度算法

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b3_disk_scheduling b3_disk_scheduling.c
cd /mnt/e/oslab/linux/output
./b3_disk_scheduling
```

按教材数据输入：

```text
200
100
9
55 58 39 18 90 160 150 38 184
1
```

对应含义：

```text
200                                // 磁道总数
100                                // 磁头初始位置
9                                  // 请求个数
55 58 39 18 90 160 150 38 184      // 请求序列
1                                  // 初始方向：磁道号增大方向
```

输出：依次显示 FIFO、SSTF、SCAN、C-SCAN 的访问顺序、移动道数和平均寻道长度。

---

## B4 读者写者问题

```bash
cd /mnt/e/oslab/linux/src
gcc -pthread -o ../output/b4_readers_writers b4_readers_writers.c
cd /mnt/e/oslab/linux/output
./b4_readers_writers
```

输出：会看到 reader/writer 的申请、读写、结束，以及最后的 `shared_data`。

---

## B5 哲学家进餐问题

```bash
cd /mnt/e/oslab/linux/src
gcc -pthread -o ../output/b5_dining_philosophers b5_dining_philosophers.c
cd /mnt/e/oslab/linux/output
./b5_dining_philosophers
```

输出：会看到哲学家思考、拿起叉子、进餐、放下叉子的过程。

---

## B6 命令行解释器

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b6_mini_shell b6_mini_shell.c
cd /mnt/e/oslab/linux/output
./b6_mini_shell
```

输入顺序：

```text
help
pwd
cd
cd ..
pwd
cd /mnt/e/oslab/linux/output
pwd
echo hello world
environ
cd not_exist_dir
ls
sleep 10 &
jobs
unknowncmd
exit
```

对应含义：

```text
help                         // 查看 shell 支持的内部命令
pwd                          // 查看当前目录
cd                           // 不带参数时输出当前目录
cd ..                        // 切换到上一级目录
pwd                          // 再次查看当前目录
cd /mnt/e/oslab/linux/output // 切回实验输出目录
pwd                          // 再次确认当前目录
echo hello world             // 测试 echo
environ                      // 输出全部环境变量
cd not_exist_dir             // 测试目录不存在时的错误提示
ls                           // 测试前台外部命令执行
sleep 10 &                   // 启动后台子进程，便于 jobs 看到运行中状态
jobs                         // 查看后台子进程、PID 和命令名
unknowncmd                   // 测试错误命令提示
exit                         // 测试退出命令
```

输出：进入 `myshell>` 提示符后，应能看到 help 表格、目录切换结果、环境变量输出、cd 错误提示、前台命令结果、后台任务运行状态、错误命令提示和 exit 退出效果。

---

## B7 进程调度算法

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b7_process_scheduler b7_process_scheduler.c
cd /mnt/e/oslab/linux/output
./b7_process_scheduler
```

输入顺序：

```text
1
7
```

对应含义：

```text
1    // 选择 sample data
7    // 运行全部调度算法
```

输出：按时间片逐步显示各进程状态，最后输出汇总表。

---

## B8 动态分区管理

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b8_dynamic_partition b8_dynamic_partition.c
cd /mnt/e/oslab/linux/output
./b8_dynamic_partition
```

首次适应输入顺序：

```text
1
```

对应含义：

```text
1    // 运行 First Fit Simulation，程序会自动执行指导书 7 个步骤
```

最佳适应输入顺序：

```text
2
```

对应含义：

```text
2    // 运行 Best Fit Simulation，程序会自动执行指导书 7 个步骤
```

最坏适应输入顺序：

```text
3
```

对应含义：

```text
3    // 运行 Worst Fit Simulation，程序会自动执行指导书 7 个步骤


输出：程序会先显示当前策略，再自动显示初始内存布局，然后按指导书 7 个步骤依次输出每一步的当前内存分配情况。

固定实验序列对应指导书步骤：

```text
① 作业1申请 50KB
② 释放作业2（250KB, 200KB）
③ 释放作业3（450KB, 150KB）
④ 作业4申请 200KB
⑤ 释放作业1（80KB, 50KB）
⑥ 释放作业4（150KB, 200KB）
⑦ 作业5申请 600KB
```

---

## B9 简单文件系统

```bash
cd /mnt/e/oslab/linux/src
gcc -o ../output/b9_simple_fs b9_simple_fs.c
cd /mnt/e/oslab/linux/output
./b9_simple_fs
```

输入顺序：

```text
dir
login user1
create file0 rw
open file0 rw
write file0 this is file0
read file0
close file0
dir
chmod file0 r
dir
open file0 r
read file0
close file0
delete file0
dir
logout
exit
```

对应含义：

```text
dir                          // 查看主目录
login user1                  // 用户登录
create file0 rw              // 创建文件 0
open file0 rw                // 打开文件 0
write file0 this is file0    // 写文件 0
read file0                   // 读文件 0
close file0                  // 关闭文件 0
dir                          // 列目录，观察文件名/物理地址/保护码/长度
chmod file0 r                // 修改 file0 的保护码，体现读写保护
dir                          // 再次列目录，观察保护码已变为 r
open file0 r                 // 以只读方式打开 file0
read file0                   // 读取 file0，验证只读访问
close file0                  // 关闭 file0
delete file0                 // 删除文件 0
dir                          // 再次列目录
logout                       // 用户退出
exit                         // 关闭文件系统
```

输出：进入 `fs:/ >` 提示符后，会依次显示登录、列目录、创建文件、读写文件、修改保护码、删除文件、退出等结果。

说明：当前代码实现的是简化版二级文件系统命令集，重点覆盖 Login、Dir、Create、Delete、Open、Close、Read、Write 这些指导书要求的核心操作。
