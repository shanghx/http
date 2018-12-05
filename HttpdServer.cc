#include "HttpdServer.hpp"

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
	/*定义一个对象，启动服务器*/
	HttpdServer *serp = new HttpdServer(atoi(argv[1]));
	serp->InitServer();
	serp->Start();


	/*服务器退出后，释放对象*/
	delete serp;
	return 0;
}
