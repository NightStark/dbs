#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int NS_Daemon( int nochdir, int noclose )
{
   pid_t pid;

   //if ( !nochdir && chdir("/") != 0 )	//如果nochdir=0,那么改变到"\"根目录
	 if ( !nochdir && chdir(".") != 0 )	//如果nochdir=0,那么改变到"."根目录 为了打开网页目录，网页目录可以设置为可配置
       return -1;
   
   if ( !noclose )//如果没有close
   {
     int fd = open("/dev/null", O_RDWR);//打开空洞文件.

     if ( fd < 0 )
      return -1;

	//对于每个进程,它的fds文件描述符表中:0,1和2文件句柄位置对应的fops文件操作函数集,
	//fdt->fd[0],fdt->fd[1],fdt->fd[2],
	//规定将分别与标准输入,标准输出,标准错误输出相关联.
	//所以用户应用程序调用open函数打开文件时,默认都是以3索引为开始句柄,故当前open返回的文件句柄最小值为3.
	//dup2(unsigned int oldfd, unsigned int newfd)系统调用就是用oldfd的fops操作文件集file,复制到newfd所在处
	//即：fdt->fd[newfd] = fdt->fd[oldfd];
     if (dup2( fd, 0 ) < 0 || //使用字符设备/dev/null的fops函数操作集,替换0句柄对应的文件操作集.
     	 dup2( fd, 1 ) < 0 || //使用字符设备/dev/null的fops函数操作集,替换1句柄对应的文件操作集.
         dup2( fd, 2 ) < 0 )  //使用字符设备/dev/null的fops函数操作集,替换2句柄对应的文件操作集.
     {
       close(fd);
      return -1;
     }
	//如果上面替换成功,那么键盘的任何操作将不会对该进程产生任何影响,因为0,1,2句柄所在处的fops文件操作集已经都变成了,
	//被重定向为"/dev/null"空洞设备的fops.所以对0,1,2句柄的读写操作,也就是在对/dev/null设备作读写操作.
     close(fd);				//关闭打开的/dev/null
  }
  
   pid = fork();			//创建子进程.
   if (pid < 0)
    return -1;

   if (pid > 0)
    _exit(0);				//返回执行的是父进程,那么父进程退出,让子进程变成真正的孤儿进程.

	//ok,我们期望的daemon子进程执行到这里了.
   if ( setsid() < 0 )		//设置session id.
     return -1;

   return 0;				//成功创建daemon子进程.
}

