#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include "heap_timer.h"


using std::cout;
using std::endl;

#define PORT 6666
#define MAX_EVENTS 1024
#define MAX_BUF_SIZE 1024

struct Event;

using readHandle = void(*)(Event *, ITimerContainer<Event> *);
using writeHandle = void(*)(Event *, ITimerContainer<Event> *);

struct Event{    
    int fd;    
    char ip[64];    
    uint16_t port;    
    epoll_event event; 
    void *timer;
    char buf[MAX_BUF_SIZE];    
    int buf_size;
    readHandle read_cb;
    writeHandle write_cb;
};

int epfd;
int pipefd[2];

// 超时处理的回调函数
void timeout_handle(Event *cli){    
    if(cli == nullptr)    
    {        
        return ;    
    }        
    cout << "Connection time out, fd:" << cli->fd << " ip:[" << cli->ip << ":" << cli->port << "]" << endl;
    
    epoll_ctl(epfd, EPOLL_CTL_DEL, cli->fd, &cli->event);
    
    close(cli->fd);
    delete cli;
}

void err_exit(const char *reason){    
    cout << reason << ":" << strerror(errno) << endl;    
    exit(1);
}

// 设置非阻塞 ---->epoll
int setNonblcoking(int fd)
{    
    int old_option = fcntl(fd, F_GETFL);    
    int new_option = old_option | O_NONBLOCK;    
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 设置端口复用
void setReusedAddr(int fd){    
    int reuse = 1;    
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

//server端初始化
int socket_init(unsigned short port, bool reuseAddr){    
    int fd = socket(AF_INET, SOCK_STREAM, 0);    
    if(fd < 0)    
    {        
        err_exit("socket error");    
    }
    if(reuseAddr)    
    {        
        setReusedAddr(fd);    
    }
    struct sockaddr_in addr;    
    bzero(&addr, 0);    
    addr.sin_family = AF_INET;    
    addr.sin_port = htons(port);    
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));  

    if(ret < 0)    
    {        
        err_exit("bind error");    
    }

    setNonblcoking(fd);
    ret = listen(fd, 128);    
    if(ret < 0)    
    {        
        err_exit("listen error");    
    }
    return fd;
}


void readData(Event *ev, ITimerContainer<Event> *htc)
{    
    ev->buf_size = read(ev->fd, ev->buf, MAX_BUF_SIZE - 1);    
    if(ev->buf_size == 0)    
    {        
        close(ev->fd);        
        htc->deleteTimer((Timer<Event> *)ev->timer);        
        epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &ev->event);        
        cout << "Remote Connection has been closed, fd:" << ev->fd << " ip:[" << ev->ip << ":" << ev->port << "]" << endl;        
        delete ev;                
        return;    
    }
    ev->event.events = EPOLLOUT;    
    epoll_ctl(epfd, EPOLL_CTL_MOD, ev->fd, &ev->event);
    
}

void writeData(Event *ev, ITimerContainer<Event> *htc)
{    
    write(ev->fd, ev->buf, ev->buf_size);
    ev->event.events = EPOLLIN;    
    epoll_ctl(epfd, EPOLL_CTL_MOD, ev->fd, &ev->event);
    // 重新设置定时器    
    htc->resetTimer((Timer<Event> *)ev->timer, 15000);
}

void acceptConn(Event *ev, ITimerContainer<Event> *htc)
{    Event *cli = new Event;    
    struct sockaddr_in cli_addr;    
    socklen_t sock_len = sizeof(cli_addr);    
    int cfd = accept(ev->fd, (struct sockaddr *)&cli_addr, &sock_len);    
    if(cfd < 0)    
    {        
        cout << "accept error, reason:" << strerror(errno) << endl;        
        return;    
    }     
    setNonblcoking(cfd);
    cli->fd = cfd;    
    cli->port = ntohs(cli_addr.sin_port);    
    inet_ntop(AF_INET, &cli_addr.sin_addr, cli->ip, sock_len);    
    cli->read_cb = readData;    
    cli->write_cb = writeData;
    auto timer = htc->addTimer(15000);      //设置客户端超时值15秒    
    timer->setUserData(cli);    
    timer->setCallBack(timeout_handle);    
    cli->timer = (void *)timer;
    cli->event.events = EPOLLIN;    
    cli->event.data.ptr = (void *) cli;    
    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cli->event);
    cout << "New Connection, ip:[" << cli->ip << ":" << cli->port << "]" << endl;
}


void sig_handler(int signum){    
    char sig = (char) signum;    
    write(pipefd[1], &sig, 1);
}


int add_sig(int signum){    
    struct sigaction sa;    
    memset(&sa, 0, sizeof(sa));    
    sa.sa_handler = sig_handler;    
    sa.sa_flags |= SA_RESTART;    
    sigfillset(&sa.sa_mask);
    return sigaction(signum, &sa, nullptr);
}

int main(int argc, char *argv[])
{    // 信号处理    
    int ret = add_sig(SIGINT);    
    if(ret < 0)    
    {        
        err_exit("add sig error");    
    }    
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);    
    if(ret < 0)    
    {        
        err_exit("socketpair error");    
    }
    int fd = socket_init(PORT, true);   
    Event server;    
    Event sig_ev;    
    server.fd = fd;    
    sig_ev.fd = pipefd[0];        
    epfd = epoll_create(MAX_EVENTS);    
    if(epfd < 0)    
    {        
        err_exit("epoll create error");    
    }
    sig_ev.event.events = EPOLLIN;    
    sig_ev.event.data.ptr = (void *) &sig_ev;;
    server.event.events = EPOLLIN;    
    server.event.data.ptr = (void *)&server;
    epoll_ctl(epfd, EPOLL_CTL_ADD, pipefd[0], &sig_ev.event);    
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &server.event);
    cout << "------ Create TimerContainer ------" << endl;
    ITimerContainer<Event> *htc = new HeapTimerContainer<Event>;
    cout << "------ Create TimerContainer over ------" << endl;
    struct epoll_event events[MAX_EVENTS];    
    int nready = 0;
    int timeout = 1000;      //设置超时值为10秒    
    char buf[1024] = {0};    
    bool running = true;    
    while(running)    
    {        // 将定时容器中定时时间最短的时长作为epoll_wait的最大等待时间        
        auto min_expire = htc->getMinExpire();        
        timeout = ((int)min_expire == -1) ? 10000 : (int)min_expire - getMSec();                
        nready = epoll_wait(epfd, events, MAX_EVENTS, timeout);        
        if(nready < 0)        
        {            
            cout << "epoll wait error, reason:" << strerror(errno) << endl;        
        } else if(nready > 0)  {            // 接收新的连接            
            for(int i = 0; i < nready; i++)            
            {                
                Event *ev =  (Event *) events[i].data.ptr;                // 接受新的连接    
                if(ev->fd == pipefd[0])                
                {                       
                    int n = read(pipefd[0], buf, sizeof(buf));                    
                    if(n < 0)                    
                    {                        
                        cout << "deal read signal error:" << strerror(errno) << endl;                        
                        continue;                     
                    }  else if(n > 0)  {                        
                        for(int i = 0; i < n; i++)                        
                        {                            
                            switch (buf[i])                            
                            {                            
                                case SIGINT:                                
                                running = false;                                
                                break;                            
                            }                         
                        }                    
                    }                
                }  else if(ev->fd == fd ) {                    
                    acceptConn(ev, htc);                
                }  else if(ev->event.events & EPOLLIN) {                    
                    ev->read_cb(ev, htc);                
                }  else if(ev->event.events & EPOLLOUT) {                    
                    ev->write_cb(ev, htc);                
                }            
            }      
        } else        
        {            
            htc->Tick();        
        }    
    }
    
    close(fd);     
    close(pipefd[0]);     
    close(pipefd[1]);     
    delete htc;
    return 0;
}