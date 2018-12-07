#include "HttpdServer.hpp"
#include <signal.h>
#include  <unistd.h>

static void Usage(std::string proc_)
{
	std::cout << "Usage " << proc_ << " prot" << std::endl;
	
}

//./ HttpdServer 8080

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
		exit(1);
		
	}

	/*在启动服务之前,先将SIGPIPE信号忽略，防止断开连接时，程序退出*/
	signal(SIGPIPE, SIG_IGN);

	/*定义一个对象，启动服务器*/
	HttpdServer *serp = new HttpdServer(atoi(argv[1]));
	serp->InitServer();
//	daemon(1,0);//将当前进程设为守护进程
	serp->Start();


	/*服务器退出后，释放对象*/
	delete serp;
	return 0;
}
