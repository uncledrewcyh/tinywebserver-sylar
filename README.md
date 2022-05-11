# TinyWebServer-sylar 基于协程的C++高性能服务器框架
## 快速运行
### 环境
* 服务器测试环境
  * Ubuntu 20.04.3 LTS
  * MySQL版本8.0.29
* 浏览器测试环境
  * Windows、Linux均可
  * Chrome
  * FireFox
  * 其他浏览器暂无测试
### 依赖库安装
* 测试前确认已安装MySQL数据库
  ```C++
  sudo apt-get install mysql-client
  sudo apt-get install mysql-server
  sudo apt-get install libmysql++-dev
  ```
  ```C++
  // 建立yourdb库
  create database yourdb;
  
  // 创建user表
  USE yourdb;
  CREATE TABLE user(
      username char(50) NULL,
      passwd char(50) NULL
  )ENGINE=InnoDB;
  
  // 添加数据
  INSERT INTO user(username, passwd) VALUES('name', 'passwd');
  ```
* 测试前确认已安装YAML-CPP
  ```C++
  git clone https://github.com/jbeder/yaml-cpp.git
  cd yaml-cpp
  cd build && mkdir build
  make
  make install
  ```  
* 测试前确认已安装boost
  ```C++
  sudo apt install libdev-all-dev
  ```       
### 配置文件
* 修改`conf/TinyWebserverConfig.yml`中的服务器初始化信息

  ```YAML
   host: "0.0.0.0:9006" # 主机地址
   db:                  # 数据库信息
      url: localhost
      user: root
      password: root
      databasename: yourdb
      port: 3306
      maxconn: 15
   logs:                # 日志配置
      - name: root
         level: info
         appenders:
            - type: FileLogAppender
               foramt: '%d{%Y-%m-%d %H:%M:%S} %t %N %F [%p] [%c] %f:%l %m%n'
               file: /home/pudge/workspace/pudge-server/logs/root.txt
            - type: StdoutLogAppender
               foramt: '%d{%Y-%m-%d %H:%M:%S} %t %N %F [%p] [%c] %f:%l %m%n'
               
      - name: system
         level: info
         appenders:
            - type: FileLogAppender
               foramt: '%d{%Y-%m-%d %H:%M:%S} %t %N %F [%p] [%c] %f:%l %m%n'
               file: /home/pudge/workspace/pudge-server/logs/system.txt
            - type: StdoutLogAppender
               foramt: '%d{%Y-%m-%d %H:%M:%S} %t %N %F [%p] [%c] %f:%l %m%n'
  ```
### 编译运行
* build

  ```C++
  sh ./build.sh
  ```

* 启动server

  ```C++
  ./bin/tiny_webserver
  ```

* 浏览器端
  ```C++
  ip:9006
  ```
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

`Thread`：线程类，构造函数传入线程入口函数和线程名称，线程入口函数类型为`std::function<void()>`。在`pthread_create`绑定`Thread::run`函数，并将`Thread`的`this`指针作为参数传入，在`Thread::run`中完成线程生存期变量`t_thread`和`t_thread_name`的赋值以及成员变量`thread->m_id`的赋值.

`Semaphore`: 为了确保构造线程池时，每个成员变量`thread->m_id`已经初始化成功完成.

线程同步类（这部分被拆分到mutex.h)中：  

`Semaphore`: 计数信号量，基于sem_t实现  
`Mutex`: 互斥锁，基于pthread_mutex_t实现  
`RWMutex`: 读写锁，基于pthread_rwlock_t实现  
`Spinlock`: 自旋锁，基于pthread_spinlock_t实现  

### 协程模块

协程：用户态的线程，相当于线程中的线程，更轻量级。后续配置socket相关的系统调用的hook，可以把复杂的异步调用，封装成同步操作。降低业务逻辑的编写复杂度。该协程是基于`ucontext_t`来实现的，利用`makecontext`和`swapcontext`等api实现协程的创建和切换. 协程栈大小可由`g_fiber_stack_size`配置变量进行修改配置.

非对称协程调度模型:
```
子协程SubFiber只能和主协程MainFiber切换，而不能和另一个子协程切换.
Thread --> MainFiber <--> SubFiber
              ^
              |
              v
            subFiber  
```
实现方法：
利用如下两个变量进行调度切换：
```cpp
static thread_local Fiber* t_fiber = nullptr; ///当前调用协程
static thread_local Fiber::ptr t_threadFiber = nullptr; ///主协程
```

`t_threadFiber` 在 `MainFiber` 的构造中进行初始化。

```
首次调用GetThis(), 此时无任何运行协程 -> new Fiber -> shared from this 

-> t_threadFiber = main_fiber;

```

`t_fiber` 在 `SetThis` 的中进行赋值

三处`SetThis`, 出现在新的协程切换进来时。
```
Fiber::Fiber() -> setThis()

Fiber::call()  -> setThis()

Fiber::swapIn() -> setThis()
```

协程有五种状态
```
/// 初始化状态
INIT,
/// 暂停状态
HOLD,
/// 执行中状态
EXEC,
/// 结束状态
TERM,
/// 可执行状态
READY,
/// 异常状态
EXCEPT
```
1. 协程类的构造函数初始化默认为 `INIT`状态，主协程构造中直接赋值为`EXEC`(默认是在运行的)
2. `HOLD`: 暂停状态，让出CPU执行权，完成异步系统调用的关键点，在Hook的Socket API中，如果出现阻塞状态，则切换为该状态.
3. `EXEC`：在`swapIn`和`call`以及`Fiber()`中进行设置，标志该协程享有CPU执行权.
4. `TERM`: 在协程`callBack`函数执行完毕后设置，标志协程已经执行完毕.
5. `READY`: 只在`YieldToReady`中出现，尽量不使用该切换函数.因为协程的执行应该由协程自主决定，若是出现该状态，则调度器只是把它重新加入到任务队列中.
6. `EXCEPT`在协程`callBack`中抛出异常时出现，同TERM同样处理。

### 协程调度模块

`Scheduler`协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，也可以指定协程在固定的线程中执行。是一个N-M的协程调度模型，N个线程，M个协程。重复利用每一个线程。

两种运行模式：
1.主线程参与调度，也用于消息队列的添加. 主线程负责任务的消息队列`m_fibers`的添加, 副线程负责协程的调度.  `use_caller == true`，如下
```
MainThread --> MainFiber <-->  m_rootFiber <--> workFiber     (第 1 类)[需要陷入~IOManager中才进行调度]
                   |               |
                   |               |
                   v               v
                schedule          run
                   
SubThread  --> MainFiber <--> workFiber                       (第 2 类)
                   |
                   |               
                   v 
                  run
                  ...
```
2.主线程不参与调度, 可用于消息队列`m_fibers`的添加. `use_caller == false`，如下

```
MainThread --> MainFiber  
                   |               
                   |               
                   v               
                schedule          
                               
SubThread  --> MainFiber <--> workFiber
                   |
                   |               
                   v 
                  run
                  ...
```
单独使用由于`Idle`函数一直循环，造成调度线程一直占用CPU，最好不单独使用，可配合`epoll`使用.

### IO协程调度模块
```
   IOManager     <- Scheduler
      ^
      |
TimerManager
```
继承自协程调度器`Scheduler`，封装了`epoll（Linux）`，支持注册socket fd事件回调。只支持读写事件。IO协程调度器解决了协程调度器在idle情况下CPU占用率高的问题，当调度器idle时，调度器会阻塞在epoll_wait上，此时无协程任务执行，则让出线程的CPU执行权.当IO事件发生或添加了新调度任务时再返回, 其余阻塞的调度线程，可以通过一对`pipe fd`来实现通知调度协程有新任务(`tickle`)。

### 定时器模块

`IOManager`的父类`TimerManager`，功能是在指定超时时间结束之后执行回调函数。定时的实现机制是idle协程的`epoll_wait`超时，大体思路是创建定时器时指定超时时间和回调函数，然后以当前时间加上超时时间计算出超时的绝对时间点，然后所有的定时器按这个超时时间点排序，从最早的时间点开始取出超时时间作为idle协程的epoll_wait超时时间，epoll_wait超时结束时把所有已超时的定时器收集起来，执行它们的回调函数。注意一点，如果出现更早需要执行的任务, 需要执行`onTimerInsertedAtFront`(执行`tickle`方法)去唤醒调度线程，对`epoll_wait`的超时时间进行修正，确保更早的任务能完成。

### Hook模块

hook系统底层和socket创建和设置相关的API，socket io相关的API，以及sleep系列的API。

1. 通过hook模块，可以使一些不具异步功能的API，展现出异步的性能, 比如说 Mysql，通过hook改造后的`socket`和`read`系统调用实现了异步操作.
2. 使得后台逻辑服务几乎不用修改逻辑代码就可以完成异步化改造。
3. 可以用写同步代码的方式实现异步的性能，避免了实现异步带来回调地狱。

实现方法: 通过动态库的全局符号介入功能，用自定义的接口来替换掉同名的系统调用接口。由于系统调用接口基本上是由C标准函数库libc提供的，所以这里要做的事情就是用自定义的动态库来覆盖掉libc中的同名符号, 利用系统调用`dlsym`找到被覆盖掉的libc中的实际系统调用函数，根据hook是否开启，来选择是否执行hook后的系统调用。

该框架对以下函数进行了hook，并且只对socket fd进行了hook，如果操作的不是socket fd，那会直接调用系统原本的api，而不是hook之后的api：
```
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

### Socket模块
封装了`Socket`类和统一的`Address`地址(包含TCP套接字，UDP套接字，Unix域套接字)类，提供了如下方法:
1. 创建各种类型的套接字对象的方法
2. 设置套接字选项，比如超时参数
3. bind/connect/listen方法，实现绑定地址、发起连接、发起监听功能 
4. accept方法，返回连入的套接字对象
5. 发送、接收数据的方法
6. 获取本地地址、远端地址的方法
7. 获取套接字类型、地址类型、协议类型的方法
8. 取消套接字读、写的方法


### ByteArray序列化模块
ByteArray二进制序列化模块，提供对二进制数据的常用操作。提供读写入基础类型int8_t,int16_t,int32_t,int64_t等，Protobuff(Varint和ZagZig)基础类型的读写，TLV格式序列化字符串的读写, 支持序列化到文件，以及从文件反序列化等功能。

ByteArray的底层存储是固定大小的块，以链表形式组织。每次写入数据时，将数据写入到链表最后一个块中，如果最后一个块不足以容纳数据，则分配一个新的块并添加到链表结尾，再写入数据。ByteArray会记录当前的操作位置，每次写入数据时，该操作位置按写入大小往后偏移，如果要读取数据，则必须调用setPosition重新设置当前的操作位置。

### TcpServer模块
基于Socket类，封装了一个通用的TcpServer的服务器类，提供简单的API，使用便捷，可以快速绑定一个或多个地址，启动服务，监听端口，accept连接，处理socket连接等功能。具体业务功能更的服务器实现，只需要继承该类就可以快速实现

TcpServer类采用了Template Pattern设计模式，它的HandleClient是交由继承类来实现的。使用TcpServer时，必须从TcpServer派生一个新类，并重新实现子类的handleClient操作，example中的`TinyWebserver`即是这样操作。

### Stream模块
封装流式的统一接口。将文件，socket封装成统一的接口。使用的时候，采用统一的风格操作。基于统一的风格，可以提供更灵活的扩展。所有的流结构都继承自抽象类Stream，Stream类规定了一个流必须具备read/write接口(纯虚函数)和readFixSize/writeFixSize接口，继承自Stream的类必须实现备read/write接口。

现已实现套接字流结构`SocketStream`，将套接字封装成流结构，以支持Stream接口规范，除此外，SocketStream还支持套接字关闭操作以及获取本地/远端地址的操作。

### HTTP模块
采用Ragel（有限状态机，性能媲美汇编），实现了HTTP/1.1的简单协议实现和uri的解析。基于SocketStream实现了HttpSession (HTTP服务器端的链接）。基于TcpServer实现了HttpServer。提供了完整的HTTP的客户端API请求功能，HTTP基础API服务器功能.

### Servlet模块
仿照java的servlet，实现了一套Servlet接口，实现了ServletDispatch，FunctionServlet。NotFoundServlet。支持uri的精准匹配，模糊匹配(`fnmatch`)等功能。和HTTP模块，一起提供HTTP服务器功能


