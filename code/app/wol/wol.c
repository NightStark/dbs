#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>

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

static inline void *_init_buf(void**p, int iLen)
{
    if (*p == NULL) {
        *p = malloc(iLen);
    }

    return *p;
}

#define init_buf(b, l) \
    if (NULL == _init_buf((void **)&b, l)) { \
        return NULL; \
    }

unsigned char *
_str2mac (const char *str, unsigned char *buf)
{
	unsigned int mac[6];
	int num1=0, num2=0, i=0;
	init_buf(buf, 6);
	memset(buf, 0, 6);
	for(i=0; str[i]!='\0'; i++) {
		if(str[i] == ':')
			num1 ++;
		if(str[i] == '-')
			num2 ++;
	}
	if(num1 == 5)
		i = sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else if(num2 == 5)
		i = sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else
		return buf;

	if (i < 6)
		return NULL;

	*(uint8_t *)buf = mac[0];
	*(uint8_t *)(buf+1)  = mac[1];
	*(uint8_t *)(buf+2)  = mac[2];
	*(uint8_t *)(buf+3)  = mac[3];
	*(uint8_t *)(buf+4)  = mac[4];
	*(uint8_t *)(buf+5)  = mac[5];
	return buf;
}

int send_wol(unsigned char *mac)
{
    int fd = -1; //套接口描述字
    int i = 0;
    char buf[1024];
    char *p = NULL;
    struct sockaddr_in address;//处理网络通信的地址
    int ret = 0;
    int on=1;
    int data_len = 0;

    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    //address.sin_addr.s_addr=inet_addr("255.255.255.255");//这里不一样
    address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    address.sin_port = htons(7);

    //创建一个 UDP socket
    fd=socket(AF_INET, SOCK_DGRAM, 0);//IPV4  SOCK_DGRAM 数据报套接字（UDP协议）
    if (fd < 0) {
        printf("socket create failed.\n");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &on, sizeof(on)) < 0) {
        printf("socket opt set failed.\n");
        close(fd);
        fd = -1;
        return -1;
    }

    p = buf;
    memset(buf, 0xFF, 6);
    p += 6;
    for (i = 0; i < 16; i++, p += 6) {
        memcpy(p, mac, 6);
    }

    data_len = p - buf;

    __dump_data(buf, data_len, "wol pkt");

    ret = sendto(fd, buf, data_len, 0, (struct sockaddr *)&address, sizeof(address));
    perror("sendto");
    printf("date len %d send len:%d\n", data_len, ret);

    close(fd);
    fd = -1;

    return 0;
}

int main(int argc, char **argv)
{
    char c = 0;
    unsigned char dest_mac_str[32] = {0};
    unsigned char dest_mac[6] = {0};

    printf("start wol\n");

    while (1){
		c = getopt (argc, argv, "m:");
        printf("c = %d\n", c);
		if (c == 0xFF)
			break;
        switch (c){
            case 'm':
                snprintf((char *)dest_mac_str, sizeof(dest_mac_str), "%s", optarg);
                _str2mac((const char *)dest_mac_str, dest_mac);
                break;
            defaule:
                printf("unknow para.\n");
        }
    }

/*
    if (dest_mac[0] == 0) {
        printf("no mac \n");
        return -1;
    }
    */
    printf("dest mac:%s\n", dest_mac_str);

    send_wol(dest_mac);

    return 0;
}
