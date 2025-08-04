#pragma once
class noncopyable{
public:
/*
delete关键字，显示的禁止函数
即禁止子类使用拷贝构造和赋值构造
*/
    noncopyable(const noncopyable&)=delete;
    noncopyable operator=(const noncopyable&)=delete;
protected:
    noncopyable()=default;
    ~noncopyable()=default;
};