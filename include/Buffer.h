#pragma once 
#include<vector>
#include<string>
#include<algorithm>
#include<stddef.h>

class Buffer
{
private:
    std::vector<char>buffer_;
    size_t readerIndex;//指向预留空间的末尾，可读数据的起始
    size_t writerIndex;//指向已存储数据的末尾，可写数据的起始
    char* begin(){//起始地址
        return &(*buffer_.begin());
    }
    const char*begin()const{
        return &(*buffer_.begin());
    }
    void  makeSpace(size_t len){
        if(prependableBytes()+writeableBytes()-kCheaPrepend<len){//说明除去预留空间最小大小和等待读的空间，依旧存放不下数据，所以需要扩容
            //buffer_.resize(len+kCheaPrepend-prependableBytes()-writeableBytes());节省一下空间；
            buffer_.resize(writerIndex+len);//直接扩容，添加到可写的后边
        }
        else{
            size_t readable=writerIndex+len;
            std::copy(begin()+readerIndex,begin()+writerIndex,begin()+kCheaPrepend);//copy函数：第一个参数是拷贝的起始位置，第二个位置是结尾，左开右闭，第三个参数是拷贝的位置的起始地址
            readerIndex=kCheaPrepend;
            writerIndex=readerIndex+readable;
        }
    }
public:
    static const size_t kCheaPrepend=8;//初始预留的prepebabel空间大小,即在已有数据前可写数据缓冲区的大小;
    static const size_t kInitialSize=1024;
    explicit Buffer(size_t initialSize=kInitialSize):buffer_(kInitialSize+initialSize),readerIndex(kCheaPrepend),writerIndex(kCheaPrepend){};
    size_t readableBytes()const{//可读区域大小
        return writerIndex-readerIndex;
    }
    size_t writeableBytes()const{//可写区大小
        return buffer_.size()-writerIndex;
    }
    size_t prependableBytes()const{//预备区大小
        return writerIndex;
    }
    //返还缓冲区可读数据的起始地址
    const char* peek(){
        return begin()+readerIndex;
    }
    void retrieve(size_t len){//回收数据
        if(len<readableBytes()){
            readerIndex+=len;
        }
        else{
            retieveAll();
        }
    }
    void retieveAll(){
        readerIndex=writerIndex=kCheaPrepend;
        return;
    }
    //把onmessage函数上报的buffer数据转换成string类
    std::string retrieveAllAsString(){
        return retrieveAsStirng(readableBytes());
    }
    std::string retrieveAsStirng(size_t len){
        std::string result(peek(),len);//string的拷贝构造还能从地址直接拷贝666；
        retrieve(len);//把读出来的数据的内存大小取回
        return result;
    }
    void ensureWritableBytes(size_t len){
        if(len>writeableBytes()){
            makeSpace(len);
        }
        return ;
    }
    void append(const char* data,size_t len){//向buffer里写数据
        ensureWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex+=len;
        return;
    }
    char* beginWrite(){
        return begin()+writerIndex;
    }
    const char* beginWrite() const{
        return begin()+writerIndex;
    }
    ssize_t readFd(int fd,int* saveErrno);
    ssize_t writeFd(int fd,int* saveErrno);
    ~Buffer();
};
