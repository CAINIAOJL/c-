#ifndef TIMER_COMMON_H
#define TIMER_COMMON_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <utility>
#include <functional>

/**
 * @brief 获取当前时间戳，单位：毫秒
 * @return 当前时间戳
 */
time_t getMSec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

template<typename _User_Data>
class Timer {
public:
    //using TimeOutCallBack = void(_User_Data*); /* 超时回调函数类型 */
    using TimeOutCallBack = std::function<void(_User_Data*)>;

    Timer(int msec) : user_data(nullptr), _cb_func(nullptr) {
        this->expire = getMSec() + msec;
    }

    ~Timer() {

    }

    void setTimeout(int timeout) {
        this->expire = getMSec() + timeout;
    }

    time_t getExpire() const {
        return this->expire;
    }

    void setUserData(_User_Data* data) {
        this->user_data = data;
    }

    void handleTimeout() {
        if(_cb_func) {
            _cb_func(user_data);
        }
    }

    void setCallBack(TimeOutCallBack cb) {
        this->_cb_func = std::move(cb);
    }
private:
    time_t expire;
    _User_Data* user_data;
    TimeOutCallBack _cb_func;
};

template<typename _User_Data>
class ITimerContainer  {
public:
    ITimerContainer() = default;
    virtual ~ITimerContainer() = default;

    virtual void Tick() = 0;
    virtual Timer<_User_Data>* addTimer(time_t timeout) = 0;
    virtual void deleteTimer(Timer<_User_Data>* timer) = 0;
    virtual void resetTimer(Timer<_User_Data>* timer, time_t timeout) = 0;
    virtual time_t getMinExpire() = 0;
};


#endif