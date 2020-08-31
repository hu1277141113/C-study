#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <iostream>
#include <string>
#include <sys/time.h>
#define Normal 1
#define Warning 2
#define Fatal 3

//[错误级别][时间][错误信息][所在文件][行数]

uint64_t GetTimeStamp()
{
    struct timeval time;
    if(gettimeofday(&time,nullptr)==0)
    {
        return time.tv_sec;
    }
    return 0;
}

void Log(std::string level,std::string msg,std::string file_name,int num)
{
    std::cerr<<"[ "<<level<<" ][ "<<GetTimeStamp()<<" ][ "<<msg<<" ][ "<<file_name<<" ][ "<<num<<" ]"<<std::endl;
}

#define LOG(level,message) Log(#level,message,__FILE__,__LINE__)

#endif
