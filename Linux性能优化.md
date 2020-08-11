# 工具图
![](http://brendangregg.com/Perf/bpf_book_tools.png)
![](https://static001.geekbang.org/resource/image/0f/ba/0faf56cd9521e665f739b03dd04470ba.png)

# 基础篇
## 平均负载

平均负载是指单位时间内，系统处于可运行或不可中断状态的平均进程数。可运行状态包括运行和等待CPU的进程，不可中断状态的进程则是处于内核态关键流程的进程，这些流程是不可打断的，如等待IO响应。

平均负载高有可能是CPU密集型进程导致的，也有可能是IO导致的。

### uptime
```shell
☁  /home/jnjyan/study/http  uptime
 14:16:07 up  5:03,  0 users,  load average: 0.52, 0.58, 0.59
```
当前时间、系统运行时间、正在登陆用户数、过去1min、5min、15min的平均负载。


假设平均负载为2，在一个cpu的机器上，意味着由一半的进程竞争不到CPU，在有两个CPU的机器上，意味着CPU刚好被占用，在4个CPU的系统上，意味着CPU有50%的空闲。

因此，平均负载的理想情况应该等于CPU的个数。
```shell
☁  /home/jnjyan  grep 'model name' /proc/cpuinfo | wc -l
8
```
### stress
stress是一个Linux系统压力测试工具，可以用于一场进程模拟平均负载升高的场景。

### sysstat
sysstat包含了常用的Linux性能工具，用来监控和分析系统的性能。
1. mpstat
    常用的多核CPU性能分析工具，用于实时查看每个CPU的性能指标，以及所有CPU的平均指标。
2. pidstat
    常用的进程性能分析工具，用来实时查看进程的CPU、内存、IO以及上下文切换等性能指标。

## CPU上下文切换

### 进程上下文切换

Linux按照特权等级，将进程的运行空间分为内核空间和用户空间，分别对应着特权等级的Ring0和Ring3。

Ring0具有最高权限，可以直接访问所有资源，Ring3属于用户态，只能访问受限资源，不能直接访问内存等硬件设备，必须通过系统调用陷入到内核中，才能访问这些特权资源。

### 线程上下文切换
### 中断上下文切换
