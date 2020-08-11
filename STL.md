<!--
 * @Author: JNJYan
 * @LastEditors: JNJYan
 * @Email: jjy20140825@gmail.com
 * @Date: 2020-07-10 09:57:30
 * @LastEditTime: 2020-08-08 09:37:51
 * @Description: Modify here please
 * @FilePath: /Interview/STL.md
--> 
# 空间配置器allocator
## allocator接口
```c++
allocator::value_type
allocator::pointer
allocator::const_pointer
allocator::reference
allocator::const_reference
allocator::size_tyep
allocator::difference_type
allocator::rebind

allocator::allocator()
allocator::allocator(const allocator&)
template <class U>allocator::allocator<const allocator<U>&>
allocator::~allocator()
pointer allocator::address(reference x)const
const_pointer allocator::address(const_reference x)const
pointer allocator::allocate(size_type n, const void*=0)
void allocator::deallocate(pointer p, size_type n)
size_type allocator::max_size() const
void allocator::construct(pointer p, const T& x)
void allocator::destroy(pointer p)
```

## 具备次配置能力的SGI空间配置器
SGI STL的配置其名称为alloc而非allocator，且不接受任何参数。
```c++
template<class T, class Alloc=alloc> //缺省使用alloc为配置器
```

### std::allocator
SGI也有allocator的配置器，但其效率不佳，只是将c++的`::operator new/delete`做了一层封装。

### std::alloc
new过程分为两个：1.分配内存，2.调用构造函数。

alloc将这两阶段操作分为`allocate/deallocate`和`construct/destroy`。

#### construct()/destroy()
construct接受一个指针p和初值value，将初值设定到指针所指的空间上，采用placement new来实现该操作。

destroy有两个版本，第一个版本接收一个指针，将指针指向的对象析构，第二个版本接收头尾迭代器，将[head, tail)的所有对象析构，首先通过`value_type()`获得迭代器所指对象的类型，再利用`__type_traits<T>`判断该类别的析构函数是否无关痛痒，若是就什么都不做，若否才遍历整个范围，调用前述第一个版本的destroy()。

### alloc
对象构造前的空间配置和对象析构后的释放由`<stl_alloc.h>`负责。

1. 从堆请求空间
2. 考虑多线程状态
3. 考虑内存不足时的应变策略
4. 考虑内存碎片问题

SGI设计了双层级配置器，第一级别配置器使用`malloc/free`，第二级别视情况采用不同的策略：
当请求内存大于128bytes时，调用第一级配置器，模拟c++的`set_new_handler`处理内存不足的状况；小于128bytes时，采用复杂的memory pool整理方式，不再求助于第一级配置器。通过维护16个自由链表负责16钟小型区块的次配置能力。

自由链表trick，采用union结构作为指针链表，union结构包含两个对象，一个指向相同union的指针，另一个时指向实际空闲内存，当内存已被分配，就可以指向下一个union，未被分配则指向内存。

### allocate()接口
此函数首先判断区块大小，大于128字节就调用第一级配置器，小于128bytes就检查对应的自由链表，若自由链表中有可用的区块，就拿来用，若没有，则将区块大小上调至8的倍数边界，然后调用`refill()`为free list重新填充空间。

### deallocate()接口
首先判断区块大小，大于128字节就调用第一级配置器，小于128字节就找出对应的自由链表，将区块挥回收。

#### refill()
发现自由链表中没有可用区块，调用该函数为自由链表重新填充空间，新的空间将取自内存池(`chunk_alloc()`),缺省火得20个新区块，空间不足时，而言可能小于20。

### 内存池memory pool
`chunk_alloc()`从内存池中取空间给自由链表使用，若内存池空间足够，则直接调出20个区块给自由链表。当一个区块时也不能提供时，就调用`malloc()`从堆钟分配。若堆空间也不足，`chunk_alloc()`就四处寻找尚有未用区块、且区块够大的自由链表，找到了就交出，找不到就调用第一级配置器。因为第一级配置器有new-handler机制，或许会有机会释放其他内存来使用，若可以就成功，若不可以发生`bad_alloc`异常。



## 内存基本处理工具
STL定义有五个全局函数，作用域未初始化空间上。

`construct(),destroy(),uninitialized_copy(),uninitialized_fill(),uninitialized_fill_n()`，后三个分别对应于高层次函数`copy(),fill(),fill_n()`。头文件`<memory>`
### uninitialized_copy
```c++
template<calss InputIterator, class ForwardIterator>
ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result);
```
能够将内存的配置和对象的构造行为分离开，若[result, result+(last-first))范围内的每一个迭代器都指向未初始化区域，就会调用`copy constructor`给[first, last)范围内的每个对象生成一个副本，放入目标位置。

c++规定了`uninitialized_copy()`句有`commit or rollback`语义，要么构造出所有元素，要么不构造任何东西，即失败调用析构。

### uninitialized_fill
```c++
template<calss ForwardIterator, class T>
ForwardIterator uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& x);
```
同样能将内存配置和对象的构造行为分离开，若[first,last)范围内的每个迭代器都指向未初始化内存，那么该函数就会在该范围内产生x的复制，即该函数会针对操作范围内的每个迭代器，调用`construct(&*i, x)`，在迭代器i所指之处生成x的拷贝。

同样保证`commit or rollback`语义。
### uninitialized_fill_n
```c++
template<calss ForwardIterator, class Size, class T>
ForwardIterator uninitialized_fill_n(ForwardIterator first, Size n, const T& x);
```
同样将内存配置和构造分离，并为指定范围的所有元素设定相同的初值。若[first, first+n)范围内的每一个迭代器都指向未初始化的内存，那么`uninitialized_fill_n`会调用`copy constructor`，在该范围内生成x的拷贝。

同样保证`commit or rollback`语义。

函数逻辑是，首先取出迭代器first的value_type，然后判断该类型是否为POD(Plain Old Data)类型。POD就是标量类型，或者传统的struct类型，POD类型必然拥有trivial ctor/dtor/copy/assignment函数，因此可以对POD类型采用最有效率的初值填写手法，而对非POD类型采取最保险安全的做法。

# 迭代器iterators
迭代器模式：提供一种方法，使之能够依序遍历某个容器内的所有元素，而又无需暴露该容器的内部表达方式。

STL的中心思想在于：将数据容器和算法分开，即容器和算法的泛型化。利用模板类和模板函数来实现。

## 迭代器失效
### vector
`push_back()`之后，end操作返回的迭代器肯定会失效。

在内存重新分配之后，所有迭代器失效。插入元素若重新分配内存，则所有迭代器失效，若没有重新分配内存则，插入元素之后的所有元素迭代器失效。

删除元素时，被删除元素之后的任何元素的迭代器都将失效。

### deque
在首部和尾部插入元素不会使任何迭代器失效。

在首部和尾部删除元素，会使指向被删除元素的迭代器失效。

在deque容器的任何其他位置插入和删除操作，会使指向该容器元素的所有迭代器都失效。

### list
除了删除元素时，指向被删除元素的迭代器失效外，其他迭代器都不会受影响。

### stack/queue/priority_queue
适配容器，不能遍历。

### set/multiset/map/multimap
若删除元素，则指向该删除元素的迭代器失效，其他操作均不会失效。


# 序列式容器
vector、string、deque、list

## erase/remove
vector中的erase和algorithm中的remove有什么区别

erase是真的删除了元素，迭代器不能再访问了

remove只是简单地将要remove的元素移到了容器最后面，迭代器仍然能访问。

## list/vector
vector是顺序线性表，地址是连续的，类似于数组，支持随机访问，但是插入和删除元素的时候需要将改动元素后面的所有元素前移或后移，当容量不足时，需要重新分配一块大的内存，然后将vector中所有的元素拷贝过去。因此在扩容时vector效率较低。

list是线性链表，地址可以不是连续的，实现是一个双向链表，只能通过指针来访问数据，不支持随机访问，访问一个元素时，需要遍历整个链表，但是插入和删除元素时只需要更改前后两个节点的头尾指针即可，可以在O(1)实现插入和删除。

vector的迭代器可以看作是一个普通的指针，由于内存连续，所以能支持`+/+=/<`等操作。但是list内存是不连续的，所以不支持`+/+=/<`等操作。

## deque/vector
deque是一个动态数组。地址空间是分段连续。

1. deque比vector多了`push_front()`和`pop_front()`两个函数。可以对首部进行操作。
2. deque中不存在`capacity()`和`reserve()`成员函数。
3. deque的内存是分段连续的，当需要进行扩容时，只是加一段定量的内存。




# 关联式容器
set、multiset、map、multimap

## map实现
红黑树(平衡二叉树)
1. 节点为红色或黑色
2. 根节点是黑色的
3. 所有的叶子节点都为空，并且为黑色
4. 若父节点为红，则子节点都为黑色，即不存在父子节点都是红色
5. 某个节点到其子孙节点的简单路径上，都包含相同数目的黑色节点

# 适配容器
stack、queue、priority_queue

# 算法

# 仿函数

# 配接器adapters

