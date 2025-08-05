#include<time.h>

#include<Timestamp.h>

Timestamp::Timestamp(int64_t microSencondsSinceEpoch):microSencondsSinceEpoch_(microSencondsSinceEpoch){}
Timestamp Timestamp:: now(){
    return Timestamp(time(nullptr));//time(null)返回现在的距离1970年的时间间隔；
}

std::string Timestamp::toString() const{
    char buf[128]={0};
    tm *tm_time=localtime(&microSencondsSinceEpoch_);
    //格式化时间戳，用tm结构体的形式存储年月日时秒分；
    //看的出来tm里存的是整型
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d:%02d",tm_time->tm_year+1900,
        tm_time->tm_mon+1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec
    );//snprintf跟sprintf是差不多的，它限制了最多可写缓冲区的大小，比sprintf更安全
    //年/月/日 时：分：秒；
    return buf;
}