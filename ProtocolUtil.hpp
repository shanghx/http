#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__


#include <iostream>
#include <string>

#include <string.h>
#include <strings.h>
#include <sstream>
#include <unordered_map>


#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#include "Log.hpp"


#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400
#define SERVER_ERROR 500


#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define PAGE_404 "404.html"

#define HTTP_VERSION "http/1.0"

std::unordered_map<std::string, std::string> stuffix_map{

	{".html", "text/html"},
		{".htm", "text/html"},
		{".css", "text/css"},
		{".js", "application/x-javascript"}
};

class ProtocolUtil{
	public:
		/*保存请求报头的键值对在KVmap中*/
		static void MakeKV(std::unordered_map<std::string, std::string>& kv_, std::string& str_)
		{
			std::size_t pos = str_.find(": ");
			if(std::string::npos == pos)
			{
				return ;
			}

			std::string k_ = str_.substr(0, pos);
			std::string v_ = str_.substr(pos+2);

			kv_.insert(make_pair(k_, v_));
			return ;
		}
		static std::string IntToString(int code)
		{
			std::stringstream ss;
			ss << code;
			return ss.str();
		}


		static std::string CodeToDesc(int code)
		{
			switch(code)
			{
	 			case 200:
					return "OK";
				case 404:
					return "Not Found";
	 			case 400:
					return "Bad Request";
				case 500:
					return "Internal Server Error";
				default:
					return "Unknown";
			}
		}

		static std::string SuffixToType(const std::string &suffix_)
		{
			return stuffix_map[suffix_];
		}
};

class Request{

	public:
		std::string rq_line;//请求行
		std::string rq_head;//请求报头
		std::string blank;//空行
		std::string rq_text;//请求正文
	private:
		std::string method;//请求方法
		std::string uri;//URI
		std::string version;//版本号
		bool cgi;//method = POST,GET ->uri

		std::string path;//路径
		std::string param;//参数

		int resource_size;//资源大小
		std::string resource_suffix;//资源后缀
		std::unordered_map<std::string, std::string> head_kv;//键值对
		int content_length;//正文长度

	public:
		Request()
			:blank("\n")
			 ,cgi(false)
			 ,path(WEB_ROOT)//有疑问
			 ,resource_size(0)
			 ,content_length(-1)
			 ,resource_suffix(".html")
		{}

		std::string &GetParam()
		{
			return param;
		}

		int GetContentLength()
		{
			std::string cl_ = head_kv["Content-Length"];
			if(!cl_.empty())
			{
		 		std::stringstream ss(cl_);
				ss >> content_length;
			}
			return content_length;
		}

		/*获取资源大小*/
		int GetResourceSize()
		{
			return resource_size;
		}

		/*设置资源大小*/
		void SetResourceSize(int rs_)
		{
			resource_size = rs_;
		}

		/*获取资源类型*/
		std::string &GetSuffix()
		{
			return resource_suffix;
		}

	    void SetSuffix(std::string suffix_)
		{
			resource_suffix = suffix_;
		}

		/*获取资源路径*/
		std::string &GetPath()
		{
			return path;
		}

		void SetPath(std::string &path_)
		{
			path = path_;
		}
		/*请求行解析*/
		void RequestLineParse()
		{
			std::stringstream ss(rq_line);
			ss >> method >> uri >> version;
		}

		/*URI解析*/
		void UriParse()
		{
			/*先判断方法*/
			if(strcasecmp(method.c_str(), "GET") == 0)
			{
			 	std::size_t pos_ = uri.find('?');
				if(std::string::npos != pos_)
				{

		 			/*有参数*/
					cgi = true;
					path +=uri.substr(0, pos_);
					param = uri.substr(pos_ + 1);
				}
				else
				{
		 			/*无参数*/
					path += uri;
				}

			}
			else
			{
				/*POST方法*/
				path += uri;
			}

			/*如果path最后一个字符是'/',说明它是默认的主页，应加上index.html*/
			if(path[path.size() - 1] == '/')
			{
				path += HOME_PAGE;		
			}
		}

		/*判断请求方法是否合法，这里只处理POST,GET两种方法*/
		bool IsMethodLegal()
		{
			if(strcasecmp(method.c_str(), "GET") == 0||\
					(cgi = (strcasecmp(method.c_str(), "POST") == 0)))
			{
				return true;

			}
			return false;
		}

		/*判断路径是否合法*/
		bool IsPathLegal()
		{
			/*有疑问*/
			struct stat st;
			if(stat(path.c_str(), &st) < 0)
			{	
				LOG(WARNING, "path is not found!");
				return  false;
			}
			if(S_ISDIR(st.st_mode))
			{
				path += "/";
				path += HOME_PAGE;
			}
			else
			{
 				if((st.st_mode & S_IXUSR) ||\
 						(st.st_mode & S_IXGRP) ||\
						(st.st_mode & S_IXOTH))
				{
					cgi = true;
				}
			}
			resource_size = st.st_size;
			std::size_t pos = path.rfind(".");
			if(std::string::npos != pos)
			{
				resource_suffix = path.substr(pos);
			}

			return true;
		}

		/*请求报头解析*/
		bool RequestHeadParse()
		{
			int start = 0;
			while(start < rq_head.size())
			{	
	 			std::size_t pos = rq_head.find('\n', start);
				if(std::string::npos == pos)
				{
					break;
				}
				std::string sub_string_ = rq_head.substr(start, pos - start);
				if(!sub_string_.empty())
				{


					LOG(INFO, "substr is not empty!");
					ProtocolUtil::MakeKV(head_kv, sub_string_);
				}
				else
				{
					LOG(INFO, "substr is empty!");
	 				break;
				}

				start = pos + 1;
			}
			return true;
		}

		/*判断是否需要读取正文*/
		bool IsNeedRecvText()
		{
			if(strcmp(method.c_str(), "POST") == 0)
			{
				return true;
			}
			return false;
		}
		bool IsCgi()
		{
			return cgi;
		}

		/*析构函数*/
		~Request()
		{}
};


class Response{

	public:
		std::string rsp_line;//状态行
		std::string rsp_head;//响应报头
		std::string blank;//空行
		std::string rsp_text;//响应正文
		int code;//状态码
		int fd;//描述符
	public:
		Response():blank("\n"), code(OK),fd(-1)
	{}
		void MakeStatusLine()
		{
			rsp_line = HTTP_VERSION;
			rsp_line += " ";
			rsp_line += ProtocolUtil::IntToString(code);
			rsp_line += " ";
			rsp_line += ProtocolUtil::CodeToDesc(code);
			rsp_line += "\n";
		}
		void MakeResponseHead(Request *&rq_)
		{
			rsp_head = "Content-Length: ";
			rsp_head += ProtocolUtil::IntToString(rq_->GetResourceSize());

			rsp_head += "\n";
			rsp_head += "Content-Type: ";
			/*先获得资源类型*/
			std::string suffix_ = rq_->GetSuffix();
			rsp_head += ProtocolUtil::SuffixToType(suffix_);
			rsp_head += "\n";
		}

		/*打开资源*/
		void OpenResource(Request *&rq_)
		{
			std::string path_ = rq_->GetPath();

			fd = open(path_.c_str(), O_RDONLY);
		}

		~Response()
		{
			if(fd >= 0)
				close(fd);
		}
};	

class Connect{

	private:
		int sock;//对应的socket所单独创建的连接的类
	public:
		Connect(int sock_):sock(sock_)
	{}

		/*读取请求行*/
		int RecvOneLine(std::string &line_)
		{
			char c = 'x';
			while(c != '\n')
			{
	 			/*每次读取一个字符*/
				ssize_t s = recv(sock, &c, 1, 0);

				/*这里分三种情况，请求行结尾分别比为\r,\n,\r\n;
				 * 我们都统一处理成\n的形式*/
				if(s > 0)
				{
 					if(c == '\r')
					{
 						/*MSG_PEEK会让recv从sock里面在读取一个字符,
						 * 用于判断下一个字符是否为\n,
						 * 但是并不会删除当前独到的这个字符*/
						recv(sock, &c, 1, MSG_PEEK);
						if(c == '\n')
						{
	 						/*如果当前字符是\n,说明是\r\n结尾,此时在读取掉\n*/
							recv(sock, &c, 1, 0);
						}
						else 
						{
	 						/*否则,说明以\r结尾,此时把c赋值给\n即可*/
							c = '\n';
						}
					}

					/*这里就说明,是普通字符或者\n,直接插入即可*/

					line_.push_back(c);
				}
				else
				{
					break;

				}
			}
			return line_.size();
		}

		/*获取请求报头*/
		void RecvRequestHead(std::string &head_)
		{
			head_ = "";
			std::string line_;
			while(line_ != "\n")
			{
	 			line_ = "";
				RecvOneLine(line_);
				head_ += line_;
			}
		}

		/*获取 正文*/
		void RecvRequestText(std::string &text_, int len_, std::string &param_)
		{
			char c_;
			int i_ = 0;
			while(i_ < len_)
			{
	 			recv(sock, &c_, 1, 0);
				text_.push_back(c_);
				i_++;
			}

			param_ = text_;
		}

		/*发送响应信息*/

		void SendResponse(Response *&rsp_, Request *&rq_, bool cgi_)
		{
			std::string &rsp_line_ = rsp_->rsp_line;
			std::string &rsp_head_ = rsp_->rsp_head;
			std::string &blank_ = rsp_->blank;	
			send(sock, rsp_line_.c_str(), rsp_line_.size(), 0);
			send(sock, rsp_head_.c_str(), rsp_head_.size(), 0);
			send(sock, blank_.c_str(), blank_.size(), 0);
			if(cgi_)
			{

				std::string &rsp_text_ = rsp_->rsp_text;
				send(sock, rsp_text_.c_str(), rsp_text_.size(), 0);
			}
			else
			{
				//std::cout<<"send"<<std::endl;
				int &fd = rsp_->fd;			
				sendfile(sock, fd, NULL, rq_->GetResourceSize());//有疑惑
			}
		}

		~Connect()
		{
			if(sock >= 0)
				close(sock);
		}
};

class Entry{

	public:
		static int PorcessResponse(Connect *&conn_, Request *&rq_, Response *&rsp_)
		{
			if(rq_->IsCgi())
			{
					ProcessCgi(conn_, rq_, rsp_);
			}
			else
			{
				ProcessNonCgi(conn_,rq_, rsp_);
			}
		}

		static void ProcessCgi(Connect *&conn_, Request *&rq_, Response *&rsp_)
		{
			int &code_ = rsp_->code;

			/*利用匿名管道进行父子进程通信*/
			int input[2];//相对于子进程输入
			int output[2];//相对于子进程输出
			std::string &param_ = rq_->GetParam();
			std::string &rsp_text_ = rsp_->rsp_text;

			pipe(input);
			pipe(output);

			pid_t id = fork();
			if(id < 0)
			{
 	 			code_ = NOT_FOUND;
				return ;
			}
			else if(id == 0)
			{
 	 			/*子进程*/
				close(input[1]);
				close(output[0]);

				const std::string &path_ = rq_->GetPath();

				std::string cl_env = "Content-Length=";
				cl_env += ProtocolUtil::IntToString(param_.size());

				putenv((char*)cl_env.c_str());

				/*将文件描述符input[0]里面的值拷贝一份出来，赋值给0*/
				dup2(input[0],0);
				dup2(output[1],1);
				execl(path_.c_str(), path_.c_str(), NULL);
				exit(1);
			}
			else
			{
	 			close(input[0]);
				close(output[1]);

				size_t size_ = param_.size();
				size_t total_ = 0;
				size_t curr_  = 0;				
				
				const char *p_ = param_.c_str();
				/*将参数输进管道*/
				while( total_ < size_ && \
						(curr_ = write(input[1], p_ + total_, size_ - total_)) > 0)
				{
					total_ += curr_;
				}

				char c;
				while(read(output[0], &c, 1) >0)
				{
					rsp_text_.push_back(c);	
				}
				/*等待子进程处理完，返回结果*/

				waitpid(id, NULL, 0);

				close(input[1]);
				close(output[0]);

				rsp_->MakeStatusLine();
				rq_->SetResourceSize(rsp_text_.size());

				rsp_->MakeResponseHead(rq_);
				conn_->SendResponse(rsp_, rq_, true);
			}
		}

		static void ProcessNonCgi(Connect *&conn_, Request *&rq_, Response *&rsp_)
		{
			//std::cout<<"ProcessNOncgi"<<std::endl;
			int &code_ = rsp_->code;
			rsp_->MakeStatusLine();
			rsp_->MakeResponseHead(rq_);
			rsp_->OpenResource(rq_);
			conn_->SendResponse(rsp_, rq_, false);
		}

		static void Process404(Connect *&conn_, Request *&rq_, Response *&rsp_)
		{
			//std::cout<<"process404"<<std::endl;
			std::string path_ = WEB_ROOT;
			path_ += "/";
			path_ += PAGE_404;

			/*将path下的文件信息存放在st结构体中*/
			struct stat st;
			stat(path_.c_str(), &st);
			
			rq_->SetResourceSize(st.st_size);
			rq_->SetSuffix(".html");
			rq_->SetPath(path_);

			ProcessNonCgi(conn_, rq_, rsp_);
		}

		static void HandlerError(Connect *&conn_, Request *&rq_, Response *&rsp_)
		{
			int &code_ = rsp_->code;
			switch(code_)
			{
 	 	 		case 400:
					break;
				case 404:
	 				Process404(conn_, rq_, rsp_);
					break;
				case 500:
					break;
				case 503:
					break;
				default:std::cout << "Unknown code";
					break;
			}
		}

		static int  HandlerRequest(int sock_)
		{

			/*用sock_来获取已经连接的socket*/

		
			Connect *conn_ = new Connect(sock_);
			Request *rq_  = new Request();
			Response *rsp_  = new Response();

			int &code_ = rsp_->code;//用来设置rsp的状态码

			/*获取请求行*/
			conn_->RecvOneLine(rq_->rq_line);

			/*请求行解析*/
			rq_->RequestLineParse();

			/*判断请求方法*/
			if(!rq_->IsMethodLegal())
			{
				/*如果方法不对,得先接受完请求,再处理响应*/
				conn_->RecvRequestHead(rq_->rq_head);
				code_ = BAD_REQUEST;
				goto end;
			}

			/*URI解析*/
			rq_->UriParse();

			/*判断路径是否合法*/
			if(!rq_->IsPathLegal())
			{
	 			code_ = NOT_FOUND;
				goto end;
			}

			LOG(INFO, "request path is ok!");

			/*获取请求报头*/
			conn_->RecvRequestHead(rq_->rq_head);

			/*请求报头解析*/
			if(rq_->RequestHeadParse())
			{
				LOG(INFO, "parse head done!");
			}
			else
			{
				code_ = BAD_REQUEST;
				goto end;

			}

			/*判断是否需要获取请求正文,如果是GET方法,则不需要读取正文*/
			if(rq_->IsNeedRecvText())
			{
			 	conn_->RecvRequestText(rq_->rq_text,rq_->GetContentLength(),\
						rq_->GetParam());		
			}

			/*构建响应*/
			PorcessResponse(conn_, rq_, rsp_);

end:
			if(code_ != OK)
			{
				HandlerError(conn_, rq_, rsp_);
			}
			delete conn_;
			delete rq_;
			delete rsp_;
			return code_;
		}
};

#endif











