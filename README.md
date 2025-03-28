# muduo
![GitHub License](https://img.shields.io/github/license/xykCs/muduo?color=%23FFFF00)

This project is a reconstruction of the Muduo network library using C++11, based on studying and referencing its source code. It removes Muduo's dependency on the Boost library and implements a multi-Reactor, multi-threaded model according to Muduo's design principles. It's a C++ high-concurrency TCP network library based on non-blocking I/O and event-driven architecture, using the one loop per thread model where each thread can only have one EventLoop.

### Concurrency model
![image](https://github.com/user-attachments/assets/886c52a7-aa8e-4bb0-a306-75ddaebafb06)
The project adopts a master-slave multi-Reactor multi-thread model. The MainReactor is solely responsible for monitoring and dispatching new connections. Within the MainReactor, an Acceptor receives incoming connections and distributes them to SubReactors using a predefined round-robin algorithm. The SubReactors handle the read/write events of their assigned connections. 

When the start function of the TcpServer is called, a thread pool is internally created. Each thread in the pool independently runs an EventLoop (a SubReactor). The MainReactor selects a SubReactor from the thread pool via round-robin scheduling and assigns new connections to it. 

The number of SubReactors dedicated to handling read/write events is typically equal to the number of CPU cores. 

### Development Environment
- Linux kernel version 6.8.0-52-generic (ubuntu 22.04)
- CMake 3.22.1
- gcc 11.4.0

### QuickStart
Run the autobuild script:
```sh
./autobuild.sh
```

### License
This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more details.
