#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define ERR_PRINT(fmt, ...) \
	printf("[%s][%d]", __func__, __LINE__); \
    printf(fmt, ##__VA_ARGS__) 
#define MSG_PRINT(fmt, ...) \
	printf("[%s][%d]", __func__, __LINE__); \
    printf(fmt, ##__VA_ARGS__) 

#define _PK(cond) \
	({if ((cond)){        \
	 ERR_PRINT("ERROR:parameter is invalid, ["#cond"]\n"); \
	 };(cond);})

#define HTTP_RESP_SUCCESS           (0)                   
#define HTTP_RESP_ERR               (-0xFF)               
#define HTTP_RESP_ERR_PARA_ERROR    (HTTP_RESP_ERR - 1)   
#define HTTP_RESP_ERR_RECV_TIMEOUT  (HTTP_RESP_ERR - 2)   
#define HTTP_RESP_ERR_STATUS_ERR    (HTTP_RESP_ERR - 3)   
#define HTTP_RESP_ERR_GTE_STATUS_ERR  (HTTP_RESP_ERR - 4) 
#define HTTP_RESP_ERR_GTE_SERIAL_ERR  (HTTP_RESP_ERR - 5) 
#define HTTP_RESP_ERR_XML_INVALID     (HTTP_RESP_ERR - 6) 
#define HTTP_RESP_ERR_GET_RESULT_ERR  (HTTP_RESP_ERR - 7) 
#define HTTP_RESP_ERR_GET_ERRELE_ERR  (HTTP_RESP_ERR - 8) 
#define HTTP_CONN_ERR               (-0xFFF)              
#define HTTP_CONN_ERR_TIMEROUT      (HTTP_CONN_ERR - 1);  

#define HTTP_RECV_BUF_SIZE (1024)

struct http_response {
	struct http_header *header;
	int status;
	int content_length;
	int total_length;
	char *req;
	char *data;        /* buffer : the original data receive */
	int  data_buf_len; /* the length of data*/
	char *content;     /* http real content , point to data element */
	int  sockfd;
	int  serial;
	int  index;
	int  log_id;
};

struct http_header {
    char *line;
    struct http_header *next;
};

static int kconn(void)
{
    int fd = -1, err = -1;
    int opt = 1;
	int ret = 0;
	int max_fds = 0;
	fd_set skfd_set_r, skfd_set_w;
    struct timeval tmo = {0};

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.66.57");//htonl(0x7F000001),
    addr.sin_port = htons(80); 

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERR_PRINT("Unable to create socket - %d, %s\n", errno, strerror(errno));
		goto err_c;
    }
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0){
		ERR_PRINT("set non block failed.\n");
		goto err_c;
	}
	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        ERR_PRINT("Unable to connect socket - %d, %s\n", errno, strerror(errno));
        close(fd);
		goto err_c;
    }
#ifdef SO_NOSIGPIPE 
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){   
        ERR_PRINT("setsockopt SO_REUSEADDR fail.\n");
		goto err_c;
    }   

	FD_ZERO(&skfd_set_r);	
	FD_ZERO(&skfd_set_w);	
	FD_SET(fd, &skfd_set_r);
	FD_SET(fd, &skfd_set_w);
	max_fds = fd + 1;
    tmo.tv_sec = 1;
    tmo.tv_usec = 0;
	ret = select(max_fds, &skfd_set_r, &skfd_set_w, NULL, &tmo);
	if (ret < 0){
        ERR_PRINT("select error.\n");
		goto err_c;
	} else if (ret == 0)
	{
		MSG_PRINT("Select timeout.\n" );
		err = -1;
		goto err_c;
	} else {
		MSG_PRINT("connect success.\n" );
	}

    return fd;
err_c:
	close(fd);
	return err;
}

static int ksend(int skfd, const char *buf, int len)
{
    int n = 0;

    int s = 0;
	if (_PK(!buf) || _PK(skfd < 0)){
		return -1;
	}

	MSG_PRINT("buf:%s\n", buf);
    while(s < len){
        n = send(skfd, buf + s, len - s, 0);
        if(n <= 0){
            ERR_PRINT("send error, - %d, %s\n", errno, strerror(errno));
            return -1;
        }
        s += n;
    }
    return 0;
}

static int krecv(int skfd, char *buf, int maxlen)
{
    int n = 0;

	if (_PK(skfd < 0) || _PK(!buf)){
		return -1;
	}

    n = recv(skfd, buf, maxlen, 0);
    if(n < 0){
		ERR_PRINT("errno = %d\n", errno);
        return -1;
    }
    return n;
}

static int kprase_http_status_code(struct http_response *resp, char *start, char *end)
{
    char *s, *e;
    s = strchr(start, ' ');
    if(!s || s > end)
        return -1;
    while(*s == ' ' && s < end)
        s ++;
    e = s + 1;
    while(*e != ' ' && e < end)
        e ++;
    if(e == end || *e != ' ')
        return -1;
    *e = 0;
    resp->status = atoi(s);
    return 0;
}

static int kprase_http_response(struct http_response *resp)
{
    char *p = NULL;
    char *s = NULL;
    int end = 0;
    struct http_header *header = NULL;

    if(_PK(!resp))
        return -1;
    if(_PK(!resp->data))
        return -1;

    //For BHU device there is no keep-alive support. Server will close connection after send response
    resp->status = -1;
    s = resp->data;
    do{
        if(!(p = strstr(s, "\r\n"))){
            ERR_PRINT("invalide http header, no header end flag found\n");
            return -1;
        }
        *p = 0;
        if(memcmp(p + 2, "\r\n", 2) == 0){
            resp->content = p + 4;
			/* content is string so .. +1 as '\0' */
            resp->content_length = resp->total_length - (resp->content - resp->data) + 1;
			MSG_PRINT("content_len = %d, total_length = [%d] \n", resp->content_length, resp->total_length);
            end = 1;
        }
        //handle current line
        if(resp->status == -1){
            if(memcmp(s, "HTTP/", 5) == 0){
                if(kprase_http_status_code(resp, s, p))
                    return -1;
                goto next_loop;
            }
        }
        if(!(header = (struct http_header*)malloc(sizeof(*header)))){
            ERR_PRINT("oom\n");
            return -1;
        }
		memset(header, 0,sizeof(struct http_header));
        header->line = s;
        header->next = resp->header;
        resp->header = header;
next_loop:
        s = p + 2;
    }while(!end);
    return 0;
}

int kloop_recv(int tm, struct http_response *resp)
{
	int n = 0;
	int r = 0;
	char *tmp  = NULL;
	int   err  = HTTP_RESP_SUCCESS;
	int   size = HTTP_RECV_BUF_SIZE;
	fd_set fds_read, fds_write;
	int sret  = 0;
	int sdmax = 0;
	struct timeval rtmo = {0};

    do{
		FD_ZERO(&fds_read);
		FD_ZERO(&fds_write);
		FD_SET(resp->sockfd, &fds_read);
		sdmax = resp->sockfd;
		rtmo.tv_sec = tm;
		rtmo.tv_usec = 0;

		MSG_PRINT("---------\n");
		sret = select(sdmax + 1, &fds_read, &fds_write, NULL, &rtmo);
		if (sret == 0){
			err = HTTP_RESP_ERR_RECV_TIMEOUT;
			ERR_PRINT("kochab receive timer out.\n");
			/* 直接返回，socket不在这里关闭 */
			return err;
			//goto err1;
		}

		MSG_PRINT("---------%p, %p\n", resp, resp->data);
        if(size - r < HTTP_RECV_BUF_SIZE){
            size *= 2;
            if(!(tmp = (char*)realloc(resp->data, size))){
                ERR_PRINT("oom\n");
				err =  HTTP_RESP_ERR;
                goto err1;
            }
			//memset(resp->data + size/2, 0, size/2);
            resp->data = tmp;
        }

		MSG_PRINT("---------%p, %p\n", resp, resp->data);
        n = krecv(resp->sockfd, resp->data + r, size - r - 1);
		MSG_PRINT("----------------\n");
		MSG_PRINT("[[[[%s]]]]\n", resp->data);
		MSG_PRINT("errno = %d\n", errno);
        if(n < 0){
			if (errno == EAGAIN){
				r += n;
				continue;
			}
			err = HTTP_RESP_ERR;
            goto err1;
		}
        r += n;
		
		{ /* if success just exit loop, do not wait the server close the tcp link */
			resp->data[r] = 0;
			resp->total_length = r;
			if(kprase_http_response(resp)){
				ERR_PRINT("http response prase error\n");
				err =  HTTP_RESP_ERR;
				goto err1;
			}

			if (resp->status == 200 || resp->status == 302) {
				return 0;
			}
		}

		//break;
    }while(n > 0);

    resp->data[r] = 0;
    resp->total_length = r;
    if(kprase_http_response(resp)){
        ERR_PRINT("http response prase error\n");
		err =  HTTP_RESP_ERR;
        goto err1;
    }

    if(resp->status < 200 || resp->status > 299){
        ERR_PRINT("http status code %d\n", resp->status);
		err =  HTTP_RESP_ERR_STATUS_ERR;
        goto err1;
    }
	return 0;

err1:
	return err;
}

const char *http_req = 
"GET /files/hello.html HTTP/1.1\r\n"
"Host: 192.168.66.57\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: max-age=0\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
"Upgrade-Insecure-Requests: 1\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.80 Safari/537.36\r\n"
"Referer: http://192.168.66.57/files/\r\n"
"Accept-Encoding: gzip, deflate, sdch\r\n"
"Accept-Language: zh-CN,zh;q=0.8\r\n"
"If-None-Match: \"140-53669bb74dce3\"\r\n"
"If-Modified-Since: Wed, 29 Jun 2016 12:17:08 GMT\r\n\r\n";

int crate_sock_by_url(const char *url) 
{
	int skfd = -1;
	int ret = 0;

	struct http_response resp;

	memset(&resp, 0, sizeof(resp));
	
	skfd = kconn();
		
	if (skfd < 0) {
		ERR_PRINT("kconn failed.\n");
		return -1;
	}

	MSG_PRINT("kconn success.\n");

	ret = ksend(skfd, http_req, strlen(http_req));
	if (ret < 0) {
		ERR_PRINT("ksend failed.\n");
		return -1;
	}

	MSG_PRINT("send success.\n");

	resp.sockfd = skfd;
	resp.data = malloc(HTTP_RECV_BUF_SIZE);

	MSG_PRINT("---------\n");
	kloop_recv(100, &resp);

	close(skfd);
	skfd = -1;

	return 0;
}

int main(void)
{
	/*
	char buf[128] = {0};
	snprintf(buf, sizeof(buf),"cat maclist  | awk '{printf(\"%%s\\n\", $0)}'");
	printf(buf);
	*/
	while(1) {
		crate_sock_by_url("x");
		usleep(100000);
	}
	return 0;
}
