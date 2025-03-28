# muduo
![GitHub License](https://img.shields.io/github/license/xykCs/muduo?color=%23FFFF00)

This project is a reconstruction of the Muduo network library using C++11, based on studying and referencing its source code. It removes Muduo's dependency on the Boost library and implements a multi-Reactor, multi-threaded model according to Muduo's design principles. It's a C++ high-concurrency TCP network library based on non-blocking I/O and event-driven architecture, using the one loop per thread model where each thread can only have one EventLoop.

### QuickStart
This project runs on Linux and requires the following dependencies:
- CMake 3.0 and above
- g++ 4.8.1 and above

run the autobuild script:
```sh
./autobuild.sh
```
