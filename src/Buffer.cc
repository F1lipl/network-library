#include<errno.h>
#include<sys/uio.h>
#include<unistd.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd,int* saveErrno){
    char extrabuf[65536]={0};//额外数据缓冲区
    /*
    struct iovec{
       ptr_t iov_base;//iov_base指向缓冲区存放的是readv所接收的数据或是writev将要发送的数据
       size_t iov_len;//iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度；    
    }
    */
    struct iovec vec[2];//使用iovec分配两个连续的缓冲区
    const size_t writable=writeableBytes();//buffer缓冲区的可写空间大小，不一定能完全存下
    vec[0].iov_base=begin()+writable;
    vec[0].iov_len=writable;
    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof(extrabuf);
    const int iovcnt=(writable<sizeof(extrabuf))?2:1;
    const ssize_t n=::readv(fd,vec,iovcnt);//分散读；
    if(n<0){
        *saveErrno= errno;
    }
    else if(n<=writable){
        writerIndex+=n;
    }
    else{//n>writable
        writerIndex=buffer_.size();
        append(extrabuf,n-writable);//对buffer进行扩容，并将extrabuf存储的另一部分数据存储追加至buffer_;
    }
    return n;
}
ssize_t Buffer::writeFd(int fd,int* saveErrno){
    ssize_t n=::write(fd,peek(),readableBytes());
    if(n<0){
        *saveErrno=errno;
    }
    return n;
}