#ifndef __HTTPD_SERVER_HPP__
#define __HTTPD_SERVER_HPP__

#include <pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"
#include <sys/epoll.h>
class HttpdServer{

  private:
    int listen_sock;//监听socket
    int port;//端口
    ThreadPool *tp;//线程池变量
  public:
    /*构造函数*/
    HttpdServer(int port_):listen_sock(-1),port(port_)
  {}


    /*服务器初始化*/
    void InitServer()
    {
      listen_sock = socket(AF_INET, SOCK_STREAM, 0);
      if(listen_sock < 0)
      {
        LOG(ERROR, "create socket error!");		
        exit(2);
      }
      /*端口复用,当服务器挂掉时可以立即重启*/
      int opt_ = 1;
      setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt_, sizeof(opt_));

      /*定义地址*/
      struct sockaddr_in local_;
      local_.sin_family = AF_INET;
      local_.sin_port = htons(port);
      local_.sin_addr.s_addr = INADDR_ANY;

      /*socket和地址绑定*/
      if(bind(listen_sock, (struct sockaddr*)&local_, sizeof(local_)) < 0)
      {
        LOG(ERROR, "bind socket error!");
        exit(3);
      }

      /*开始监听*/
      if(listen(listen_sock, 5) < 0)
      {	
        LOG(ERROR, "listen socket error!");
        exit(4);
      }

      /*到这一步,说明已经监听到socket了*/
      LOG(INFO, "InitServer Success!");

      /*此时要将线程池初始化*/
      tp = new ThreadPool();
      tp->InitThreadPool();
    }

		void setnonblock(int fd)
		{
    	int flag = fcntl(fd, F_GETFL, 0);
    	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
		}

    /*启动服务器函数*/
    void Start()
    {
      LOG(INFO, "Start Server Begin!");
      // 采用epoll来监听就绪事件
      //1.创建epoll句柄
      int epfd = epoll_create(10);
      if(epfd < 0)
      {
        LOG(ERROR,"epoll_create error!");
        exit(5);
      }

      //2.向内核添加事件
      struct epoll_event ev;
      ev.events = EPOLLIN;
      ev.data.fd = listen_sock;
      epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev);

      while(1)
      {
        struct epoll_event evs[100];
        int nfds = epoll_wait(epfd, evs, 100, -1);
        if(nfds < 0)
        {
          LOG(ERROR, "epoll_wait error!");
          exit(6);
        }
        else if(nfds == 0)
        {
          LOG(INFO,"have no data arrived now!");
          continue;
        }
        //此时说明有就绪事件
        int i = 0;
        for(i = 0;i < nfds; i++)
        {
          //判断就绪的是监听描述符还是读写就绪描述符
          if(evs[i].data.fd == listen_sock)
          {
            struct sockaddr_in peer_;
            socklen_t len_ = sizeof(peer_);
            int new_fd = accept(listen_sock, (struct sockaddr*)&peer_, &len_);
            if(new_fd < 0)
            {
              LOG(WARNING,"accept error!");
              continue;
            }
            LOG(INFO,"have a data arrived!");
						//setnonblock(new_fd);
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = new_fd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev);
          }
          else 
          {
              int sock_ = evs[i].data.fd;
              Task t;
              t.SetTask(sock_, Entry::HandlerRequest);
              tp->PushTask(t);
              //epoll_ctl(epfd, EPOLL_CTL_DEL, sock_, NULL);
          }
         }
        }
      }

      /*析构函数*/
      ~HttpdServer()
      {
        if(listen_sock != -1)
        {
          close(listen_sock);
        }
        port = -1;
      }
};

#endif
