#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>

#include "../include/symbol.h"
#include "../include/type.h"
#include "../include/debug.h"
#include "../include/string.h"
#include "../../include/list.h"
#include "../include/opdata.h"
#include "../include/msg.h"
#include "../include/msg_epoll.h"


int main(void)
{
    int fd;
    int connfd;
	struct sockaddr_in servaddr;
   	UCHAR ucMsgBuf[1024];
	ULONG ulRet = 0;	
	ULONG i = 0;
	
    fd =socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd)
    {
    	ERR_PRINTF("socet create failed!");
        return 0;
    }

	
	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(NSDB_MSG_LISTEN_ADDR);
	servaddr.sin_port = htons(NSDB_MSG_LISTEN_PORT);
    connfd = connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));
	if(-1 == connfd)
	{
		ERR_PRINTF("connetc failed!");
		return 0;
	}

	

	memset(ucMsgBuf, 0, 1024);

	strcpy(ucMsgBuf, "This is a test! 0");
	while(1)
	{
		sleep(1);

		memset(ucMsgBuf, 0 ,1024);
		
		sprintf(ucMsgBuf, "This is Test : %lu", i++);
		
		ulRet = send(fd, ucMsgBuf, strlen(ucMsgBuf), 0);
		if(-1 == ulRet)
		{
			ERR_PRINTF("sent error!");
		}
		ERR_PRINTF("msg len %d\n", ulRet);
		ERR_PRINTF("MSG : %s", ucMsgBuf);
	}

	close(connfd);
	close(fd);

    return 0;
}

