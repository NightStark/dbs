#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <ifaddrs.h>
#include <syslog.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "typedef.h"
#include "dmdb.h"
#ifdef __cplusplus
}

#endif
#include "hk.h"
#include "kmsg.h"
//#include "libcgi/cgi-lib.h"
//#include "libcgi/html-lib.h"

#define CGI_SID_SIZE        32
#define HTTP_HEADER_COOKIE  "Set-Cookie"
#define SOCK_IO_TIMEOUT     3 


struct http_header {
    char *line;
    struct http_header *next;
};

static const char *Post_hdr =
            "POST /cgi-bin/cgiSrv.cgi HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Cookie: sid=%s\r\n"
            "Accept-Encoding: identity\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: text/xml\r\n"
            "\r\n";

static const char *Get_hdr =
            "GET /cgi-bin/cgiSrv.cgi?%s HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Cookie: sid=%s\r\n"
            "Accept-Encoding: identity\r\n"
//            "Content-Length: %d\r\n"
//            "Content-Type: text/xml\r\n"
            "\r\n";

static const char *Op_hdr =
            "POST /cgi-bin/cgiSrv.cgi HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Cookie: sid=%s\r\n"
            "Accept-Encoding: identity\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
            "\r\n";

static const char *Cmd_hdr =
            "POST /cgi-bin/cgiSrv.cgi HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Cookie: sid=%s\r\n"
            "Accept-Encoding: identity\r\n"
            "Content-Length: %d\r\n"
//			"Connection: keep-alive\r\n"
//			"Accept: application/xml, text/xml, */*;\r\n"
//			"X-Requested-With: XMLHttpRequest\r\n"
//			"Accept: application/xml, text/xml, */*;\r\n"
           "Content-Type: text/xml\r\n"
            "\r\n";

#define HTTP_KOCHAB_USERNAME_BUF_SZIE (32)
#define HTTP_KOCHAB_PASSWORD_BUF_SZIE (32)
#define HTTP_KOCHAB_OP_BUF_SZIE		  (256)
#define HTTP_KOCHAB_USERNAME		  "admin"
#define HTTP_KOCHAB_PASSWORD		  "admin"
#define HTTP_KOCHAB_ADDRESS			  "127.0.0.1"

#define HTTP_RECV_BUF_SIZE (1024)

struct login_info {
	char sid[CGI_SID_SIZE];
	char user[HTTP_KOCHAB_USERNAME_BUF_SZIE + 1];
	char pwd[HTTP_KOCHAB_PASSWORD_BUF_SZIE + 1];
	char login_op[HTTP_KOCHAB_OP_BUF_SZIE + 1];
	int  islogin;
	int  islocked;
	pthread_mutex_t loginfo_mutex;
};

struct login_info *g_login_info = NULL;

static int kconn(void);
int klogin(struct http_response *resp ,struct login_info *login);

static void kurl_encode(const char *src, char *dst, size_t dst_len) {
    static const char *dont_escape = "._-$,;~()";
    static const char *hex = "0123456789abcdef";
    const char *end = dst + dst_len - 1;

    for (; *src != '\0' && dst < end; src++, dst++) {
        if (isalnum(*(const unsigned char *) src) ||
                strchr(dont_escape, * (const unsigned char *) src) != NULL) {
            *dst = *src;
        } else if (dst + 2 < end) {
            dst[0] = '%';
            dst[1] = hex[(* (const unsigned char *) src) >> 4];
            dst[2] = hex[(* (const unsigned char *) src) & 0xf];
            dst += 2;
        }
    }

    *dst = '\0';
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

static char *kget_http_header(struct http_response *resp, const char *header)
{
    struct http_header *h;
    int len;
    h = resp->header;
    len = strlen(header);
    while(h){
        if(memcmp(h->line, header, len) == 0 && h->line[len] == ':')
            return h->line;
        h = h->next;
    }
    return NULL;
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
			DBG_PRINT("content_len = %d, total_length = [%d] \n", resp->content_length, resp->total_length);
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


static int krecv(int skfd, char *buf, int maxlen)
{
    int n = 0;

	if (_PK(skfd < 0) || _PK(!buf)){
		return -1;
	}

    n = recv(skfd, buf, maxlen, 0);
    if(n < 0){
		ERR_PRINT("errno = %d\n", errno);
        ERR_PRINT("oom\n");
        return -1;
    }
    return n;
}

static int ksend(int skfd, const char *buf, int len)
{
    int n = 0;

    int s = 0;
	if (_PK(!buf) || _PK(skfd < 0)){
		return -1;
	}

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

static int ksock_timeout(int sockfd, int tmout)
{
    struct timeval tmo = {0};

    tmo.tv_sec = tmout;
    tmo.tv_usec = 0;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tmo, sizeof(tmo)) \
            || -1 == setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tmo, sizeof(tmo))){   
        ERR_PRINT("setsockopt error.\n");
        return -1; 
    }   

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

		sret = select(sdmax + 1, &fds_read, &fds_write, NULL, &rtmo);
		if (sret == 0){
			err = HTTP_RESP_ERR_RECV_TIMEOUT;
			ERR_PRINT("kochab receive timer out.\n");
			/* 直接返回，socket不在这里关闭 */
			return err;
			//goto err1;
		}

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

		///MSG_PRINT("---------%p, %p\n", resp, resp->data);
        n = krecv(resp->sockfd, resp->data + r, size - r - 1);
		//MSG_PRINT("----------------\n");
		//MSG_PRINT("[[[[%s]]]]\n", resp->data);
		//MSG_PRINT("errno = %d\n", errno);
        if(n < 0){
			if (errno == EAGAIN){
				r += n;
				continue;
			}
			err = HTTP_RESP_ERR;
            goto err1;
		}
        r += n;
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
static int khttp_talk(char *buf, int len, int tm, struct http_response *resp)
{
	int   err  = HTTP_RESP_SUCCESS;

	if (_PK(!buf) || _PK(len <= 0) || _PK(!resp)){
		MSG_PRINT("parameters error.\n");	
		return HTTP_RESP_ERR_PARA_ERROR;
	}

    if((resp->sockfd = kconn()) < 0){
        ERR_PRINT("connect to server failed\n");
        return HTTP_RESP_ERR;
    }
    ksock_timeout(resp->sockfd, tm);
	//MSG_PRINT("sending:[\n%s\n]\n", buf);
    if(ksend(resp->sockfd, buf, len) < 0){
        err = HTTP_RESP_ERR;
		goto err1;
	}

	err = kloop_recv(tm, resp);
    //return err;

err1:
	/* time out do not close it is not good, but we will call this func again until not time out */
    if(resp->sockfd != -1 && err != HTTP_RESP_ERR_RECV_TIMEOUT){
        close(resp->sockfd);
        resp->sockfd = -1;
    }
    return err;
}

char * kget_sid(const char *cookie, char *sid)
{
    int i;
    if (_PK(cookie == NULL) || _PK(sid == NULL))
        return NULL;
    char *sid1 = strstr((const char *)cookie, (char *)"sid=");
    if (sid1 == NULL)
        return NULL;
    sid1 += 4;/* "sid=" size */
    for (i=0; i<CGI_SID_SIZE; i++) {
        if (sid1[i] == ';' || sid1[i] == 0)
            break;
    }   
    if (i >= CGI_SID_SIZE)
        return NULL;
    sid[i] = 0;
    strncpy(sid, sid1, i); 
    return sid;
}

/* return : socket fd */
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
    addr.sin_addr.s_addr = inet_addr(HTTP_KOCHAB_ADDRESS);//htonl(0x7F000001),
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
		err = HTTP_CONN_ERR_TIMEROUT;
		goto err_c;
	} else {
		MSG_PRINT("connect success.\n" );
	}

    return fd;
err_c:
	close(fd);
	return err;
}

//创建http_response结构
struct http_response * kcreate_http_response(void)
{
	struct http_response *resp = NULL;

    if(!(resp = (struct http_response*)malloc(sizeof(*resp)))){
        ERR_PRINT("oom\n");
        goto fail;
    }
    memset(resp, 0, sizeof(*resp));
    if(!(resp->data = (char*)malloc(HTTP_RECV_BUF_SIZE))){
        ERR_PRINT("oom\n");
        goto fail;
    }
	memset(resp->data, 0, HTTP_RECV_BUF_SIZE);
	resp->data_buf_len = HTTP_RECV_BUF_SIZE;
	resp->sockfd = -1;

	return resp;
fail:
	if (resp) {
		free(resp);
		resp = NULL;
	}
	return NULL;
}

//释放http_response结构体
void kfree_http_response(struct http_response *resp)
{
    struct http_header *tmp;
    struct http_header *p;

    if(resp){
        if(resp->data) {
            free(resp->data);
			resp->data = NULL;
		}
        p = resp->header;
        while(p){
            tmp = p->next;
            free(p);
            p = tmp;
        }

		/* make sure resp->sockfd is < 0 close before here. */
		if (resp->sockfd > -1) {
			close(resp->sockfd);
			resp->sockfd = -1;
		}
        free(resp);
		resp = NULL;
    }

	return;
}

static void kinit_http_response(struct http_response *resp)
{
	char *t  = NULL;

	t = resp->data;
	if (t){
		memset(t, 0, resp->data_buf_len);
	}
	memset(resp, 0, sizeof(struct http_response));
	/* keep malloc point  */
	resp->data = t;

	return;
}

int kget_path(const char *path, struct http_response *resp, const char *sid)
{
	int ret = 0;
    char buf[1024] = {0};
    char args[256] = {0};
	if (_PK(!path) || _PK(!resp)){
		return -1;	
	}

    kurl_encode(path, buf, sizeof(buf));
    snprintf(args, sizeof(args), "xml=[%s]", buf);
    snprintf(buf, sizeof(buf), Get_hdr, args, sid);
    strcat(buf, args);

    if((ret = khttp_talk(buf, strlen(buf), SOCK_IO_TIMEOUT, resp)) < 0){
        ERR_PRINT("http talk fail\n");
        return ret;
    }

    return 0;
}

int kxml_apply(const char *xml, struct http_response *resp, const char *sid)
{
	int ret = 0;
    int len;
    int n = 0;
    char buf[1024*10] = {0};
    
    if(_PK(!xml) || _PK(!resp) || _PK(!sid))
        return -1;
    len = strlen(xml);
    n = snprintf(buf, sizeof(buf), Post_hdr, sid, len);
    if((int)n + len > (int)sizeof(buf)){
        ERR_PRINT("size too large, not enough buffer");
        return -1;
    }
    strcat(buf, xml);

    if((ret = khttp_talk(buf, strlen(buf), SOCK_IO_TIMEOUT, resp)) < 0){
		if (ret == HTTP_RESP_ERR_RECV_TIMEOUT) {
			DBG_PRINT("timer out will try again.\n");
		} else {
			ERR_PRINT("http talk fail\n");
		}
        return ret;
    }

    return 0;
}

int kop(const char *opstr, struct http_response *resp, const char *sid)
{
	int ret = 0;
    int n;
    int len;
    char buf[1024] = {0};

    if(_PK(!opstr) || _PK(!resp) || _PK(!sid))
        return -1;
    len = strlen(opstr);
    n = snprintf(buf, sizeof(buf), Op_hdr, sid, len);
    if((int) n + len > (int)sizeof(buf)){
        ERR_PRINT("size too large, not enough buffer");
        return -1;
    }
    strcat(buf, opstr);

    if((ret = khttp_talk(buf, strlen(buf), SOCK_IO_TIMEOUT, resp)) < 0){
        ERR_PRINT("http talk fail\n");
        return ret;
    }

    return 0;
}

int kcmd(const char *cmdstr, struct http_response *resp, const char *sid)
{
	int ret = 0;
    int len;
    int n;
    char buf[1024] = {0};

    if(_PK(!cmdstr) || _PK(!resp) || _PK(!sid)){
        return -1;
	}
    len = strlen(cmdstr);
    n = snprintf(buf, sizeof(buf), Cmd_hdr, sid, len);
    if((int) n + len > (int)sizeof(buf)){
        ERR_PRINT("size too large, not enough buffer");
        return -1;
    }
    strcat(buf, cmdstr);

    if((ret = khttp_talk(buf, strlen(buf), SOCK_IO_TIMEOUT, resp)) < 0){
        ERR_PRINT("http talk fail\n");
        return ret;
    }

    return 0;
}

#define KLOGIN_TH_KEEP     (0)
#define KLOGGIN_TH_WAKEUP  (1)
int wake_up = KLOGIN_TH_KEEP;
pthread_mutex_t klogin_keep = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  klogin_cont = PTHREAD_COND_INITIALIZER;
static inline int get_login_info(int log_id, struct login_info *loginfo, int len);
static inline int set_login_info(int log_id, const struct login_info *loginfo, int len);

void *klogin_th(void *arg)
{
	int  ret = 0, log_id = 0;
	struct http_response *resp   = NULL;
	struct login_info tmp_login;
	struct timeval  now;
	struct timespec outtime;
	char xmlBuf[32] = {0};

	MSG_PRINT("thread %s is start\n" , __func__);

	signal(SIGPIPE,SIG_IGN); 
	prctl(PR_SET_NAME, (unsigned long)"login_keep");

	snprintf(xmlBuf, sizeof(xmlBuf), "%s", "dev.sys.time");

	resp = kcreate_http_response();
	if (!resp){
		goto err;	
	}

	while(1){
		for ( log_id= 0;  log_id < HTTP_KOCHAB_LOGIN_MAX; log_id++){
			memset(&tmp_login, 0, sizeof(tmp_login));
			if (get_login_info(log_id, &tmp_login, sizeof(struct login_info)) < 0){
				goto err;	
			}

			if (tmp_login.islogin == HTTP_KOCHAB_NO_LOGIN){
				continue;
			}

			MSG_PRINT("user = %s, pwd = %s, sid = %s, login again to keep loging.\n", tmp_login.user, tmp_login.pwd, tmp_login.sid);

			kinit_http_response(resp);
			/*
			if(klogin(resp, &tmp_login) < 0){
				tmp_login.islogin = HTTP_KOCHAB_NO_LOGIN;
				ERR_PRINT("login fail\n");
				goto err;	
			}
			*/
			/* use get a xml to flush session timeer */
			resp->req = xmlBuf;
			if (kmain(resp, MT_QUERY) != 0) {
				ERR_PRINT("flush session timer failed.");
				goto err;
			}

			DBG_PRINT("flush login session %s", resp->content);

			tmp_login.islogin = HTTP_KOCHAB_IS_LOGIN;
			/* update login infomaton */
			if (set_login_info(log_id, &tmp_login, sizeof(struct login_info)) < 0){
				goto err;	
			}

		}
		/* wait 5 minutes can be wakeup by logout */
		gettimeofday(&now, NULL);
		outtime.tv_sec  = now.tv_sec + HTTP_KOCHAB_SLEEP_5MINS;
		outtime.tv_nsec = now.tv_usec * 1000;
		pthread_mutex_lock(&klogin_keep);
		ret = pthread_cond_timedwait(&klogin_cont, &klogin_keep, &outtime);
		if (errno > 132) {
			/* unkonow err just continue and try */
			pthread_mutex_unlock(&klogin_keep);
			continue;
		}
		if (ret != 0 && errno != ETIMEDOUT && errno != 0 && errno != EINTR){
			ERR_PRINT("wait errno = %d.\n", errno);
		}
		if (wake_up == KLOGGIN_TH_WAKEUP){
			MSG_PRINT("stop keep loop and thread will exit.\n");
			pthread_mutex_unlock(&klogin_keep);
			break;
		}
		pthread_mutex_unlock(&klogin_keep);
		MSG_PRINT("try login and keep.\n");
	}

	kfree_http_response(resp);
	resp = NULL;
	return (void *)0;
err:
	kfree_http_response(resp);
	resp = NULL;
	return (void *)-1;
}

int login_init(void)
{
	int len = 0, i = 0;
	pthread_t lg_tid;

	len = sizeof(struct login_info) * HTTP_KOCHAB_LOGIN_MAX;
	g_login_info = (struct login_info *) malloc(len);
	if (NULL == g_login_info){
		ERR_PRINT("malloc failed.\n");
		return -1;
	}
	memset(g_login_info, 0, len);
	for(i = 0; i < HTTP_KOCHAB_LOGIN_MAX; i++){
		pthread_mutex_init(&(g_login_info[i].loginfo_mutex), NULL);
	}

	if (pthread_create(&lg_tid, NULL, klogin_th, NULL) != 0){
		MSG_PRINT("create keep login failed.\n");
		return -1;
	}

	return 0;
}

void login_fini(void)
{
	int i;

	log(LOG_DEBUG, "login list will be finished.........");
	if (NULL == g_login_info) {
		log(LOG_DEBUG, "why finished again.........");
		return;
	}
	for(i = 0; i < HTTP_KOCHAB_LOGIN_MAX; i++){
		pthread_mutex_destroy(&(g_login_info[i].loginfo_mutex));
	}

	free(g_login_info);
	g_login_info = NULL;
	return;
}

//please memset loginfo
static inline int get_login_info(int log_id, struct login_info *loginfo, int len){
	if(_PK(!loginfo)){
		return -1;
	}
	if (_PK(len < (int)sizeof(struct login_info))){
		MSG_PRINT("loginfo buf size si too small.\n");
		return -1;
	}

	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	memcpy(loginfo, &g_login_info[log_id], len);
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return 0;
}

static inline int set_login_info(int log_id, const struct login_info *loginfo, int len)
{
	if(_PK(!loginfo)){
		return -1;
	}
	if (_PK(len > (int)sizeof(struct login_info))){
		MSG_PRINT("loginfo buf size is too large.\n");
		return -1;
	}

	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	strcpy(g_login_info[log_id].sid,      loginfo->sid);
	strcpy(g_login_info[log_id].user,     loginfo->user);
	strcpy(g_login_info[log_id].pwd,      loginfo->pwd);
	strcpy(g_login_info[log_id].login_op, loginfo->login_op);
	g_login_info[log_id].islocked = loginfo->islocked;
	g_login_info[log_id].islogin  = loginfo->islogin;
	
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return 0;
}

int is_login(int log_id)
{
	int islogin = 0;
	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	islogin = g_login_info[log_id].islogin;
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return (islogin == HTTP_KOCHAB_IS_LOGIN);
}

static inline void set_login(int log_id, int islogin)
{
	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	g_login_info[log_id].islogin = islogin;
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return;
}

static inline const char * get_login_sid(int log_id)
{
	char * sid = NULL;	
	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	sid = g_login_info[log_id].sid;
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return sid;
}

int login_setlock(int log_id, int f)
{
	if (_PK((log_id < 0) || (log_id > HTTP_KOCHAB_LOGIN_MAX)))
		return -1;

	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	g_login_info[log_id].islocked = f;
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return 0;
}

int login_islock(int log_id)
{
	int islock = 0;
	if (_PK((log_id < 0) || (log_id > HTTP_KOCHAB_LOGIN_MAX)))
		return -1;

	pthread_mutex_lock(&(g_login_info[log_id].loginfo_mutex));
	islock = (g_login_info[log_id].islocked == HTTP_KOCHKA_LOGIN_LOCK)?1:0;
	pthread_mutex_unlock(&(g_login_info[log_id].loginfo_mutex));

	return islock;
}

void login_lock(int log_id)
{
	login_setlock(log_id, HTTP_KOCHKA_LOGIN_LOCK);
	return;
}
void login_unlock(int log_id)
{
	login_setlock(log_id, HTTP_KOCHKA_LOGIN_UNLOCK);
	return;
}

int apply_for_log_id(void){
	int i;
	for (i = 0; i < HTTP_KOCHAB_LOGIN_MAX; i++){
		pthread_mutex_lock(&(g_login_info[i].loginfo_mutex));
		if (g_login_info[i].islocked == HTTP_KOCHKA_LOGIN_UNLOCK){
			pthread_mutex_unlock(&(g_login_info[i].loginfo_mutex));
			return i;
		}
		pthread_mutex_unlock(&(g_login_info[i].loginfo_mutex));
	}

	return -1;
}

int klogin(struct http_response *resp ,struct login_info *login)
{
    char *cookie = NULL;

	if (_PK(!resp) || _PK(!login))
		return -1;

	if (login->user[0] == '\0' || login->pwd[0] == '\0'){
		MSG_PRINT("username or password is invalid.\n");
		return -1;
	}

    if(kop(login->login_op, resp, login->sid) < 0){
        ERR_PRINT("http talk fail\n");
        return -1;
    }

    if(!strstr(resp->content, "result=ok")){
        ERR_PRINT("login failed, result not ok\n");
        goto out;
    }
    if(!(cookie = kget_http_header(resp, HTTP_HEADER_COOKIE))){
        ERR_PRINT("login failed, no cookie get\n");
        goto out;
    }
    if(!kget_sid(cookie, login->sid)){
        ERR_PRINT("login failed, no sid get\n");
        goto out;
    }
	return 0;

out:
    return -1;
}



int klogin_and_keep(int log_id)
{
	char tmp_user[128]  = {0};
	char tmp_pwd[128]   = {0};
	struct http_response *resp   = NULL;
	struct login_info login_info = {{0}};

	strncpy(login_info.user, HTTP_KOCHAB_USERNAME, sizeof(login_info.user));
	strncpy(login_info.pwd,  HTTP_KOCHAB_PASSWORD, sizeof(login_info.pwd));
	kurl_encode(login_info.user, tmp_user, sizeof(tmp_user));
	kurl_encode(login_info.pwd,  tmp_pwd,  sizeof(tmp_pwd));
	snprintf(login_info.login_op, sizeof(login_info.login_op), "op=login&user=%s&password=%s", tmp_user, tmp_pwd);
	/* for safe need thread lock */
	if (set_login_info(log_id, &login_info, sizeof(login_info)) < 0){
		return -1;
	}

	/* login kochab  */
	resp = kcreate_http_response();
	if (!resp){
		goto err;	
	}

	MSG_PRINT("try login id:%d user:%s", 
			log_id,
			g_login_info[log_id].user);

	if(klogin(resp, &login_info) < 0){
		login_info.islogin = HTTP_KOCHAB_NO_LOGIN;
		ERR_PRINT("login fail\n");
		goto err;	
	}
	kfree_http_response(resp);
	resp = NULL;
	login_info.islogin = HTTP_KOCHAB_IS_LOGIN;
	/* update data */
	if (set_login_info(log_id, &login_info, sizeof(login_info)) < 0){
		return -1;
	}
	/* login keep is in klogin thread. */

	MSG_PRINT("login kochab as \n\tlogin id:%d\n\tuser:%s\n\tsid:%s\n", 
			log_id,
			g_login_info[log_id].user, 
			g_login_info[log_id].sid);

	return 0;
err:
	kfree_http_response(resp);
	resp = NULL;
	return -1;
}

static const char *opstr_logout = "op=logout";
int klogout_and_stopkeep(int log_id, int stopKeep){
	struct http_response *resp = NULL;

    if (KLOGOUT_STOP_KEEP == stopKeep) {
        pthread_mutex_lock(&klogin_keep);
        wake_up = KLOGGIN_TH_WAKEUP;
        pthread_cond_signal(&klogin_cont);
        pthread_mutex_unlock(&klogin_keep);
    }

	if (is_login(log_id) == HTTP_KOCHAB_IS_LOGIN){
		set_login(log_id, HTTP_KOCHAB_NO_LOGIN);
		resp = kcreate_http_response();
		if(kop(opstr_logout ,resp, g_login_info[log_id].sid) < 0){
			ERR_PRINT("get path fail\n");
			goto err_s;
		}
		kfree_http_response(resp);
		resp = NULL;
	}

	//pthread_join(lg_tid, NULL);

	return 0;
err_s:
	kfree_http_response(resp);
	resp = NULL;
	return -1;
}


static const char *statuss[HTTP_CMD_RET_STATUS_MAX] = {
	/*[HTTP_CMD_RET_STATUS_DONE]    = */ "done",
	/*[HTTP_CMD_RET_STATUS_DOING]   = */ "doing",
	/*[HTTP_CMD_RET_STATUS_ERR]     = */ "error",
	/*[HTTP_CMD_RET_STATUS_DENY]    = */ "deny",
	/*[HTTP_CMD_RET_STATUS_NONE]    = */ "none",
	/*[HTTP_CMD_RET_STATUS_NEXT]    = */ "next",
	/*[HTTP_CMD_RET_STATUS_UNKNOWN] = */ "unknown",
};

#define HTTP_QUERY_RESUALT_ARRAYS_SIZE (HTTP_QUERY_RESUALT_MAX - __HTTP_QUERY_RESUALT__ - 1)
static const char *results[HTTP_QUERY_RESUALT_ARRAYS_SIZE] = {
	/* [HTTP_QUERY_RESUALT_NOT_SUPPORT - __HTTP_QUERY_RESUALT__ - 1] = */ "not_support",
};

#define HTTP_CFG_RET_ARRAY_SIZE (HTTP_CFG_RET_MAX - __HTTP_CFG_RET_ERR__ - 1)
static const char *errs[HTTP_CFG_RET_ARRAY_SIZE] = {
	/* [HTTP_CFG_RET_INVALID_CT     - __HTTP_CFG_RET_ERR__ - 1] = */ "No such container",
	/* [HTTP_CFG_RET_INVALID_LIFAIL - __HTTP_CFG_RET_ERR__ - 1] = */ "Load table item failed",
};

int kmain(struct http_response *resp, int type)
{
	int ret    = 0;
	int snum   = 0;
	char *s    = NULL;
	struct conf_l  *l  = NULL;
	struct conf_ct *ct = NULL;

	if (_PK(!resp)){
		return -1;
	}

	switch (type)
	{
		case MT_QUERY:
			ret = kget_path(resp->req, resp, get_login_sid(resp->log_id));
			if (ret != 0){
				goto exit_kh;
			}
			if(!(l = conf_str2list(resp->content, resp->content_length))){
				ERR_PRINT("kochab response XML is invalid.\n");
				ret =  HTTP_RESP_ERR_XML_INVALID;
				goto exit_kh;
			}
			if (0 != conf_path2ct(l, resp->req, &ct)) {
				DBG_PRINT("get path err.\n", s);
				if ((s = conf_path2value(l, "return.result", NULL, NULL))) {
					DBG_PRINT("get path err.result:[%s].\n", s);
					for (snum = 0; snum < HTTP_QUERY_RESUALT_ARRAYS_SIZE; snum++) {
						if(!strncmp(results[snum], s, strlen(results[snum]))){
							break;
						}
					}
					if (snum >= HTTP_QUERY_RESUALT_ARRAYS_SIZE){
						ERR_PRINT("can not handle this result.\n");
						ret = HTTP_RESP_ERR_STATUS_ERR;
						goto exit_kh;
					}
					DBG_PRINT("path return result : [%s]\n", results[snum]);
					ret = snum + __HTTP_QUERY_RESUALT__ + 1;
				} else {
					ERR_PRINT("can no get err result.\n");
					ret = HTTP_RESP_ERR_GET_RESULT_ERR;
					goto exit_kh;
				}
			}

			break;
		case MT_CFG:
			ret = kxml_apply(resp->req, resp, get_login_sid(resp->log_id));
			if (ret != 0){
				goto exit_kh;
			}
			if(!(l = conf_str2list(resp->content, resp->content_length))){
				ERR_PRINT("kochab response XML is invalid.\n");
				ret =  HTTP_RESP_ERR_XML_INVALID;
				goto exit_kh;
			}
			if (!(s = conf_path2value(l, "return.result", NULL, NULL))) {
				ERR_PRINT("get xml response return result err.\n", s);
				ret = HTTP_RESP_ERR_GET_RESULT_ERR;
				goto exit_kh;
			}
			if (strncmp("ok", s, strlen("ok")) != 0) {
				ERR_PRINT("xml apply err. result : [%s].\n", s);
				if (!(s = conf_path2value(l, "return.err", NULL, NULL))) {
					ERR_PRINT("get return err msg failed.\n");
					ret = HTTP_RESP_ERR_GET_ERRELE_ERR;
					goto exit_kh;
				}
				for(snum = 0; snum < HTTP_CFG_RET_ARRAY_SIZE; snum++){
					DBG_PRINT("%s == %s", errs[snum], s);
					if (!strncmp(errs[snum], s, strlen(errs[snum]))) {
						break;
					}
				}
				if (snum >= HTTP_CFG_RET_ARRAY_SIZE){
					ERR_PRINT("can not handle this result err.\n");
					ret = HTTP_RESP_ERR_STATUS_ERR;
					goto exit_kh;
				}
				DBG_PRINT("xml return result err: [%s]\n", errs[snum]);
				ret = snum + __HTTP_CFG_RET_ERR__ + 1;
			}
			break;
		case MT_CMD:
			ret = kcmd(resp->req, resp, get_login_sid(resp->log_id));
			if (ret != 0){
				goto exit_kh;
			}
			if(!(l = conf_str2list(resp->content, resp->content_length))){
				ERR_PRINT("kochab response XML is invalid.\n");
				ret =  HTTP_RESP_ERR_XML_INVALID;
				goto exit_kh;
			}
			if (!(s = conf_path2value(l, "return.status", NULL, NULL))) {
				ERR_PRINT("get cmd status error");
				ret = HTTP_RESP_ERR_GTE_STATUS_ERR;
				goto exit_kh;
			}

			for (snum = HTTP_CMD_RET_STATUS_DONE; snum < HTTP_CMD_RET_STATUS_MAX; snum++) {
				if(!strncmp(statuss[snum], s, strlen(statuss[snum]))){
					break;
				}
			}

			if (snum >= HTTP_CMD_RET_STATUS_MAX){
				ERR_PRINT("can not handle this status.\n");
				ret = HTTP_RESP_ERR_STATUS_ERR;
				goto exit_kh;
			}

			DBG_PRINT("cmd return status : [%s]\n", statuss[snum]);
			if (snum == HTTP_CMD_RET_STATUS_DOING) {
				if (!(s = conf_path2value(l, "return.index", NULL, NULL))) {
					ERR_PRINT("get cmd serial error");
					ret = HTTP_RESP_ERR_GTE_SERIAL_ERR;
					goto exit_kh;
				}
				resp->index = atoi(s);

				if (!(s = conf_path2value(l, "return.serial", NULL, NULL))) {
					ERR_PRINT("get cmd serial error");
					ret = HTTP_RESP_ERR_GTE_SERIAL_ERR;
					goto exit_kh;
				}
				resp->serial = atoi(s);
			}

			ret = snum;
			
			break;
		case MT_OP:
			ret = kop(resp->req, resp, "");
			if (0 != ret) {
				ERR_PRINT("kop failed.");
				goto exit_kh;
			}

			DBG_PRINT("op resp:%s", resp->content);

			/*
			 * content = "result=ok" or "result=failed,err_message=pwd wrong" need process at client
			if(!strstr(resp->content, "result=ok")){
				ERR_PRINT("op failed, result not ok\n");
				goto exit_kh;
			}
			*/

			break;
		default:
			return -1;
	}

	DBG_PRINT("cont = [%s], len = [%d]", resp->content, resp->content_length);

exit_kh:
	if (l){
		conf_list_free(l);
		l = NULL;
	}
	return ret;
	//return HTTP_RESP_ERR_RECV_TIMEOUT;
}

