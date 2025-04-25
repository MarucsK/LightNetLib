# muduo
![GitHub License](https://img.shields.io/github/license/xykCs/muduo?color=%23FFFF00)

This project is a reconstruction of the Muduo network library using C++11, based on studying and referencing its source code. It removes Muduo's dependency on the Boost library and implements a multi-Reactor, multi-threaded model according to Muduo's design principles. It's a C++ high-concurrency TCP network library based on non-blocking I/O and event-driven architecture, using the one loop per thread model where each thread can only have one EventLoop.

### Development Environment
- Linux kernel version 6.8.0-52-generic (ubuntu 22.04)
- CMake 3.22.1
- gcc 11.4.0

### Build
Run the autobuild script:
```sh
./autobuild.sh
```

### License
This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more details.
