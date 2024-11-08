#ifndef __TIMER_COMMON2_H__
#define __TIMER_COMMON2_H__

#include <stdio.h>
#include <sys/time.h>

#define HEAP_DEFAULT_SIZE 128
time_t getMSec(){    struct timeval tv;    gettimeofday(&tv, NULL);    return tv.tv_sec * 1000 + tv.tv_usec / 1000;}


template <typename _User_Data>class Timer{public:    Timer() : _user_data(nullptr), _cb_func(nullptr) {};    Timer(int msec) : _user_data(nullptr), _cb_func(nullptr)    {        this->_expire = getMSec() + msec;    }
    ~Timer()    {
    }

    void setTimeout(time_t timeout){        this->_expire = getMSec() + timeout;    }

    time_t getExpire()    {        return _expire;    }

    void setUserData(_User_Data *userData){        this->_user_data = userData;    }

    void handleTimeOut(){        if(_cb_func)        {            _cb_func(_user_data);        }  }

    using TimeOutCbFunc = void (*)(_User_Data *);    
    void setCallBack(TimeOutCbFunc callBack){        this->_cb_func = callBack;    }


private:     
    time_t _expire;                    // 定时器生效的绝对时间                
    _User_Data *_user_data;            // 用户数据    
    TimeOutCbFunc _cb_func;           // 超时时的回调函数
};

template <typename _UData>class ITimerContainer {public:    ITimerContainer() = default;    virtual ~ITimerContainer() = default;
public:    virtual void tick() = 0;                   virtual Timer<_UData> *addTimer(time_t timeout) = 0;    virtual void delTimer(Timer<_UData> *timer) = 0;    virtual void resetTimer(Timer<_UData> *timer, time_t timeout) = 0;    virtual int getMinExpire() = 0;};



#endif