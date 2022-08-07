# HP_Server
从0搭建一个跨平台高性能服务器

version 1.0
### 语言: c++
平台: visual studio
消息定义: 结构体消息
技术点：
1. 服务端和客户端的select模型
2. 多线程处理连接和通信
3. 设计二级缓冲区处理系统内核缓冲区溢出的问题
4. 解决因设置缓冲区导致的粘包少包问题————拆包、拼包
5. 服务端设计生产者消费者模式，主线程接收到的客户端连接分配给子线程建立收发包通信
6. 生产者和消费者之间建立消息缓冲队列作为代理，对客户端连接分流[自解锁、原子计数、高精度计数]
实现：
1. 服务端单线程————客户端1000连接————5.5W 包/s
2. 服务端四线程————客户端1000连接————30W  包/s

version 2.0
语言: c++
平台: visual studio
消息定义: 结构体消息
技术点：
continue
实现：
continue
