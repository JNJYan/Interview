<!--
 * @Author: JNJYan
 * @LastEditors: JNJYan
 * @Email: jjy20140825@gmail.com
 * @Date: 2020-07-09 14:11:44
 * @LastEditTime: 2020-08-11 22:41:10
 * @Description: Modify here please
 * @FilePath: /Interview/redis.md
--> 
[TOC]
# 数据结构
## Simple dynamic string
Redis没有直接使用C语言传统的字符串表示（以空字符结尾的字符数组，以下简称C字符串），而是自己构建了一种名为简单动态字符串（simple dynamic string，SDS）的抽象类型，并将SDS用作Redis的默认字符串表示。

SDS除了用来保存数据库中的字符串值，还可以用作缓冲区(AOF缓冲区、客户端中的输入缓冲区)
```c++
struct sdshdr{
    int len; // 已使用，char字符串中的`\0`不计算在在len中，
    int alloc; // 总大小
    char buf[];
};
```
### sds/c字符串
1. O(1)获取字符串长度
2. 避免了缓冲区溢出，复制字符串时，若空间不足，则先扩展，再拼接。
3. 减少修改字符串长度时所需的内存重分配次数。
4. 二进制安全。
5. 兼容部分C字符串函数。

### 空间预分配
当预留空间不足时，sds增长会进行空间分配，会为SDS分配额外的未使用空间，若增长后的长度小于1M，则预留实际分配长度的空间，即sds实际大小为实际分配的两倍，否则，sds实际大小为实际分配的空间+1M。（以上所述实际分配空间不包含'\0'，实际上应当加1。

### 惰性空间释放
不释放空间，而是作为未使用空间。

### 二进制安全
C字符串不能包含空字符，因此只能保存文本数据，不能保存像图片、音频、视频、压缩文件等二进制数据。

SDS是用len值来判断字符串是否结束。但buf仍然按照C字符串的格式(将末尾置未空字符)，使得保存文本的SDS可以重用`string.h`库定义的部分函数。

## 链表
当一个列表键包含了数量比较多的元素时，又或者列表中包含的元素都是比较长的字符串时，就会采用链表作为列表键的底层实现。

```c++
typedef struct listNode{
    struct listNode* prev;
    struct listNode* next;
    void *value;
} listNode;
typedef struct list{
    listNode* head;
    listNode* tail;
    unsigned long len;
    void* (*dup)(void *ptr); // 复制节点所保存的值
    void* (*free)(void *ptr); // 释放节点所保存的值
    void* (*match)(void *ptr, void *key); // 对比节点所保存的值是否与输入key相等
} list;
```
### 链表特性
1. 链表被广泛用于实现Redis的各种功能，如列表键、发布订阅、慢查询、监视器等
2. 双向无环链表
3. 函数指针实现多态

## 字典
字典使用哈希表作为底层实现
```c++
//dict key-value
typedef struct dictEntry{
    void *key;
    union{
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next;
} dictEntry;
// hash table
typedef struct dictht{
    dictEntry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} dictht;
typedef struct dictType {
    uint64_t (*hashFunction)(const void *key);
    void *(*keyDup)(void* privdata, const void *key);
    void *(*valDup)(void* privdata, const void *obj);
    int (*keyCompare)(void* privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void* privdata, void* key);
    void (*valDestructor)(void* privdata, void* obj);
} dictType;

typedef struct dict{
    dictType *type;
    void *privdata;
    dictht ht[2]; // rehash相关
    long rehashidx; // rehash相关，记录rehash进度，若没有进行rehash，则为1
    unsigned long iterators;
} dict;
```
![](http://www.54manong.com/ebook/%E5%A4%A7%E6%95%B0%E6%8D%AE/20181208232851/Redis%E8%AE%BE%E8%AE%A1%E4%B8%8E%E5%AE%9E%E7%8E%B0%20(%E6%95%B0%E6%8D%AE%E5%BA%93%E6%8A%80%E6%9C%AF%E4%B8%9B%E4%B9%A6)-%E9%BB%84%E5%81%A5%E5%AE%8F%20%E8%91%97/images/000345.jpg)

### 冲突解决
拉链法，且每次都添加到链表头部。

### rehash
1. 为字典的`ht[1]`哈希表分配空间，空间大小取决于要执行的操作，以及`ht[0]`当前包含的键值对数量(`ht[0].used`)。
   1. 若为扩展，则`ht[1]`为第一个大于等于`ht[0].used*2`的`2^n`。
   2. 若为收缩，则`ht[1]`为第一个大于等于`ht[0].used`的`2^n`。
2. 将保存在`ht[0]`中的所有键值对rehash到`ht[1]`上。
3. 当`ht[0]`包含的所有键值对都迁移到了`ht[1]`之后，释放`ht[0]`，将`ht[1]`设置为`ht[0]`，并新建一个`ht[1]`空白哈希表，为下一次rehash做准备。

### rehash触发
负载因子计算公式
$load_factor = ht[0].used / ht[0].size$

1. 服务器目前没有在执行`BGSAVE`或`BGREWRITEAOF`，并且哈希表的负载因子大于等于1。
2. 服务器目前正在执行`BGSAVE`或`BGREWRITEAOF`，并且哈希表的负载因子大于等于5。
3. 当哈希表负载因子小于0.1时，执行收缩操作。

在执行`BGSAVE`或`BGREWRITEAOF`过程中，Redis需要创建当前服务器进程的子进程，而大多数操作系统采用CoW(copy on write)来优化紫禁城的使用效率，所以在进程存在期间，服务器会提高扩展操作所需的负载因子，从而尽可能避免在子进程存在期间进行哈希表扩展操作，尽可能避免不必要的内存写入操作，最大限度地节约内存。

### 渐进式rehash
如果`ht[0]`中包含百万、千万级别的键值对，则不可能一次性将这些键值对都rehash到`ht[1]`。

在rehash执行过程中，先查找`ht[0]`在查找`ht[1]`。新增键值对都保存到`ht[1]`中。

1. 为`ht[1]`分配空间，让字典同时拥有`ht[0]`和`ht[1]`两个哈希表。
2. 在字典中维持一个索引计数器变量rehashidx，并将它的值设置为0，表示rehash工作正式开始。
3. 在rehash期间，每次对字典执行添加、删除、查找、更新操作时，还会顺带将`ht[0]`哈希表在rehashidx索引上的所有键值对rehash到`ht[1]`，当rehash工作完成后，程序将rehashidx属性的值增一。
4. 当`ht[0]`的所有键值对都被rehash到`ht[1]`，将rehashidx属性的值设为-1，表示操作已经完成。


### 字典总结
1. 字典用于实现Redis的各种功能，如数据库或哈希键。
2. 字典使用hash表作为底层实现，每个字典都带有两个哈希表，一个存储，一个用作rehash。

## 跳表
平均查找时间复杂度`O(logN)`，最坏情况`O(N)`。

Redis使用跳表来作为有序集合键的，如果一个有序集合中包含的元素数量比较多，又或者一个有序集合中元素的成员是比较长的字符串时，会采用跳表来实现。

有序集合键和集群节点中用作内部数据结构，除此之外，跳表在Redis里面没有其他用途。

![](http://www.54manong.com/ebook/%E5%A4%A7%E6%95%B0%E6%8D%AE/20181208232851/Redis%E8%AE%BE%E8%AE%A1%E4%B8%8E%E5%AE%9E%E7%8E%B0%20(%E6%95%B0%E6%8D%AE%E5%BA%93%E6%8A%80%E6%9C%AF%E4%B8%9B%E4%B9%A6)-%E9%BB%84%E5%81%A5%E5%AE%8F%20%E8%91%97/images/000365.jpg)


```c++
typedef struct zskiplistNode {
    sds ele;
    double score;
    struct zskiplistNode *backward;
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        unsigned long span;
    } level[];
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;

typedef struct zset {
    dict *dict; // dict中key为元素，value为score，所以可以实现O(1)时间查找分值。
    zskiplist *zsl;
} zset;
```
跳表：
1. header：指向跳表的表头
2. tail：指向跳表的表尾
3. level：记录目前跳表内，层数最大的节点的层数。
4. length：记录跳表的长度，即跳表目前包含的节点数量。

跳表节点：
1. score：节点分值
2. backward：后退指针
3. level：层，包含前进指针和跨度。跨度记录前进指针所指节点与当前节点的距离。
4. ele：成员对象

#### 层
每次创建一个新的跳表节点，都会根据幂次定律(越大的数出现的概率越小)，随机生成一个介于1和32之间的值作为level数组的而大小。

#### 前进指针
每层都有一个指向表尾方向的前进指针，用于从表头到表尾访问节点。

遍历时，从高层向下，访问跨度为一的下一个节点，否则就下降层数。

#### 分值和成员
分值是一个double类型的浮点数，所有节点按照分值从小到大排序。

节点的成员对象是一个sds对象，在同一个跳表中，节点保存的成员对象必须唯一，但是节点保存的分值却可以相同，分值相同的节点按照成员对象在字典序中的大小进行排序。

### 跳表总结
1. 跳表是有序集合的底层实现之一。
2. 每个跳表节点的层高都是1到32之间的随机数。
3. 在同一个条表中，多个节点可以包含相同的分支，但每个结点的成员对象必须唯一。
4. 节点按照分值大小排序，分值相同时，节点按照成员对象的大小进行排序。

## 整数集合
整数集合是集合键的底层实现之一，当一个集合只包含整数数值元素，且集合的元素数量不多，就会使用整数集合作为集合键的底层实现。

```c++
typedef struct intset {
    uint32_t encoding;
    uint32_t length;  //元素数量即contents的长度
    int8_t contents[];  //有序数组，真正类型取决于encoding
} intset;
```

### 整数升级
将一个新元素添加到整数集合中，并且新元素的类型要比整数集合现有所有元素的类型都要长时，整数集合需要进行升级，然后才能将新元素添加到集合中。

1. 根据新元素的类型，扩展整数集合底层数组的空间大小，为新元素分配空间。
2. 将底层数组现有的所有元素都转换成与新元素相同的类型，并将类型转换后的元素放置到对应的位置上，并且要维持底层数组的有序性不变。
3. 将新元素添加到底层数组里面。

添加新元素的时间复杂度为O(n)。

因为引发升级的新元素的长度总是比整数集合中现有所有元素的长度都大，因此这个新元素的值要大于所有现有元素，要么小于所有现有元素。即放在数组头或尾。

不会进行降级。

### 整数集合总结
1. 整数集合是集合键的底层实现之一。
2. 底层实现为数组，这个数组以有序、无重复的方式保存集合元素，根据新加入元素的类型，改变数组的类型。
3. 升级操作为整数集合带来了操作性上的灵活性，并尽可能节约了内存。
4. 整数集合支持升级，不支持降级。

## 压缩列表


# Reids在游戏业务中的应用
游戏一般会使用mongoDB或者Mysql存储游戏信息，但数据存放在硬盘中，读取速度较慢，采用Redis，提高读写速度。

## Cache
Reids缓存简要信息如(在线状态、等级等部分信息)。

## 业务状态
数据量较小，或数据量较大但数据访问频率特别高。

## 避免大Key
大Key表示一个Key对应的value非常大，比如记录玩家的当前在线状态，用一个key为`online_status`的hashmap存储了所有玩家的在线信息`uid:status`。应当拆分为每个玩家做成一个`key-value`的方式，key为`online_status_{uid}`，value为是否在线。原因是Redis是基于Key来做分布式的，若创建了一个大Key，会导致分布式功能失效，所有的请求都会到达同一个Redis节点。

无法检测。

## 避免HotKey
某个Key读写频率很高，繁忙。可以检测。