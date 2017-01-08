#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <memory.h>
#include <unistd.h>
//#include <linux/in.h>
#include <netinet/in.h>
//#include <linux/inet_diag.h>
#include <arpa/inet.h>
#include <signal.h>

/**
  关于 sockaddr  sockaddr_in  socketaddr_un说明
http://maomaozaoyue.blog.sohu.com/197538359.html
*/

#define PORT    11910   //定义通信端口
#define BACKLOG 5       //定义侦听队列长度
#define buflen  1024

void process_conn_server(int s);
void sig_pipe(int signo);

int ss,sc;  //ss为服务器socket描述符，sc为某一客户端通信socket描述符

int main(int argc,char *argv[])
{

	int ReUseAddr = 1;
    struct sockaddr_in server_addr; //存储服务器端socket地址结构
    struct sockaddr_in client_addr; //存储客户端 socket地址结构

    int err;    //返回值
    pid_t pid;  //分叉进行的ID

    /*****************socket()***************/
    ss = socket(AF_INET,SOCK_STREAM,0); //建立一个序列化的，可靠的，双向连接的的字节流
    if(ss<0)
    {
        printf("server : server socket create error\n");
        return -1;
    }
    //注册信号
    if(SIG_ERR == signal(SIGTSTP,sig_pipe))
    {
        printf("信号挂接失败\n");
        return -1;
    }
    else
        printf("信号挂接成功\n");

    err = setsockopt(ss, 
            SOL_SOCKET, 
            SO_REUSEADDR, 
            &ReUseAddr, 
            sizeof(ReUseAddr));    

    /******************bind()****************/
    //初始化地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;           //协议族
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //本地地址
    server_addr.sin_port = htons(PORT);

    err = bind(ss,(struct sockaddr *)&server_addr,sizeof(struct sockaddr));
    if(err<0)
    {
        printf("server : bind error\n");
        return -1;
    }

    /*****************listen()***************/
    err = listen(ss,BACKLOG);   //设置监听的队列大小
    if(err < 0)
    {
        printf("server : listen error\n");
        return -1;
    }

    /****************accept()***************/
    /**
      为类方便处理，我们使用两个进程分别管理两个处理：
      1，服务器监听新的连接请求;2,以建立连接的C/S实现通信
      这两个任务分别放在两个进程中处理，为了防止失误操作
      在一个进程中关闭 侦听套接字描述符 另一进程中关闭
      客户端连接套接字描述符。注只有当所有套接字全都关闭时
      当前连接才能关闭，fork调用的时候父进程与子进程有相同的
      套接字，总共两套，两套都关闭掉才能关闭这个套接字
      */

    for(;;)
    {
        socklen_t addrlen = sizeof(client_addr);
        //accept返回客户端套接字描述符
        sc = accept(ss,(struct sockaddr *)&client_addr,&addrlen);  //注，此处为了获取返回值使用 指针做参数
        if(sc < 0)  //出错
        {
            continue;   //结束此次循环
        }
        else
        {
            printf("server : connected\n");
        }

        process_conn_server(sc);
    }
}

/**
  服务器对客户端连接处理过程；先读取从客户端发送来的数据，
  然后将接收到的数据的字节的个数发送到客户端
  */

//通过套接字 s 与客户端进行通信
void process_conn_server(int s)
{
    ssize_t size = 0;
    char buffer[buflen];  //定义数据缓冲区
    for(;;)
    {
        //等待读
        size = 0;
        size = read(s,buffer,buflen);
        if (size <= 0) {
            close(s);
            s = -1;
            printf("%s", "connect break.\n");
            break;
        }

        //输出从客户端接收到的数据
        printf("%s",buffer);

        //结束处理
        if(strcmp(buffer,"quit") == 0)
        {
            close(s);   //成功返回0，失败返回-1
            return ;
        }
        sprintf(buffer,"%d bytes altogether\n",size);
        write(s,buffer,strlen(buffer)+1);
    }

    return;
}
void sig_pipe(int signo)
{
    printf("catch a signal\n");
    if(signo == SIGTSTP)
    {
        printf("接收到 SIGTSTP 信号\n");
        int ret1 = close(ss);
        int ret2 = close(sc);
        int ret = ret1>ret2?ret1:ret2;
        if(ret == 0)
            printf("成功 : 关闭套接字\n");
        else if(ret ==-1 )
            printf("失败 : 未关闭套接字\n");

        exit(1);
    }
}

