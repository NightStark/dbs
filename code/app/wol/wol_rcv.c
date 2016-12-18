/* 
 *  * File:   main.c
 *   * Author: tianshuai
 *    *
 *     * Created on 2011年11月29日, 下午10:34
 *      */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>

int port=7;

static void
__dump_data(unsigned char *ptr, int len, const char *info_fmt, ...)
{
    va_list ap;
    int i;
    printf("\n************************\n");
    printf("dump [");
    va_start(ap, info_fmt);
    vprintf(info_fmt, ap);
    va_end(ap);
    printf("]\n");

    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n************************\n");
}

int main(int argc, char** argv) {

    int sin_len;
    char message[256] = {0};

    int socket_descriptor;
    struct sockaddr_in sin;
    printf("Waiting for data form sender \n");

    bzero(&sin,sizeof(sin));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=htonl(INADDR_ANY);
    sin.sin_port=htons(port);
    sin_len=sizeof(sin);

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    bind(socket_descriptor,(struct sockaddr *)&sin,sizeof(sin));

    while(1)
    {
        recvfrom(socket_descriptor,message,sizeof(message),0,(struct sockaddr *)&sin,&sin_len);
        printf("Response from server:%s\n",message);
        __dump_data(message, sizeof(message), "recv msg");
        if(strncmp(message,"stop",4) == 0)//接受到的消息为 “stop”
        {

            printf("Sender has told me to end the connection\n");
            break;
        }
    }

    close(socket_descriptor);
    exit(0);

    return (EXIT_SUCCESS);
}
