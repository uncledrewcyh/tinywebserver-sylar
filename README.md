# TinyWebServer-sylar 基于协程的C++高性能服务器框架

## 模块概述

### 日志模块
支持流式日志风格写日志和格式化风格写日志。

流式日志使用：
```cpp
pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("log_name");
PUDGE_LOG_INFO(g_logger) << "this is a log";
```

格式化日志使用：
```cpp
pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("log_name");
PUDGE_LOG_FMT_INFO(g_logger, "%s", "this is a log"); 
```

日志格式器使用:
日志格式器，执行日志格式化，负责日志格式的初始化。
解析日志格式，将用户自定义的日志格式，解析为对应的FormatItem。
```

日志格式举例：%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
格式解析：
%d{%Y-%m-%d %H:%M:%S} : %d 标识输出的是时间 {%Y-%m-%d %H:%M:%S}为时间格式，可选 DateTimeFormatItem
%T : Tab[\t]               TabFormatItem
%t : 线程id                ThreadIdFormatItem
%N : 线程名称              ThreadNameFormatItem
%F : 协程id                FiberIdFormatItem
%p : 日志级别              LevelFormatItem       
%c : 日志名称              NameFormatItem
%f : 文件名                FilenameFormatItem
%l : 行号                  LineFormatItem
%m : 日志内容              MessageFormatItem
%n : 换行符[\r\n]          NewLineFormatItem
str: 普通字符串            StringFormatItem
```

与日志模块相关的类：

`LogEvent`: 日志事件，用于保存日志信息，比如，日期时间，线程/协程id，文件名/行号等，以及日志消息内容。

`LogEventWarp`: 日志事件包装器，将日志事件与日志器包装在一起，通过宏定义简化日志模块的使用，当包装器析构时候，则调用日志器`Logger`的`log`方法进行输出；

`LogFormatter`: 日志格式器，包含了`FormatItem`的日志格式化内容的抽象基类及其派生类，新增加的格式内容通过继承该抽象基类，对日志格式化内容就行修改增加. 构造时可指定一个格式模板字符串`m_pattern`，根据该模板字符串定义的模板项对一个日志事件进行格式化，解析出日志格式内容对应的`FormatItem`对象，如普通字符串对应的是`StringFormatItem`类型, 生成对应的`m_items`解析结果。提供`format`方法对`LogEvent`对象进行格式化输出并返回对应的字符串(格式化日志)或流(流式日志)，实质上是遍历`m_iterms`调用`FormatItem`的`format`方法进行输出。

`LogAppender`：日志输出器，用于将日志输出到不同的目的地，比如终端和文件等。`LogAppender`内部包含一个`LogFormatter`成员，提供`log`方法对`LogEvent`对象进行输出到不同的目的地。这是一个抽象基类，通过继承的方式可派生出不同的Appender，目前默认提供了StdoutAppender和FileAppender两个类，用于输出到终端和文件。

`Logger`: 日志器，用于写日志，包含名称，日志级别两个属性，以及数个LogAppender成员对象，一个日志事件经过判断高于日志器自身的日志级别时即会启动Appender进行输出。日志器默认不带Appender，需要用户进行手动添加。

`LoggerManager`：日志器管理类，单例模式，包含全部的日志器集合，用于创建或获取日志器。`LoggerManager`初始化时自带一个`root`日志器，这为日志模块提供一个初始可用的日志器。

### 配置模块

采用约定优于配置的思想。定义即可使用。支持变更通知功能。使用YAML文件做为配置内容，配置名称大小写不敏感。支持级别格式的数据类型，支持STL容器(vector,list,set,map等等), 支持自定义类型的支持（需要实现序列化和反序列化方法)。

使用方式如下：

```cpp
pudge::ConfigVar<std::string>::ptr g_host_addr = 
            pudge::Config::Lookup("host", std::string("0.0.0.0:9006"), "host ip config");
```

定义了一个服务器主机地址参数，可以直接使用`g_host_addr->getValue()`获取参数的值.

对应的yaml文件格式如下：

```yaml
host: "0.0.0.0:9006"
```

与配置模块相关的类：

`ConfigVarBase`: 配置参数基类，定义了一个配置参数的基本属性和方法，比如名称，描述，以及toString/fromString两个纯虚函数，配置参数的类型和值由继承类实现。

`ConfigVar`: 由`ConfigVarBase`派生于的配置参数类，这是一个模板类，有三个模板参数，一个是类型T，另外两个是`FromStr`和`ToStr`反序列化和序列化的类类型仿函数(支持偏特化方法)，共中`FromStr`用于将字符串转类型`T`，`ToStr`用于将`T`转字符串。这两个模板函数以`Boost`库的`boost::lexical_cast`为基础，通过定义不同的偏特化仿函数类来实现`基础数据类型的STL类`和的序列化与反序列化，通过定义不同的特例化仿函数类来实现自定义类型的转换，其中`Log`模块、`Mysql_pool`模块的参数配置均是利用了特例化仿函数进行读取配置。ConfigVar类包含了一个`T`类型的配置参数值成员和一个参数变更回调函数数组. `toString/fromString`用到了模板参数`toStr/fromStr`。此外，ConfigVar还提供了`setValue/getValue`方法用于设置/修改值，并触发回调函数数组，以及`addListener/delListener`方法用于添加或删除回调函数。

`Config`: ConfigVar的管理类，采用单例模式确保整个文件只有一个实例化对象。包含有`ConfigVarMap`成员来管理全部的`ConfigVar`对象。提供`Lookup`方法，根据参数名称查询配置参数。如果调用Lookup查询时同时提供了默认值和配置描述信息，那么在未找到对应的配置时，会创建一个新的配置项，这样就保证了配置模块定义即可用的特性。除此外，Config类还提供了`LoadFromYaml`和`LoadFromConfDir`两个方法，用于从YAML结点或定义的配置文件路径中加载配置文件。。

### 线程模块

线程模块，封装了pthread里面的一些常用功能，Thread,Semaphore,Mutex,RWMutex等对象，可以方便开发中对线程日常使用。

线程模块相关的类：

Thread：线程类，构造函数传入线程入口函数和线程名称，线程入口函数类型为void()，如果带参数，则需要用std::bind进行绑定。线程类构造之后线程即开始运行，构造函数在线程真正开始运行之后返回。

线程模块相关的类：

`Thread`：线程类，构造函数传入线程入口函数和线程名称，线程入口函数类型为void()，如果带参数，则需要用std::bind进行绑定。线程类构造之后线程即开始运行，构造函数在线程真正开始运行之后返回。

线程同步类（这部分被拆分到mutex.h)中：  

`Semaphore`: 计数信号量，基于sem_t实现  
`Mutex`: 互斥锁，基于pthread_mutex_t实现  
`RWMutex`: 读写锁，基于pthread_rwlock_t实现  
`Spinlock`: 自旋锁，基于pthread_spinlock_t实现  


### 协程模块

协程：用户态的线程，相当于线程中的线程，更轻量级。后续配置socket hook，可以把复杂的异步调用，封装成同步操作。降低业务逻辑的编写复杂度。 目前该协程是基于ucontext_t来实现的，后续将支持采用boost.context里面的fcontext_t的方式实现。

协程原语：

`resume`：恢复，使协程进入执行状态  
`yield`: 让出，协程让出执行权

yield和resume是同步的，也就是，一个协程的resume必然对应另一个协程的yield，反之亦然，并且，一条线程同一时间只能有一个协程是执行状态。

### 协程调度模块

协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，也可以指定协程在固定的线程中执行。是一个N-M的协程调度模型，N个线程，M个协程。重复利用每一个线程。

限制：  
一个线程只能有一个协程调度器
潜在问题：  
调度器在idle情况下会疯狂占用CPU，所以，创建了几个线程，就一定要有几个类似while(1)这样的协程参与调度。

### IO协程调度模块

继承自协程调度器，封装了epoll（Linux），支持注册socket fd事件回调。只支持读写事件。IO协程调度器解决了协程调度器在idle情况下CPU占用率高的问题，当调度器idle时，调度器会阻塞在epoll_wait上，当IO事件发生或添加了新调度任务时再返回。通过一对pipe fd来实现通知调度协程有新任务。

### 定时器模块

在IO协程调度器之上再增加定时器调度功能，也就是在指定超时时间结束之后执行回调函数。定时的实现机制是idle协程的epoll_wait超时，大体思路是创建定时器时指定超时时间和回调函数，然后以当前时间加上超时时间计算出超时的绝对时间点，然后所有的定时器按这个超时时间点排序，从最早的时间点开始取出超时时间作为idle协程的epoll_wait超时时间，epoll_wait超时结束时把所有已超时的定时器收集起来，执行它们的回调函数。

sylar的定时器以gettimeofday()来获取绝对时间点并判断超时，所以依赖系统时间，如果系统进行了校时，比如NTP时间同步，那这套定时机制就失效了。sylar的解决办法是设置一个较小的超时步长，比如3秒钟，也就是epoll_wait最多3秒超时，如果最近一个定时器的超时时间是10秒以后，那epoll_wait需要超时3次才会触发。每次超时之后除了要检查有没有要触发的定时器，还顺便检查一下系统时间有没有被往回调。如果系统时间往回调了1个小时以上，那就触发全部定时器。个人感觉这个办法有些粗糙，其实只需要换个时间源就可以解决校时问题，换成clock_gettime(CLOCK_MONOTONIC_RAW)的方式获取系统的单调时间，就可以解决这个问题了。

### Hook模块

hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。通过hook模块，可以使一些不具异步功能的API，展现出异步的性能。如（mysql）

hook实际就是把系统提供的api再进行一层封装，以便于在执行真正的系统调用之前进行一些操作。hook的目的是把socket io相关的api都转成异步，以便于提高性能。hook和io调度是密切相关的，如果不使用IO协程调度器，那hook没有任何意义。考虑IOManager要调度以下协程：  
协程1：sleep(2) 睡眠两秒后返回  
协程2：在scoket fd1上send 100k数据  
协程3：在socket fd2上recv直到数据接收成功   

在未hook的情况下，IOManager要调度上面的协程，流程是下面这样的：
1. 调度协程1，协程阻塞在sleep上，等2秒后返回，这两秒内调度器是被协程1占用的，其他协程无法被调度。
2. 调度协徎2，协程阻塞send 100k数据上，这个操作一般问题不大，因为send数据无论如何都要占用时间，但如果fd迟迟不可写，那send会阻塞直到套接字可写，同样，在阻塞期间，调度器也无法调度其他协程。
3. 调度协程3，协程阻塞在recv上，这个操作要直到recv超时或是有数据时才返回，期间调度器也无法调度其他协程

上面的调度流程最终总结起来就是，协程只能按顺序调度，一旦有一个协程阻塞住了，那整个调度器也就阻塞住了，其他的协程都无法执行。像这种一条路走到黑的方式其实并不是完全不可避免，以sleep为例，调度器完全可以在检测到协程sleep后，将协程yield以让出执行权，同时设置一个定时器，2秒后再将协程重新resume，这样，调度器就可以在这2秒期间调度其他的任务，同时还可以顺利的实现sleep 2秒后再执行的效果。send/recv与此类似，在完全实现hook后，IOManager的执行流程将变成下面的方式：  
1. 调度协程1，检测到协程sleep，那么先添加一个2秒的定时器，回调函数是在调度器上继续调度本协程，接着协程yield，等定时器超时。
2. 因为上一步协程1已经yield了，所以协徎2并不需要等2秒后才可以执行，而是立刻可以执行。同样，调度器检测到协程send，由于不知道fd是不是马上可写，所以先在IOManager上给fd注册一个写事件，回调函数是让当前协程resume并执行实际的send操作，然后当前协程yield，等可写事件发生。
3. 上一步协徎2也yield了，可以马上调度协程3。协程3与协程2类似，也是给fd注册一个读事件，回调函数是让当前协程resume并继续recv，然后本协程yield，等事件发生。
4. 等2秒超时后，执行定时器回调函数，将协程1 resume以便继续执行。
5. 等协程2的fd可写，一旦可写，调用写事件回调函数将协程2 resume以便继续执行send。
6. 等协程3的fd可读，一旦可读，调用回调函数将协程3 resume以便继续执行recv。

上面的4、5、6步都是异常的，系统并不会阻塞，IOManager仍然可以调度其他的任务，只在相关的事件发生后，再继续执行对应的任务即可。并且，由于hook的函数对调用方是不可知的，调用方也不需要知道hook的细节，所以对调用方也很方便，只需要以同步的方式编写代码，实现的效果却是异常执行的，效率很高。

hook的重点是在替换api的底层实现的同时完全模拟其原本的行为，因为调用方是不知道hook的细节的，在调用被hook的api时，如果其行为与原本的行为不一致，就会给调用方造成困惑。比如，所有的socket fd在进行io调度时都会被设置成NONBLOCK模式，如果用户未显式地对fd设置NONBLOCK，那就要处理好fcntl，不要对用户暴露fd已经是NONBLOCK的事实，这点也说明，除了io相关的函数要进行hook外，对fcntl, setsockopt之类的功能函数也要进行hook，才能保证api的一致性。

sylar对以下函数进行了hook，并且只对socket fd进行了hook，如果操作的不是socket fd，那会直接调用系统原本的api，而不是hook之后的api：  

```cpp
sleep
usleep
nanosleep
socket
connect
accept
read
readv
recv
recvfrom
recvmsg
write
writev
send
sendto
sendmsg
close
fcntl
ioctl
getsockopt
setsockopt
```
