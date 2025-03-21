#pragma once

/*
noncopyable被继承后,派生类对象可以正常构造和析构
但是派生类对象无法进行拷贝和赋值
原因：
拷贝一个派生类对象时，编译器首先需要拷贝其基类部分
所以先调用基类的拷贝构造函数，然后再调用派生类的拷贝构造函数
而基类的拷贝构造被delete，所以派生类无法拷贝（赋值同理）
*/
class noncopyable{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};