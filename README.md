# HP_Server
**从0搭建一个跨平台高性能服务器**

### version 1.0
### 语言: c++
### 平台: visual studio 2017
### 消息定义: 结构体消息
### 技术点：
1. 服务端和客户端的select模型
2. 多线程处理连接和通信
3. 设计二级缓冲区处理系统内核缓冲区溢出的问题
4. 解决因设置缓冲区导致的粘包少包问题————拆包、拼包
5. 服务端设计生产者消费者模式，主线程接收到的客户端连接分配给子线程建立收发包通信
6. 生产者和消费者之间建立消息缓冲队列作为代理，对客户端连接分流[自解锁、原子计数、高精度计数]
### 实现：
1. 服务端单线程————客户端1,000连接————发包【1000B / 1个】————5.5W 包/s
![单线程1000客户端连接](https://user-images.githubusercontent.com/72861097/183279738-153dd02b-d6f9-42d4-91d1-2580ad2b59dc.jpg)
2. 服务端四线程————客户端1,000连接————发包【1000B / 1个】————30W  包/s
![s4c1000](https://user-images.githubusercontent.com/72861097/183279724-3ce151f0-fd72-447a-a619-f3fa1f4eb8a8.jpg)

---

### version 2.0
### 语言: c++
### 平台: visual studio 2017
### 消息定义: 结构体消息
### 技术点：
1. 使用visual studio 性能探测器，发现资源消耗主要有三部分：select拷贝、recv、send
   * 使用fd_set_copy 保存上一次的fd集合, 若没有新的客户端加入或退出, 默认使用副本进行传递，节约了每次重置fd_set的资源
   * 测试服务器单发单收的能力，发现：recv > send,  因此分离收发业务，提高性能
2. 服务端为发送业务添加定时发送和定量发送功能
3. 内存管理：实现更稳定的收发
    * 开辟内存池——从系统中申请足够大小的内存，由程序自己管理【避免外部内存碎片的产生】，内存池并不能提高消息收发的速率，只是能保证程序处于一种稳定的状态。*具体做法：重载new*
    * 智能指针——`std::shared_ptr<classA> cA = std::make_shared<classA>();`
    * 对象池——创建足够多的对象，避免创建释放对象的消耗。*具体做法：封装对象池类，由需要使用的函数继承即可*
4. 心跳检测：60s内无心跳则断开与客户端的连接
5. 使用条件变量和互斥锁实现可靠的信号机制
6. 消息异步发送
7. 添加日志记录功能
8. 封装线程库
9. 模块分装，降低耦合度
### 实现：
1. 服务器4线程————客户端4线程1,000连接———发包【100B / 10个】————35W 包/s
![c1000](https://user-images.githubusercontent.com/72861097/183801353-8749f5f8-06a5-4512-8b15-f8067ee49136.jpg)
2. 服务器4线程————客户端4线程10,000连接————发包【100B / 10个】————120W 包/s
![c10000](https://user-images.githubusercontent.com/72861097/183801550-761a9287-fb86-4b07-9e4d-ba88bfd579ca.jpg)
