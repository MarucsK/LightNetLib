# muduo
![GitHub License](https://img.shields.io/github/license/xykCs/muduo?color=%23FFFF00)

This project is a reconstruction of the Muduo network library using C++11, based on studying and referencing its source code. It removes Muduo's dependency on the Boost library and implements a multi-Reactor, multi-threaded model according to Muduo's design principles. It's a C++ high-concurrency TCP network library based on non-blocking I/O and event-driven architecture, using the one loop per thread model where each thread can only have one EventLoop.

### Concurrency model
![image](https://github.com/user-attachments/assets/886c52a7-aa8e-4bb0-a306-75ddaebafb06)
The project adopts a master-slave multi-Reactor multi-thread model. The MainReactor is solely responsible for monitoring and dispatching new connections. Within the MainReactor, an Acceptor receives incoming connections and distributes them to SubReactors using a predefined round-robin algorithm. 

The SubReactors handle the read/write events of their assigned connections. 

When the start function of the TcpServer is called, a thread pool is internally created. Each thread in the pool independently runs an event loop (a SubReactor). The MainReactor selects a SubReactor from the thread pool via round-robin scheduling and assigns new connections to it. 

The number of SubReactors dedicated to handling read/write events is typically equal to the number of CPU cores, ensuring optimal resource utilization and parallelism. 

### QuickStart
This project runs on Linux and requires the following dependencies:
- CMake 3.0 and above
- g++ 4.8.1 and above

run the autobuild script:
```sh
./autobuild.sh
```
