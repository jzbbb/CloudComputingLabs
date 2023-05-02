#include <bits/stdc++.h> // C++标准库头文件
#include <stdio.h>       // 标准输入输出库头文件
#include <stdlib.h>      // 标准库头文件
#include <unistd.h>      // 提供对 POSIX 操作系统 API 的访问的头文件
#include <sys/socket.h>  // 套接字编程相关头文件
#include <sys/types.h>   // UNIX系统调用的基本数据类型头文件
#include <sys/epoll.h>   // epoll事件驱动库头文件
#include <arpa/inet.h>   // IP地址转换函数头文件
#include <sys/types.h>   // UNIX系统调用的基本数据类型头文件
#include <ctype.h>       // 字符处理函数头文件
#include <errno.h>       // 错误代码宏定义头文件
#include <fcntl.h>       // 文件控制函数头文件
#include <assert.h>      // 断言库头文件
#include <sched.h>       // 调度相关函数库头文件
#include <dirent.h>      // 目录操作库头文件
#include <getopt.h>      // 命令行参数解析库头文件
#include <stdbool.h>
#include <regex.h>
#include "threadpool.h"  // 自定义线程池头文件
#include "http.h"     // 自定义HTTP连接类头文件

using namespace std;

#define MAX_REQUESTS 1024 // 最大HTTP连接数
#define MAX_EVENT 1024    // 最大epoll事件数
#define ERROR_SIZE 256

static char errbuf[ERROR_SIZE] = {0};
static const char *ip_format = "^([0-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-4]).([0-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]).([0-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]).([1-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-5])$";

bool get_IP_legal(regex_t *ipreg, const char *ip){
	regmatch_t pmatch[1];
	const size_t nmatch = 1;
	int status = regexec(ipreg, ip, nmatch, pmatch, 0);
	if(status == 0) {
		//printf("Match Successful!\n");
        return true;
	}
	else if(status == REG_NOMATCH) {
		regerror(status, ipreg, errbuf, ERROR_SIZE);	
		//printf("%s\n", errbuf);
        return false;
		memset(errbuf, 0, ERROR_SIZE);
	}
}
// ip转换成int 进行非法判断
unsigned int Is_ip_legal(char *ip)
{   
    //编译正则
    regex_t ipreg1;
	int reg = regcomp(&ipreg1, ip_format, REG_EXTENDED);
	if(reg != 0) {
		regerror(reg, &ipreg1, errbuf, ERROR_SIZE);	
		printf("%s\n", errbuf);
		memset(errbuf, 0, ERROR_SIZE);
		return 0;
	}
    if(!get_IP_legal(&ipreg1 ,ip)){
        printf("Illegal IP!! Please Check!\n");
        return __INT32_MAX__;
    }
    unsigned int re = 0;
    unsigned char tmp = 0;
    //printf("%s\n", ip);
    //printf("%d\n", strlen(ip));
    while (1) {
        if (*ip != '\0' && *ip != '.') {
            tmp = tmp * 10 + *ip - '0';
        } else {
            re = (re << 8) + tmp;
            if (*ip == '\0')
                break;
            tmp = 0;
        }
        ip++;
    }
    return re;
}

extern void set_signal(int sig, void (*handler)(int));       // 声明信号处理函数
extern void add_event(int fd_epoll, int fd, bool one_short); // 添加监听文件到epoll对象
extern void delete_event(int fd_epoll, int fd_socket);       // 从epoll对象中删除监听文件
extern void mod_event(int fd_epoll, int fd_socket, int ev);  // 修改epoll对象中的监听事件

int main(int argc, char *argv[])
{
    // IP地址、端口号、线程数
    int ip, port, threads;
    if (argc != 7) // 命令行参数不为7则打印提示信息
    {
        cout << "please check your input, example:" << endl;
        cout << "./httpserver --ip 127.0.0.1 --port 8888 --threads 8" << endl;
        exit(1);
    }
    while (1) // 解析命令行参数
    {
        int index = 0;
        static struct option long_options[] = {// basic必须的三个参数
                                               {"ip", required_argument, 0, 0},
                                               {"port", required_argument, 0, 0},
                                               {"threads", required_argument, 0, 0}};
        int Is_Parsing_Complete = getopt_long(argc, argv, "", long_options, &index);
        if (Is_Parsing_Complete == -1) // 解析参数完毕
        {
            break;
        }
        switch (index) // 转换参数形式
        {
        case 0:
            ip = Is_ip_legal(optarg);     // 将ip地址转换为整数形式
            if (ip == __INT32_MAX__) // 非法地址退出
            {
                exit(1);
            }
            break;
        case 1:
            port = atoi(optarg); // 将端口号转换为整数形式
            break;
        case 2:
            threads = atoi(optarg); // 将线程数转换为整数形式
            break;
        default:
            cout << "Parameter Error!!!"<<endl;
            exit(1);
        }
    }

    // Initialize

    // 忽略SIGPIPE信号
    // 在 TCP 通信中，如果客户端断开连接后服务器还发送数据，就会产生 SIGPIPE 信号，如果不处理会导致进程退出，因此这里将其忽略。
    set_signal(SIGPIPE, SIG_IGN);

    // 初始化线程池
    ThreadPool<Http> *pool = NULL;
    try
    {
        pool = new ThreadPool<Http>(threads, 512);
        cout << "Thread pool created successfully" << endl;
    }
    catch (...)
    {
        cout << "Failed to create thread pool" << endl;
        exit(-1); // 创建失败退出
    }

    // 存储连接到服务器的客户端请求
    Http *Requests = new Http[MAX_REQUESTS];

    // 创建套接字
    int http_server = socket(PF_INET, SOCK_STREAM, 0);

    // 设置端口复用，让服务端在断开后能够立即重启
    int Port_reuse = 1;
    setsockopt(http_server, SOL_SOCKET, SO_REUSEADDR, &Port_reuse, sizeof(Port_reuse));

    // 绑定本地 IP 地址和端口号
    // 需要<arpa/inet.h>
    struct sockaddr_in addr_server;
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(port);
    addr_server.sin_addr.s_addr = htonl(ip);
    bind(http_server, (struct sockaddr *)&addr_server, sizeof(addr_server));

    // 监听端口
    listen(http_server, 5);

    // epoll监听
    epoll_event events[MAX_EVENT];
    // 创建epoll对象
    int file_epoll = epoll_create(64);

    // 添加监听文件
    add_event(file_epoll, http_server, false);
    Http::m_epollfd = file_epoll;

    // 事件监听循环
    while (true)
    {
        cout << "Waiting for events......" << endl;
        // 一直等待事件到来
        int num = epoll_wait(file_epoll, events, MAX_EVENT, -1);
        if ((num < 0) && (errno != EINTR))
        {
            cout << "Error" << endl;
            break;
        }
        cout << "Events coming......" << endl;

        for (int i = 0; i < num; i++)
        {
            int fd_event = events[i].data.fd;
            if (fd_event == http_server)
            {
                // 服务器监听套接字响应，有新的客户端连接
                cout << "New client request connection..." << endl;
                struct sockaddr_in addr_client;
                socklen_t len_addr = sizeof(addr_client);
                // 接收连接请求
                int fd_client = accept(http_server, (struct sockaddr *)&addr_client, &len_addr);

                if (Http::m_user_cnt >= MAX_REQUESTS)
                {
                    close(fd_client);
                    continue;
                }
                Requests[fd_client].init(fd_client, addr_client);
                pool->append(Requests + fd_client);
            }
            //  else if(events[i].events & EPOLLIN) {
            //     if(Requests[fd_event].read()) {

            //     } else {
            //         Requests[fd_event].close();
            //     }

            // } else if(events[i].events & EPOLLOUT) {
            //     //这里的信号是什么时候才会被接收到呢？？？
            //     if(!Requests[fd_event].write()) {
            //         Requests[fd_event].close();
            //     }

            // }
        }
    }

    close(file_epoll);  // 关闭 epoll 实例监听的文件描述符
    close(http_server); // 关闭服务器套接字
    delete[] Requests;  // 释放 Requests 数组占用的内存
    delete pool;        // 释放线程池占用的内存

    return 0;
}
