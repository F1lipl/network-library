#pragma once

#include<iostream>
#include<string>

class Timestamp
{
private:
    int64_t microSencondsSinceEpoch_;
public:
    Timestamp(/* args */);
    explicit Timestamp(int64_t microSencondsSinceEpoch);
    static Timestamp now();
    std::string toString()const;
    ~Timestamp();
};
