#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include "timer_common.h"
#include <iostream>

#define MAX_HEAP_SIZE 128

template<typename _User_Data>
class HeapTimer {

public:
    HeapTimer() = default;
    HeapTimer(int msec) : timer(msec) {
        timer.setTimeout(msec);
    }
    ~HeapTimer() {

    }

    void setTimeout(time_t msec) {
        timer.setTimeout(msec);
    }

    time_t getExpire() const {
        return timer.getExpire();
    }

    void setUserData(_User_Data* data) {
        timer.setUserData(data);
    }

    int getPos() const {
        return _pos;
    }

    void setPos(int pos) {
        _pos = pos;
    }

    void handleTimeout() {
        timer.handleTimeout();
    }

    using TimeOutCallBack = void(_User_Data*);

    void setTimeCallback(TimeOutCallBack callback) {
        timer.setCallBack(callback);
    } 

public:
    Timer<_User_Data> timer;
private:
    int _pos;

};

template<typename _UData> 
class HeapTimerContainer : public ITimerContainer<_UData> {
public:
    HeapTimerContainer();
    HeapTimerContainer(int capacity);
    HeapTimerContainer(HeapTimer<_UData> ** initarray, int capacity, int size);
    virtual ~HeapTimerContainer() override;

public:
    virtual void Tick() override;
    Timer<_UData> * addTimer(time_t timeout) override;
    void deleteTimer(Timer<_UData>* timer) override;
    void resetTimer(Timer<_UData>* timer, time_t timeout) override;
    time_t getMinExpire() override;
    Timer<_UData>* top();
    void popTimer();


private:
    void percolateDown(int hole);
    void percolateUp(int hole);
    void resize();
    bool isEmpty() const;

private:
    int _capacity;
    int _size;
    HeapTimer<_UData> ** _array;
};

template<typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer(): HeapTimerContainer(MAX_HEAP_SIZE) {

}

template<typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer(int capacity) {
    this->_capacity = capacity;
    this->_size = 0;
    _array = new HeapTimer<_UData>* [capacity]{nullptr};
}

template<typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer(HeapTimer<_UData> ** initarray, int capacity, int size): _size(size) {
    //不够就扩容
    if(capacity < size) {
        this->_capacity = capacity = 2 * size;
    }

    _array = new HeapTimer<_UData>* [capacity];
    for(int i = 0; i < capacity; i++) {
        _array[i] = nullptr;
    }

    if(size > 0) {
        for(int i = 0; i < size; i++) {
            _array[i] = initarray[i];
        }

        for(int i = (_size - 1) >> 1; i >= 0; i--) {
            //下标从0开始，所以除以2
            percolateDown(i);
        }
    }
}

template<typename _UData>
HeapTimerContainer<_UData>::~HeapTimerContainer() {
    if(_array) {
        //先删除每一个元素
        for(int i = 0; i < _size; i++) {
            delete _array[i];
        }
        delete[] _array;
    }
}

template<typename _UData>
void HeapTimerContainer<_UData>::Tick() {
    std::cout << "-----------------tick---------------" <<std::endl;
    HeapTimer<_UData>* temp = _array[0];
    time_t cur = getMSec();//获取当前时间
    while(!isEmpty()) {
        if(!temp) {
            break;
        }

        if(temp->getExpire() > cur) {
            //说明时间未到
            break;
        }

        //否则时间到了
        temp->handleTimeout();

        popTimer();//弹出最小的
        //循环处理下一个
        temp = _array[0];
    }
}

template<typename _UData>
Timer<_UData>* HeapTimerContainer<_UData>::addTimer(time_t timeout) {
    if(_size >= _capacity) {
        this->resize(); //扩容
    }

    int hole = _size++;
    //创建定时器
    HeapTimer<_UData>* timer = new HeapTimer<_UData>(timeout);
    _array[hole] = timer;

    //上浮到正确位置
    percolateUp(hole);

    return &timer->timer;
}

template<typename _UData>
void HeapTimerContainer<_UData>::deleteTimer(Timer<_UData>* timer) {
    if(!timer) {
        return;
    }

    /* 仅仅将目标定时器的数据设置为空，延迟销毁      
    等定时器超时再删除该定时器     */
    //timer->setCallBack(nullptr);
    //timer->setUserData(nullptr);

    HeapTimer<_UData>* heaptimer = reinterpret_cast<HeapTimer<_UData>*>(timer);
    int pos = heaptimer->getPos();
    delete _array[pos];
    _array[pos] = _array[--_size];
    percolateDown(pos);

}

template<typename _UData>
void HeapTimerContainer<_UData>::resetTimer(Timer<_UData>* timer, time_t timeout) {
    HeapTimer<_UData>* heaptimer = reinterpret_cast<HeapTimer<_UData>*>(timer);

    int pos = heaptimer->getPos();
    int lastPos = _size - 1;
    if(pos != lastPos) {
        HeapTimer<_UData>* cur = _array[pos];
        _array[pos] = _array[lastPos];
        _array[lastPos] = cur;
    }
    timer->setTimeout(timeout);

    percolateUp(lastPos);
    percolateDown(lastPos);
}

template<typename _UData>
time_t HeapTimerContainer<_UData>::getMinExpire() {
    Timer<_UData>* timer = top();
    if(timer) {
        return timer->getExpire();
    }
    return -1;//失败
}


template<typename _UData>
Timer<_UData>* HeapTimerContainer<_UData>::top() {
    if(isEmpty() || _array[0] == nullptr) {
        return nullptr;
    }
    return &_array[0]->timer; //问题 <unknown> <unnamed>::timer
}

/*template <typename _UData>
Timer<_UData> *HeapTimerContainer<_UData>::top() {  
    if(isEmpty()){
        return nullptr;   
    }
    return &_array[0]->timer;
}*/


template<typename _UData>
void HeapTimerContainer<_UData>::popTimer() {
    if(isEmpty()) {
        return;
    }

    if(_array[0]) {
        delete _array[0];
        _array[0] = _array[_size - 1];
        --_size;
        percolateDown(0);
    }
}

/*template<typename _UData>
void HeapTimerContainer<_UData>::percolateDown(int hole) {
    if(_size == 0) {
        return;
    }
    
    int child = 0;

    HeapTimer<_UData>* temp = _array[hole];
    for(; (((hole << 1) + 1) <= _size - 1); hole = child) {
        
        child = hole << 1 + 1;
        if((child < (_size - 1)) && (_array[child + 1]->getExpire() < _array[child]->getExpire()))
        {            
            child++;        
        }
        if(_array[child]->getExpire() < temp->getExpire()) {
            _array[hole] = _array[child];
            _array[hole]->setPos(hole);
        } else {
            break;
        }
    }
    _array[hole] = temp;   
    _array[hole]->setPos(hole);
}*/

template<typename _UData>
void HeapTimerContainer<_UData>::percolateDown(int hole) {
    if (_size == 0) {
        return;
    }

    int child = 0;
    HeapTimer<_UData>* temp = _array[hole];
    for (; (hole * 2 + 1) < _size; hole = child) {
        child = (hole * 2) + 1;
        if (child + 1 < _size && _array[child + 1]->getExpire() < _array[child]->getExpire()) {
            child++;
        }
        if (_array[child]->getExpire() < temp->getExpire()) {
            _array[hole] = _array[child];
            _array[hole]->setPos(hole);
        } else {
            break;
        }
    }
    _array[hole] = temp;
    _array[hole]->setPos(hole);
}

template<typename _UData>
void HeapTimerContainer<_UData>::percolateUp(int hole){
    int parent = 0;
    HeapTimer<_UData>* tmep = _array[hole];

    for(; hole > 0; hole = parent) {
        parent = (hole - 1) >> 1;
        if(_array[parent]->getExpire() <= tmep->getExpire()) {
            break;
        }
        _array[hole] = _array[parent];
        _array[hole]->setPos(hole);
    }
    _array[hole] = tmep;
    _array[hole]->setPos(hole);
}

template <typename _UData>
void HeapTimerContainer<_UData>::resize() {
    HeapTimer<_UData>** temp = new HeapTimer<_UData>*[_capacity * 2];
    _capacity *= 2;
    for(int i = 0; i < _size; i++) {
        temp[i] = _array[i];
    }
    for(int i = _size; i < _capacity; i++) {
        temp[i] = nullptr;
    }

    delete[] _array;
    _array = temp;
}

template <typename _UData>
bool HeapTimerContainer<_UData>::isEmpty() const {
    return _size == 0;
}

#endif //HEAP_TIMER_H