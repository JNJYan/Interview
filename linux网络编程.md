
[TOC]

# IP

## IP地址分类
- A: 高位为0，允许有126个A类地址
- B: 高位为10，允许有16384个B类地址，每个网络有65534个主机
- C: 高位为110，允许有200万个C类地址，每个网络只有254个主机

![image-20200626142900469](C:\Users\May\AppData\Roaming\Typora\typora-user-images\image-20200626142900469.png)

# 进程控制
- fork()
    复制调用进程来创建新的进程。
- exec
    包含一系列系统调用，通过用一个新的程序覆盖原内存空间，来实现进程的转变。
- wait()
    提供了初级的进程同步措施，使一个进程等待另一个进程的结束。
- exit()
    系统调用，终止一个程序的运行。

## 进程的建立与运行
- fork()
创建新进程。

在Linux系统库`unistd.h`中声明如下
```c
pid_t fork(void);
```
父进程返回子进程的pid，子进程返回0。

### 进程的运行
- 系统调用exec系列
将一个新程序装入调用进程的内存空间，来改变调用进程的执行代码，形成新进程。若`exec`调用成功，则调用进程将被覆盖，从新程序的入口处开始执行，这样就产生了一个新进程，但其pid与调用进程相同。

在Linux系统库`unistd.h`中声明如下
```c
int execl( const char *path, const char *arg, ...);
int execlp( const char *file, const char *arg, ...);
int execle( const char *path, const char *arg , ..., char* const envp[]);
int execve( const char *file, char *const argv[], char* const envp[]);
int execv( const char *path, char *const argv[]);
int execvp( const char *file, char *const argv[]);
```
以`execl()`为例: 

第一个参数`path`给出了被执行程序所在的文件名，必须是一个有效地路径名，且文件本甚也是一个真正的可执行程序。但不能用`execl`来运行一个shell命令组成的文件。

第二个以及圣罗浩表示其他参数一起组成了该程序执行时的参数表，按照Linux惯例，参数表的第一项是不带路径的程序文件名。
`shell`本身对命令的调用也是通过`exec`调用实现的。

必须以`nullptr`来标识参数列表的结束。

`execv()`第二个参数，利用一个字符型指针数组代替了原来的参数列表，同样以`nullptr/NULL`指针结尾。

`execlp()`和`execvp()`与上述两个类似，第一个参数不再是路径，而是程序的文件名，路径由环境变量`$PATH`来指定，且这两个命令可以运行`shell`程序。

- 对`exec`传送变量的访问
任何被`exec`调用所执行的程序，都可以访问`exec`调用中的参数。

```c
int main(int argc, char* argv[]);
```

- `exec`和`fork()`的联用

### 数据和文件描述符的继承
- `fork()`、文件和数据

`fork()`建立的子进程与父进程除`fork()`返回值外完全一样，子进程所用的数据是父进程可用数据的拷贝，占用不同的内存地址空间。

在父进程中已打开的文件在子进程中同样被打开，但二者共用文件指针。

- `exec`和打开文件
可以调用`fcntl()`设置文件描述符的执行关闭位，在调用`exec`时会关闭相应的文件。
```c
fctnl(fd, F_SETFD, 1);
```

## 进程的控制
### 进程的终止
- exit()
系统调用`exit()`实现进程的终止。

在Linux系统函数库`stdlib.h`中声明如下：
```c
void exit(int status);
```

`exit()`除了停止进程的运行之外，还会关闭所有已打开的文件。

若父进程因执行了`wait()`调用而处于睡眠状态，则子进程调用`exit()`会重新启动父进程，还会完成系统内部的清理工作，如缓冲区的清除等。
在Linux系统函数库`unistd.h`中还有一个同样用于进程终止的系统调用。它只执行终止进程的动作而没有系统内部的清理工作。
```c
void _exit(int status);
```

### 进程的同步
- wait()
系统调用`wait()`是实现进程同步的简单手段。

在Linux系统函数库`sys/wait.h`中声明如下:
```c
pid_t wait(int *status);
```
参数可以是一个指向整型的指针，也可以是一个`nullptr`，若是一个有效指针，则`wait()`返回时，该指针指向子进程退出时的状态。
返回值通常时结束的子进程的pid，若返回-1，则没有子进程结束，`errno`中有出错代码`ECHILD`。

当子进程执行时，`wait()`可以暂停父进程的执行，使其等待，一旦子进程执行完，父进程会开始重新执行，若有多个子进程执行，则在第一个子进程结束时返回，恢复父进程执行。


### 进程终止的特殊情况
1. 子进程终止时，父进程并不在执行`wait()`调用。
    要终止的进程处于过渡状态(zombie)，处于这种状态的进程不使用任何内核资源，但要占用内核中的进程处理表中的一项，当其父进程执行`wait()`等待子进程时，它就会进入睡眠状态，然后将这种处于过渡状态的进程从系统中删除，父进程仍能得到子进程的结束状态。
2. 子进程尚未终止时，父进程终止。
    将子进程(包括处于过渡状态)交归系统的初始化进程归属。

### 进程控制的实例

## 进程的属性
### PID
进程标识符0是调度进程，按一定的原则分配进程。
1是初始化进程，是程序`/sbin/init`的执行，是unix系统其他进程的祖先，并且是进程的最终控制者。
```c
pid_t getpid(); //可以得到程序本身的pid
pid_t getppid(); //可以得到父进程的pid 
```
### 进程的组标识符
Linux将进程分属一些组，用进程的组标识符来标识进程所属组，进程最初是通过`fork()`和`exec`调用来继承其进程标识符，进程也可以使用系统调用`setpgrp()`，形成一个新的组。

在Linux系统函数库`unistd.h`中声明如下：
```c
int setpgrp(void);
int getpgrp(void);//获取当前进程组标识符
```
返回值是新的进程组标识符，就是调用进程的进程标识符，调用进程成为这个组的进程组首，其所建立的所有进程，将继承该进程组标识符。

当某个用户退出系统时，其相应的shell进程所启动的全部进程都要被强行终止(SIGHUP)，由进程的会话id来决定。

### 进程环境
进程环境是一个以NULL字符结尾的字符串集合，在程序中可以用一个以NULL结尾的字符型指针数组来表示它。

Linux提供了`environ`指针，可以在程序中访问其环境内容。
```c
extern char **environ;
```
可以通过`execle()`和`execve()`两种系统调用为进程指定新环境。
在Linux系统库`unistd.h`中声明如下
```c
int execle( const char *path, const char *arg , ..., char* const envp[]);
int execve( const char *file, char *const argv[], char* const envp[]);
```
参数`envp`是以`nullptr`指针结束的字符指针数组，指出了新进程的环境。

### 进程的当前目录
一个进程的当前目录初值为其父进程的当前目录，当前目录是进程的一个属性，若子进程通过`chdir()`改变了当前目录，其父进程的当前目录并没有改变，因此系统的`cd`命令实际上是一个shell自身的内部命令，而不是单独的程序文件，否则只能改变`cd`程序运行进程自己得当前目录。

进程的根目录，与绝对路径的检索起点有关，初始值也为父进程的根目录，可以通过`chroot()`来改变进程的根目录。

### 进程的有效标识符
每个进程都有一个`实际用户标识符`和一个`实际组标识符`，它们永远是启动该进程之用户的用户标识符和组标识符。

进程的`有效用户标识符`和`有效组标识符`被用来确定一个用户能否访问某个确定的文件。在通常情况下，它们与实际用户标识符和实际组标识符是一致的。

但是，一个进程或其祖先进程可以设置程序文件的`置用户标识符权限`或`置组标识符权限`。这样，当通过`exec`调用执行该程序时，其进程的有效用户标识符就取自该文件的文件主的有效用户标识符，而不是启动该进程的用户的有效用户标识符。

```c
uid_t getuid();//实际用户标识符
uid_t geteuid();//有效用户标识符

gid_t getgid();//实际组标识符
gid_t getegid();//有效用户标识符

int setuid(uid_t newuid);
int setgid(gid_t newgid);

int seteuid(uid_t newuid);
int setegid(gid_t newgid);
```
返回值为0，表示调用成功完成，返回值为-1表示调用失败。

不是超级用户所引用的进程，只能将它的有效用户标识符和有效组标识符设置为实际用户标识符和实际组标识符。

### 进程的资源
Linux提供了几个系统调用来限制一个进程对资源的使用。
```c
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
struct rlimit
{
    int rlim_cur;
    int rlim_max;
};
int getrlimit (int resource, struct rlimit *rlim);
int setrlimit (int resource, const struct rlimit *rlim);
int getrusage (int who, struct rusage *usage);
```
只有超级用户可以取消或放大对资源的限制，普通用户只能缩小，若调用成功，则返回0，否则返回-1。
其中`getrlimit()`和`setrlimit()`分别被用来取得和设定进程对资源的限制。
`resource`制定了调用操作的资源类型，第二个参数用于取得/设定具体的限制。

`getrusage()`返回当前资源的使用情况。

### 进程的优先级
系统以整型变量`nice`为基础，来决定一个特定进程可以得到的CPU时间比例。从0到其最大值，我们将`nice`值成为进程的优先数。
系统调用`nice()`，给定一个参数，将其加到当前nice值上
```c
#include <unistd.h>
nice(5);
nice(-1);
```

## 守护进程
守护进程是后台运行并独立于所有终端控制之外的进程。

### 守护进程的启动
1. 系统启动期间，通过系统初始化脚本启动守护进程，通常在目录`/etc/rc.d`下，具有超级用户权限。
2. 网络服务程序由`inetd`守护程序启动。
3. cron
4. at
5. 从终端启动&
6. nohup

### 守护进程的错误输出
Linux系统函数库`syslog.h`中声明如下
```c++
void openlog(const char *ident, int options, int facility);
void syslog(int priority, char *format,...);
syslog(LOG_INFO, "%s: %m\n", file1);
```

### 守护进程的建立

# 进程间通信
## 基本概念
1. 进程阻塞
    当进程在执行某些操作的条件得不到满足时，自动放弃CPU资源而进入休眠状态，以等待条件满足，当条件满足时，系统就将控制权返还给该进程继续进程未完的操作。
2. 共享资源
3. 上锁

## 信号
系统用信号来通知一个或多个进程异步事件的发生，信号不仅能从内核发往一个进程，也能从一个进程发往另一个进程。

### 信号定义
在Linux系统库`bits/signum.h`中定义了信号：
- SIGHUP
    终止一个终端时，内核就将该信号发往中断所控制的所有进程。
- SIGNIT
    中断键(Ctrl+C)，内核向该进程发送该信号。
- SIGQUIT
    退出键(Ctrl+\)，形成非正常终止。该进程的而影响被转贮到一个磁盘文件中，供调试使用。
- SIGKILL
    进程试图执行一条非法执行令，形成非正常终止。
- SIGTRAP
    调试程序使用的专用信号，形成非正常终止。
- SIGFPE
    形成非正常终止。
- SIGKILL
- SIGALRM
    定时器。
- SIGSTOP
- SIGUSR1/2
- SIGCHLD
    子进程结束信号，实现系统调用`exit`和`wait`

在Linux系统库`stdlibh`中定义了
```c
void abort(void);
```
向调用进程发送一个信号，产生一个非正常终止，即核心转贮，能使进程在出错时记录进程的当前状态。

### 信号的处理
在Linux系统库`signal.h`中声明如下：
```c
int signal(int sig, __sighandler_t handler);
// sig: 要处理的信号类型，可以取除了SIGKILL和SIGSTOP外的任何信号。
// handler: 描述了与信号关联的动作，取值如下：
//   - 一个返回值为整数的函数地址
//   - SIG_IGN：忽略信号
//   - SIG_DFL：恢复系统对信号的默认处理
```
### 信号与系统调用
有少数几个系统调用能被信号打断，如`wait(), pause(), read(), write(), open()`等，若被打断返回-1，并将errno设置为`EINTR`。

### 信号的复位
当一个信号的信号处理函数执行时，若进程又接收到了该信号，则信号会被自动存储而不会中断信号处理函数的执行，直到信号处理函数执行完毕后再重新调用相应的处理函数。
同种信号不能累积，若两个信号同时产生，系统并不保证接收次序。

**信号作为进程通信手段不具备可靠性，进程不能保证它发出的信号不被丢失。**

### 在进程间发送信号
进程可以向其他进程发送信号，由系统调用`kill()`来完成。
在Linux函数库`signal.h`中声明如下：
```c
int kill(pid_t pid, int sig);
// pid制定了接收对象的pid，若为0，则发送到进程组的所有进程，若为-1，则按进程标识符由高到低发送给全部进程，若小于-1，则发送给标识符为pid绝对值的进程组里的所有进程。
// sig指定发送信号的类型。
// 普通用户的进程只能向与其相同的用户标识符的进程发送信号。root用户进程可以给任何进程发送信号。
```
kill时对系统调用`kill()`的命令层接口，默认信号为`SIGTERM`，也可以指定发送其他信号类型。

### 系统调用alarm()和pause()
1. alarm()
`alarm()`可以建立一个进程的报警时钟，在始终定时器到时，用信号向程序报告。
在Linux系统函数库`unistd.h`中声明如下：
```c
unsigned int alarm(unsigned int seconds);
```

2. pause()
`pause()`能使调用进程暂停执行，直至接收到某种信号为止。
在Linux系统函数库`unistd.h`中声明如下：
```c
int pause(void);
// 返回值始终是-1，errno被设置为ERESTARTNOHAND
```

### 系统调用setjmp()和longjmp()
当接收到一个信号时，希望能跳回程序中以前的一个位置执行。
`setjmp()`能保存程序中的当前位置（是通过保存堆栈环境实现的），`longjmp()`能把控制转回到被保存的位置。
在`setjump.h`中声明如下：
```c
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);
```

## 管道(匿名管道)
将一个程序的输出与另一个程序的输入连接起来的单向通道。
### Pipe函数
使用系统函数`pipe()`建立管道，
```c
int pipe(int fd[2]);
// 返回0 表示成功，返回-1表示失败，失败信息在errno中
// fd[0]用于从管道中读，fd[1]用于向管道中写。
```

### dup函数
将管道的句柄定向到I/O上去，在子进程使用`exec`调用外部程序时，就会将管道作为它的输入输出。
可以通过系统函数`dup()`来实现
```c
int dup(int oldfd);
// 成功则返回一个新的描述符
// 错误则返回-1。
```
在dup函数中无法指定重定向的新句柄，系统将自动使用未被使用的最小文件句柄作为重定向的新句柄。

### dup2函数
```c
int dup(int oldfd, int newfd);
// 成功则返回新的描述符
// 错误则返回-1。
```
旧描述字会被自动关闭

### popen()/pclose()函数
```c
FILE *popen(char* command, char* type);
int pclose(FIFLE *stream);
```

popen()首先调用pipe建立一个管道，然后用fork()函数建立一个子进程，运行一个shell环境，在环境中运行command参数指定的程序。

### 注意事项
1. pipe调用必须在fork之前。
2. 及时关闭不需要的管道句柄。
3. 使用dup之前确定定向的目标时最小的文件句柄。
4. 匿名管道只能实现父子进程间的通信。

## 有名管道
不仅具有管道的通信功能，且句有普通文件的优点，如被多个进程共享，长期存在等等。
### 有名管道的创建
```shell
$ mknod sampleFIFO p
$ mkfifo -m 0666 sampleFIFO
```
以上两个命令都可以创建一个名为`sampleFIFO`的有名管道，`mkfifo`可以指定所建有名管道的存储权限，而`mknod`则需要之后使用`chmod`来改变有名管道的存储权限。

![image-20200627113616569](C:\Users\May\AppData\Roaming\Typora\typora-user-images\image-20200627113616569.png)

可以通过`p`指示符来辨认有名管道。

在C中使用系统函数`mkmod`建立有名管道。
```c
int mknod(char *pathname, mode_t mode, dev_t dev);
// 成功返回0
// 失败返回-1
// 第三个参数在创建有名管道时被忽略，一般传0.
```

### 注意事项
1. 有名管道必须同时有读写两个进程端，若一个进程试图向一个没有读入端进程的有名管道写入数据，则产生SIGPIPE。
2. 管道操作独立性，一个操作不会因为任何原因被中断。
3. 超过管道一次性读写上限时，要注意不要被别的进程写入管道的数据造成混乱。

## 文件和记录锁定
文件锁定的是整个文件，而记录锁定只锁定文件的某一特定部分，Unix的记录指的是从文件的某一相对位置开始的一段连续的字节流。

文件和记录锁定分为咨询式锁定和强制锁定两种，当正在运行的某一进程对它将要访问的某一文件进行了咨询锁定之后，其他进程访问该文件时会被告知共享文件已经上锁，但仍然可以操作锁定文件，只要具有对锁定文件的存取全，就可以忽视咨询锁定去操作上了锁的文件。

强制锁定，操作系统会对每个读写文件的请求进行核查，只有确定不会干扰上锁的文件时，才允许其操作。

System V和BSD都提供了咨询锁定方式。
### System V的咨询锁定
```c
#include <unistd.h>
int lockf(int fd, int function, long size);
// size为0时，锁定记录由当前位置扩展到文件尾。
// function取值：F_ULOCK: 为一个先前所定的区域解锁
//               F_LOCK: 锁定一个区域 
//              F_TLOCK: 测试并锁定一个区域
//               F_TEST: 测试一个区域是否上锁
```
当参数为`F_LOCK`时，若指定文件的对应区域已经上锁，则阻塞到该区域解锁。
当参数设置为`F_TLOCK`时，若已经上锁，则返回-1。

### BSD的咨询锁定
```c
#include <sys/file.h>
int flock(int fd, int operation);
// operation取值：LOCK_SH: 共享锁
//              LOCK_EX: 互斥锁
//              LOCK_UN: 解锁
//              LOCK_NB: 当文件被锁定时不阻塞
// flock允许以下几种锁操作：
// LOCK_SH: 阻塞性共享锁
// LOCK_EX: 阻塞性互斥锁
// LOCK_SH|LOCK_NB: 非阻塞性共享锁
// LOCK_EX|LOCK_NB: 非阻塞性互斥锁
// LOCK_UN: 解锁
```

**BSD vs System V**
1. System V是记录锁，可以指定锁定的范围，而BSD锁定的是文件。
2. System V的锁定是每个进程所独有的，可以用于父子进程之间的共享锁定。而BSD的锁定方式是可以继承的，父子进程使用的是同一锁定，因此不能用于父子进程间的文件共享锁定。

### Linux上的其他上锁
基本思想：创建和使用一个辅助文件以表示进程对共享文件的锁操作，若辅助文件存在，就表示资源被其他进程锁定了，若不存在，则可以创建文件对其进行上锁。

### System V IPC
其具体实例在内核中是以对象的形式出现的，我们称之为IPC对象。

每个IPC对象在系统内核中有一个唯一的标识符，唯一性只在每一类的IPC对象内成立，也就是说一个消息队列和一个信号量的标识符可能是相同的。

标识符只在内核中使用，程序中通过关键字key来访问，要访问同一个IPC兑现，Server和Client必须使用同一个关键字。

使用系统函数`ftok()`来生成关键字
```c
key_t ftok(char* pathname, char proj);
```
`ftok()`通过混合`pathname`所指文件的inode和minor device以及proj值来产生关键字，但并不保证关键字的唯一性，程序可以检测关键字的冲突并通过更换`pathname`和`proj`的组合来产生新的关键字。

```c++
key_t mykey = ftok("/tmp/myapp", 'a');
key_t mykey = ftok(".", 'a');
```
只要保证server和client从同一个目录运行，就可以保证他们产生的关键字是相同的。

#### 相关命令
1. ipcs
在中断显示系统内核的IPC对象状况
`-q`: 只显示消息队列
`-m`: 只显示共享内存
`-s`: 只显示信号量

2. ipcrm
强制系统删除已经存在的IPC对象
```shell
ipcrm <msg|sem|shm> <IPC ID>
# msg：消息队列，sem：信号量， shm：共享内存
```

#### 消息队列
1. 相关结构
    1. ipc_perm
    保存每个IPC对象权限信息
    2. msgbuf
    自定义传递给队列的消息的数据类型，最大长度为4056
    3. msg
    消息队列在系统内核中是以消息链表的形式出现的，而完成消息链表每个节点结构定义的就是msg结构。
    4. msgqid_ds
    被系统内核用来保存消息队列对象有关数据。
2. 相关函数
    1. msgget()
    创建新的消息队列或获取已有消息队列。
    2. msgsnd()
    向消息队列发送消息。
    3. msgrcv()
    从消息队列中取出消息。
    4. msgctl()
    直接控制消息队列的行为。
```c++
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, struct msgbuf *msgp, int msgsz, int msgflg);
int msgrcv(int msqid, struct msgbuf *msgp, int msgsz, long mtype,int msgflg);
int msgctl(int msgqid, int cmd, struct msqid_ds *buf);
```

##### msgtool工具
交互式消息队列使用工具

#### 信号量
控制多个进程对共享资源使用的计数器，System V中的信号量对象实际上是信号量的集合，可以包含多个信号量，控制多个共享资源。
#### 共享内存

# Socket
## Send返回0
1. 内核缓冲区中没有数据。
2. 对端调用close(fd)关闭连接。
3. 对端调用close(fd, SHUT_WR)，关闭写连接，半关闭。

## 套接字类型
![image-20200627154017338](C:\Users\May\AppData\Roaming\Typora\typora-user-images\image-20200627154017338.png)

### SOCK_STREAM

流套接字，TCP

![image-20200627152408923](C:\Users\May\AppData\Roaming\Typora\typora-user-images\image-20200627152408923.png)

### SOCK_DGRAM
数据报套接字

![image-20200627152449316](C:\Users\May\AppData\Roaming\Typora\typora-user-images\image-20200627152449316.png)


通过调用socket()建立一个套接字，然后调用bind()将该套接字和本地网络地址联系在一起，再调用listen()使套接字做好侦听的准备，并规定它的请求队列的长度,之后就调用accept()来接收连接。客户在建立套接字后就可调用connect()和服务器建立连接。连接一旦建立，客户机和服务器之间就可以通过调用read()和write()来发送和接收数据。最后，待数据传送结束后，双方调用close()关闭套接字。

### 原始套接字

## 套接字
### 

## I/O模式
- 阻塞I/O
- 非阻塞I/O
- I/O多路复用
- 异步I/O
- 信号驱动I/O

## nohup原理
发送SIGHUP信号。

## gdb调试
### 调试多进程
1. attach pid
    将子进程附加到gdb调试器。
2. set follow-fork-mode parent/child
    当进行fork调用后，是调试父进程还是子进程。

### 调试多线程
1. info threads
    显示当前可调式的所有线程。
2. set scheduler-locking [off|on|step]
    `off`不锁定任何线程，`on`表示只有当前被调试的线程会执行，`step`在单步执行时，只有当前线程会执行。
3. 调试多线程或线程池
    将线程数减少至1，观察程序的逻辑是否正确，再逐步增加线程/进程数量。
