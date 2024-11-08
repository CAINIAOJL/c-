#ifndef HEAP_TIMER2_H
#define HEAP_TIMER2_H


#include <iostream>
#include "timer_common2.h"

template <typename _User_Data>class HeapTimer{public:    HeapTimer() = default;    HeapTimer(int msec)    {        timer.setTimeout(msec);    }
    ~HeapTimer()    {
    }
    void setTimeout(time_t timeout){        timer.setTimeout(timeout);    }
    time_t getExpire()    {        return timer.getExpire();    }
    void setUserData(_User_Data *userData){        timer.setUserData(userData);    }
    int getPos(){        return _pos;    }
    void setPos(int pos){        this->_pos = pos;    }
    void handleTimeOut(){        timer.handleTimeOut();        }
    using TimeOutCbFunc = void (*)(_User_Data *);    void setCallBack(TimeOutCbFunc callBack){        timer.setCallBack(callBack);    }
public:    Timer<_User_Data> timer;  
private:          int _pos;                          // 保存该定时器在数组中的位置，以便查找删除操作            
};


// 定时容器，使用最小堆实现
template <typename _UData>class HeapTimerContainer : public ITimerContainer<_UData> {public:    HeapTimerContainer();    HeapTimerContainer(int capacity);    HeapTimerContainer(HeapTimer<_UData> **initArray, int arrSize, int capacity);    virtual ~HeapTimerContainer() override;
public:    virtual void tick() override;                   Timer<_UData> *addTimer(time_t timeout)  override;    void delTimer(Timer<_UData> *timer)  override;    void resetTimer(Timer<_UData> *timer, time_t timeout)  override;    int getMinExpire() override;    Timer<_UData> *top();    void popTimer();
private:    void percolateDown(int hole);    void percolateUp(int hole);    void resize();    bool isEmpty();
private:    HeapTimer<_UData> **_array;              // 堆数据    
int _capacity;                           // 堆数组的容量    
int _size;                               // 当前包含的元素
};

template <typename _UData>HeapTimerContainer<_UData>::HeapTimerContainer() : HeapTimerContainer(HEAP_DEFAULT_SIZE){
}

template <typename _UData>HeapTimerContainer<_UData>::HeapTimerContainer(int capacity){    this->_capacity = capacity;    this->_size = 0;
    _array = new HeapTimer<_UData> *[capacity]{nullptr};}

template <typename _UData>HeapTimerContainer<_UData>::HeapTimerContainer(HeapTimer<_UData> **initArray, int arrSize, int capacity) :    _size(arrSize){    if(capacity < arrSize)     {        this->_capacity = capacity = 2 * arrSize;    }
    _array = new HeapTimer<_UData> *[capacity];    for (int i = 0; i < capacity; i++)    {        _array[i] = nullptr;    }
    if(arrSize > 0)     {        for (int i = 0; i < arrSize; i++)        {           _array[i] = initArray[i];         }                for(int i = (_size - 1) / 2; i >= 0; i--)        {            percolateDown(i);       //对数组中的第(_size - 1) / 2 ~ 0个元素执行下滤操作        
}    }    }


template <typename _UData>HeapTimerContainer<_UData>::~HeapTimerContainer(){    if(_array)    {        for(int i = 0; i < _size; i++)         {            delete _array[i];        }        delete []_array;    }}

template <typename _UData>void HeapTimerContainer<_UData>::tick(){    std::cout << "----------tick----------" << std::endl;    HeapTimer<_UData> *tmp = _array[0];    time_t cur = getMSec();    // 循环处理到期的定时器    
while(!isEmpty())    {        if(!tmp)        {            break;        }
        // 如果定时器没到期，则退出循环        
        if(tmp->getExpire() > cur)        {            break;        }
        tmp->handleTimeOut();        // 将堆顶元素删除，同时生成新的堆顶定时器        
        popTimer();        tmp = _array[0];    }}


template <typename _UData>Timer<_UData> *HeapTimerContainer<_UData>::addTimer(time_t timeout){    if(_size >= _capacity)    {        this->resize();             //如果容量不够，则进行扩容    

}
    // hole是新建空穴的位置    
    int hole = _size++;    HeapTimer<_UData> *timer = new HeapTimer<_UData>(timeout);    _array[hole] = timer;
    percolateUp(hole);
    return &timer->timer;
    }

template <typename _UData>void HeapTimerContainer<_UData>::delTimer(Timer<_UData> *timer){    if(!timer)     {        return ;    }
    /* 仅仅将目标定时器的数据设置为空，延迟销毁       等定时器超时再删除该定时器     */    timer->setCallBack(nullptr);    timer->setUserData(nullptr);
}

template <typename _UData>void HeapTimerContainer<_UData>::resetTimer(Timer<_UData> *timer, time_t timeout){    // 类型强转    
HeapTimer<_UData> *htimer = reinterpret_cast< HeapTimer<_UData>* >(timer);
    // 找到该定时器在数组中的位置，将其与最后一个定时器的位置交换，然后先进行下滤操作，再进行上滤操作    
    int pos = htimer->getPos();    
    int lastPos = _size - 1;    
    if(pos != lastPos)    {        HeapTimer<_UData> *temp = _array[pos];        _array[pos] = _array[lastPos];        _array[lastPos] = temp;    }    
    timer->setTimeout(timeout);
    // 下滤 上滤    
    percolateDown(pos);    
    percolateUp(lastPos);
}

template <typename _UData>int HeapTimerContainer<_UData>::getMinExpire(){    Timer<_UData> * timer = top();    if(timer)    {        return timer->getExpire();    }
    return -1;}


template <typename _UData>Timer<_UData> *HeapTimerContainer<_UData>::top(){    if(isEmpty())    {        return nullptr;    }
    return &_array[0]->timer;}

template <typename _UData>void HeapTimerContainer<_UData>::popTimer(){    if(isEmpty())    {        return;    }
    if(_array[0])    {        delete _array[0];        // 将原来的堆顶元素替换为堆数组中最后一个元素        
    _array[0] = _array[--_size];        // 对新的堆顶元素执行下滤操作        
    percolateDown(0);    }}




#endif

