#ifndef __HTTPD_SERVER_HPP__
#define __HTTPD_SERVER_HPP__

#include <pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"
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


	/*启动服务器函数*/
		void Start()
		{
			LOG(INFO, "Start Server Begin!");
			for(;;)
			{
				struct sockaddr_in peer_;
				socklen_t len_ = sizeof(peer_);

				/*建立连接*/
				int sock_ = accept(listen_sock, (struct sockaddr*)&peer_, &len_);
				if(sock_ < 0)
				{
					LOG(WARNING,"accept error!");
					continue;
				}
				
				Task t;
				t.SetTask(sock_, Entry::HandlerRequest);
				tp->PushTask(t);
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
