#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>			
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>



#include <ns_base.h>

#include <ns_table.h>
#include <ns_get.h>

#ifdef  DBG_MOD_ID
#undef  DBG_MOD_ID
#endif
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_GET, 1)

static G_OPT_SS g_optss = {0};

#define D_ERR_PRINT(fmt, ...) 	\
	g_optss.err_p += snprintf(g_optss.err + g_optss.err_p, sizeof(g_optss.err), fmt, ##__VA_ARGS__)

#define HTTP_REQ_GET_HEADER \
"GET %s HTTP/1.1\r\n"			\
"Host: %s\r\n"					\
"Connection: keep-alive\r\n"	\
"Accept: */*\r\n"				\
"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.134 Safari/537.36\r\n"	\
"Accept-Encoding: gzip, deflate, sdch\r\n"	\
"Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"		

#define HTTP_REQ_GET_PART_HEADER \
"GET %s HTTP/1.1\r\n"			\
"Host: %s\r\n"					\
"Connection: keep-alive\r\n"	\
"Accept: */*\r\n"				\
"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.134 Safari/537.36\r\n"	\
"Accept-Encoding: gzip, deflate, sdch\r\n"	\
"Accept-Language: zh-CN,zh;q=0.8\r\n"		\
"Range: bytes=%d-%d\r\n\r\n"	

#define VERIFY_CODE "quickbird_speedtest"

#ifdef APP_NS_GET_INDEPENDENT

#define ERR_PRINT(fmt, ...) 	\
	printf("_err_: ");			\
	printf(" >> %s >> %d >>", __func__, __LINE__);		\
	printf(fmt, ##__VA_ARGS__)	

#if 1
#define MSG_PRINT(fmt, ...) \
	printf("_msg_: ");		\
	printf(" >> %s >> %d >>", __func__, __LINE__);		\
	printf(fmt, ##__VA_ARGS__)
#else
#define MSG_PRINT(fmt, ...) 
#endif

#define DAT_PRINT(fmt, ...) \
	printf("_data_: ");		\
	printf(fmt, ##__VA_ARGS__)

#else

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#define ERR_PRINT(fmt, ...) 	\
	ERR_PRINTF((fmt), ##__VA_ARGS__)

#define MSG_PRINT(fmt, ...) \
	MSG_PRINTF((fmt), ##__VA_ARGS__)

#define DAT_PRINT(fmt, ...) \
	MSG_PRINTF((fmt), ##__VA_ARGS__)

#endif

enum so_s{
	SO_S_SEND_HEADER = 0,
	SO_S_SEND_HEADER_PART, /* 1, request part of the download file */
	SO_S_RECV_RESP,        /* 2 */
	SO_S_RECV_RESP_PART,   /* 3 */
	SO_S_REDV_DATA,        /* 4 */
	SO_S_REDV_DATA_PART,   /* 5 */
	SO_S_REDV_OVER,        /* 6*/
	SO_S_RECV_RESP_PART_NULL,   /* 7,没有可用的part了，可以关闭改sock了  */ 
};

typedef struct tag_muti_thread_block_info
{
    DCL_NODE_S stNode;

    int iStartOff; /* offset of the file  */
    int iLen;      /* len of stream to download */
    int iPos;      /* offset of iStartOff  */
	int iIsLoaded; /* this block is loeded flag */
}MUTI_THRD_BLOCK_INFO;

struct so_info {
	int fd;
	int s;
	int filelen;
	int part_len;
	int part_recv_len;
	int recvHeadLen;
	int recvlen; /* recv bytes all but pkt head */ /* use for statistics */
	int recvll;  /* recv bytes one round */
	int sendLen;
	MUTI_THRD_BLOCK_INFO *pstMTblkInfo;
};


struct netspeedtest_rst {
	long rx_start;      /* start download ,usec */
	long tr_start;      /* start upload , usec */
	unsigned long long rx_bytes;      /* download bytes form rx_start */
	unsigned long long tr_bytes;      /* upload bytes form tr_start */
	int  last;          /* last of act */
	int  act;           /* download or upload */
	int  err;			
};

/*
 * [----|--------------------|-----------------------------------]
 * -----^             ^      ^
 *      |             |      |
 *    iStartOff      iPos    end
 *      |<-----iLen-------_->|
 * */

typedef enum tag_MUTI_THRD_LOAD_STATUS
{
	MUTI_THRD_LOAD_STATUS_UPLOAD = 0,
	MUTI_THRD_LOAD_STATUS_LOADING,
	MUTI_THRD_LOAD_STATUS_LOADED,

	MUTI_THRD_LOAD_STATUS_UPLOAD_MAX,
}MUTI_THRD_LOAD_STATUS_EN;

static int mtb_list_create(G_OPT_S *pstOpt)
{
	int iPos     = 0;
	int iPerBlockLen = (128 * 1024);
	MUTI_THRD_BLOCK_INFO *pstMtbInfo = NULL;
	MUTI_THRD_BLOCK_INFO *pstMtbInfoNext = NULL;

	DCL_Init(&(pstOpt->stMHead));

	if (pstOpt->filesize == 0) {
		ERR_PRINT("invalied file length.");
		return -1;
	}

	if (iPerBlockLen > (pstOpt->filesize)) {
		//iPerBlockLen = iPerBlockLen - (pstOpt->filesize);
		iPerBlockLen = pstOpt->filesize;
	}
	while(iPos < (pstOpt->filesize)) {
		pstMtbInfo = malloc(sizeof(MUTI_THRD_BLOCK_INFO));
		if (NULL == pstMtbInfo) {
			ERR_PRINT("invalied file length.");
			goto err;
		}
		memset(pstMtbInfo, 0, sizeof(MUTI_THRD_BLOCK_INFO));
		pstMtbInfo->iStartOff = iPos;
		if ((pstOpt->filesize - iPos) >= iPerBlockLen) {
			pstMtbInfo->iLen = iPerBlockLen;
		} else {
			pstMtbInfo->iLen = pstOpt->filesize - iPos;
		}

		DCL_AddTail(&(pstOpt->stMHead), &(pstMtbInfo->stNode));

		iPos += pstMtbInfo->iLen;
	}

	return 0;
err:
	DCL_FOREACH_ENTRY_SAFE(&(pstOpt->stMHead), pstMtbInfo, pstMtbInfoNext, stNode)  {
		DCL_Del(&(pstMtbInfo->stNode));
		free(pstMtbInfo);
	}
	return -1;
}

MUTI_THRD_BLOCK_INFO *mtb_list_get_block_info(void) 
{
	MUTI_THRD_BLOCK_INFO *pstMTblkInfo = NULL;

	//TODO: need a lock ?
	DCL_FOREACH_ENTRY(&(g_optss.opts_d.stMHead), pstMTblkInfo, stNode) {
		if (pstMTblkInfo->iIsLoaded == MUTI_THRD_LOAD_STATUS_UPLOAD) {
			return pstMTblkInfo;
		}
	}

	return NULL;
}

VOID mtb_list_set_block_load(MUTI_THRD_BLOCK_INFO *pstMTblkInfo, MUTI_THRD_LOAD_STATUS_EN enLoadStat) 
{
	DBGASSERT(pstMTblkInfo != NULL);

	pstMTblkInfo->iIsLoaded = enLoadStat;

	return;
}

int m_download_test(void)
{
	int cnt = 0;
	MUTI_THRD_BLOCK_INFO *pstMtbInfo = NULL;

	g_optss.opts_d.filesize = 1000000;
	mtb_list_create(&(g_optss.opts_d));
	DCL_FOREACH_ENTRY(&(g_optss.opts_d.stMHead), pstMtbInfo, stNode) {
		MSG_PRINTF("Start off:%d blen=%d", pstMtbInfo->iStartOff, pstMtbInfo->iLen);
		cnt += pstMtbInfo->iLen;
	}
	MSG_PRINTF("cnt len=%d", cnt);

	return 0;
}

static int
getHostByName(const char *hostName, char *host, int len)
{
    const char  *ptr = NULL;
    char **pptr = NULL;
    struct hostent *hptr = NULL;
    char   str[16] = {0};

    ptr = hostName;

    if ((hptr = gethostbyname(ptr)) == NULL){
        ERR_PRINT("gethostbyname error for host:%s\n", ptr);
        return -1;
    }

    //MSG_PRINT("official hostname:%s\n", hptr->h_name);

    //for( pptr = hptr->h_aliases; *pptr != NULL; pptr++){
    //    MSG_PRINT("    alias: %s\n", *pptr);
    //}

    switch( hptr->h_addrtype ){
        case AF_INET:
        case AF_INET6:
            for ( pptr = hptr->h_addr_list; *pptr != NULL; pptr++ ){
                MSG_PRINT( "    address: %s\n",
                         inet_ntop( hptr->h_addrtype, *pptr, str, sizeof(str)));
            }
            break;
        default:
            ERR_PRINT( "unknown address type\n" );
            break;
    }

    sprintf(host, "%s", str);

    return 0;
}

static int 
socket_open(char *ip,int Port){
	int sockfd;
	struct sockaddr_in seraddr;

	if (NULL == ip){
		goto err;
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd){
		ERR_PRINT("Create socket failed.\n");
		goto err;
	}

	memset(&seraddr, 0 , sizeof(struct sockaddr_in));
	seraddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &seraddr.sin_addr);
	seraddr.sin_port = htons(Port);

	if(fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0){
		ERR_PRINT("Set Non BLock %s:%d Failed!", ip, Port);
		goto err1;
	}

	if(-1 == connect(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr))){
        MSG_PRINT("connect server failed.errno = %d.\n", errno);
        if (errno == EINPROGRESS) {
            MSG_PRINT("connect will do later .\n");
        } else {
            ERR_PRINT("connect server failed.errno = %d.\n", errno);
            goto err1;
        }
	}

	return sockfd;

err1:
	close(sockfd);
err:
	return -1;
}

static void 
socket_close(int fd)
{
	close(fd);
	return;
}

static int 
socket_send(int fd, char *buf, int len)
{
	int ret = 0;
	ret = send(fd, buf, len, 0);
	return ret;
}

static int 
ck_url(G_OPT_S *g_optss)
{
	char *p = NULL, *p1 = NULL, *pp = NULL;
	char  port[16] = {0};

	if (g_optss->urlBuf[0] == 0){
		ERR_PRINT("URL or filepath is INVALID.Please Check.\n");
		goto err;
	}

	/* http://dl.quickbird.com:80/speedtest20M.zip */
	p = strstr(g_optss->urlBuf, "http://");
	if (p == NULL){
		ERR_PRINT ("URL addrss is INVALID.Please Check.\n");
		goto err;
	}
	p += 7;
	p1 = strstr(p, ":");
	if (!p1){
		p1 = strstr(p, "/");
		if (!p1){
			ERR_PRINT ("URL addrss is INVALID.Please Check .\n");
			goto err;
		}
	}else{
		pp = p1;
	}
	if (p1 - p >= sizeof(g_optss->hostName)){
		ERR_PRINT("host name is too long.\n");
		goto err;
	}
	//strncpy(g_optss->hostName, p, p1 - p);
    snprintf(g_optss->hostName, p1 - p + 1, "%s", p);
    MSG_PRINT ("-------------%s.\n", g_optss->hostName);
	if (pp){
		pp += 1;
		p1 = strstr(pp, "/");
		if (!p1){
			ERR_PRINT ("URL addrss is INVALID.Please Check .\n");
			goto err;
		}
		if (p1 - pp >= sizeof(port)){
			ERR_PRINT("port string is too long.\n");
			goto err;
		}
		strncpy(port, pp, p1 - pp);
	}else{
		strncpy(port, "80", sizeof(port));
	}
	
	p = p1;
    /* //use the full path as the URL 
	p1 = strstr(p, "?");
	if (p1){
		if (p1 - p>= sizeof(g_optss->u_path)){
			ERR_PRINT("url path string is too long.\n");
			goto err;
		}
		strncpy(g_optss->u_path, p, p1 - p);
	}else{
    */
		strncpy(g_optss->u_path, p, sizeof(g_optss->u_path));
	//}
			
	if (getHostByName(g_optss->hostName, g_optss->ipAddr, sizeof(g_optss->ipAddr)) < 0) {
        goto err;
    }
	
	g_optss->port = atoi(port);

	MSG_PRINT("=%s=%s=%s=%d=\n", g_optss->hostName, g_optss->ipAddr, g_optss->u_path, g_optss->port);
	return 0;

err:
	return -1;
}

#ifdef APP_NS_GET_INDEPENDENT
static int 
opt_ckeck(int argc, char *argv[])
{
	int c  = 0;

    /* set default duration and default period */
    g_optss.duration = 10;
    g_optss.period   = 2;

	while (1){
		c = getopt (argc, argv, "uD:U:f:T:p:dh");
		if (c == -1)
			break;
      	switch (c){
      	case 'u':
      		g_optss.flag |= OPT_FLAG_UP_FILE;
      		break;
		case 'd':
			g_optss.flag |= OPT_FLAG_DOWNLOAD_FILE;
			break;
		case 'D':
			g_optss.flag |= OPT_FLAG_DOWNLOAD_FILE;
			snprintf(g_optss.opts_d.urlBuf, sizeof(g_optss.opts_d.urlBuf), "%s", optarg);
			MSG_PRINT("Download url = %s\n", g_optss.opts_d.urlBuf);
			break;
		case 'U':
      		g_optss.flag |= OPT_FLAG_UP_FILE;
			snprintf(g_optss.opts_u.urlBuf, sizeof(g_optss.opts_u.urlBuf), "http://%s/", optarg);
			//snprintf(g_optss.opts_u.urlBuf, sizeof(g_optss.opts_u.urlBuf), "%s", optarg);
			MSG_PRINT("opt flags = 0x%08X\n",   (unsigned int)g_optss.flag);
			MSG_PRINT("Upload url = %s\n",   g_optss.opts_u.urlBuf);
			break;
		case 'f':
			//MSG_PRINT("file = %s\n", optarg);
			sprintf(g_optss.opts_d.filePath, "%s", optarg);
			sprintf(g_optss.opts_u.filePath, "%s", optarg);
			/* http://117.78.18.108/post_mac_ruian.php? */
			break;
		case 'T':
			g_optss.duration = atoi(optarg);
			MSG_PRINT("-T = %d\n",  g_optss.duration);
			break;
		case 'p':
			g_optss.period = atoi(optarg);
			MSG_PRINT("-p = %d\n",  g_optss.period);
			break;
		case 'h':
		default:
			MSG_PRINT("\n\tuse: -U  http:192.168.1.1:80 -T 10 -p 2 \n");
            exit(0);
		}
	}

	if (0 != (g_optss.flag & OPT_FLAG_DOWNLOAD_FILE)){
		if(ck_url(&(g_optss.opts_d)) < 0){
			goto err;
		}
	}
	if (0 != (g_optss.flag & OPT_FLAG_UP_FILE)){
		if(ck_url(&(g_optss.opts_u)) < 0){
			goto err;
		}
	}

    if (!g_optss.flag) {
        MSG_PRINT("\n\t down or up!? \n");
        goto err;
    }

	return 0;

err:
	return -1;
}
#else
static int 
opt_ckeck(int argc, char *argv[])
{

	if (0 != (g_optss.flag & OPT_FLAG_DOWNLOAD_FILE)){
		MSG_PRINT("Download url = %s\n", g_optss.opts_d.urlBuf);
		if(ck_url(&(g_optss.opts_d)) < 0){
			goto err;
		}
	}
	if (0 != (g_optss.flag & OPT_FLAG_UP_FILE)){
		MSG_PRINT("Upload url = %s\n",   g_optss.opts_u.urlBuf);
		if(ck_url(&(g_optss.opts_u)) < 0){
			goto err;
		}
	}

    if (!g_optss.flag) {
        MSG_PRINT("\n\t down or up!? \n");
        goto err;
    }

	return 0;

err:
	return -1;
}
#endif

static int create_open_donwload_file(G_OPT_S *pstOptD)
{
	FILE *fp_d = NULL;
	char  *p = NULL, *p_start = NULL;

	DBGASSERT(NULL != pstOptD);

	if (pstOptD->filePath[0] == '\0') {
		p_start = p = (char *)(pstOptD->urlBuf);
		p = p + strlen(pstOptD->urlBuf);
		while(p != p_start) {
			if (*p == '/') {
				break;
			}
			p--;
		}
		if (*p == '/') {
			snprintf(pstOptD->filePath, sizeof(pstOptD->filePath), "%s", p + 1);
		} else {
			snprintf(pstOptD->filePath, sizeof(pstOptD->filePath), "%s", "download.default");
		}

	}

	MSG_PRINT("download file path:%s", pstOptD->filePath);

	fp_d = fopen(pstOptD->filePath, "w+");
	if (fp_d == NULL) {
		ERR_PRINT("open file failed %s", pstOptD->filePath);
		return -1;
	}

	pstOptD->fp = fp_d;

	return 0;
}

static int g_file_is_truncated = 0;
static int ftruncate_download_file(G_OPT_S *pstOpt)
{
	int fd = -1;

	if (g_file_is_truncated == 1) {
		return 0;
	}

	fd = fileno(pstOpt->fp);
	if (-1 == fd) {
		ERR_PRINT("file to fd NO. failed %s", pstOpt->filePath);
		return -1;
	}

	MSG_PRINT("truncated file [%s] to len:%d", pstOpt->filePath, pstOpt->filesize);
	if ( -1 == ftruncate(fd, pstOpt->filesize)) {
		ERR_PRINT("file truncate . failed %s", pstOpt->filePath);
		return -1;
	}

	if (g_file_is_truncated == 0) {
		g_file_is_truncated = 1;
	}

	return 0;
}

static int close_download_file(G_OPT_S *pstOpt)
{
	DBGASSERT(NULL !=  pstOpt);

	if (pstOpt->fp) {
		fclose(pstOpt->fp);
		pstOpt->fp = NULL;
	}

	return 0;
}

static int write_download_file(G_OPT_S *pstOpt, int offset, void *pData, int iDataLen)
{
	int iwrite = 0;

	if (pstOpt->fp == NULL) { 
		ERR_PRINT("file is no open!");
		return -1;
	}

	fseek(pstOpt->fp, offset, SEEK_SET);

	iwrite = fwrite(pData, 1, iDataLen, pstOpt->fp);
	if (iwrite != iDataLen) {
		ERR_PRINT("file write!");
		return -1;
	}

    /*
	MSG_PRINT("write file:%s pos:%d len:%d loaded%d", 
        pstOpt->filePath, 
        pstOpt->filePos, 
        iDataLen, 
        pstOpt->filePos + iDataLen);
        */

	pstOpt->filePos += iwrite;	

	return 0;
}

static int 
do_upload(struct so_info *so)
{
	char filePkt[FILE_PAK_LEN + 1 + 512] = {0};
	int  readRet = 0;
	char httpHeaderBuf[HTTP_BUF_SIZE] = {0};
	int sendLen = 0;
	int sendtlen = 0;

	if (so->s == SO_S_SEND_HEADER){
		/* use it to query */
		sprintf(httpHeaderBuf, "%s", VERIFY_CODE);
		if((sendLen == socket_send(so->fd, httpHeaderBuf, strlen(httpHeaderBuf))) == -1){
			ERR_PRINT("Send http header Failed.");
			goto err0;
		}
		//MSG_PRINT("\nSend http VERIFY_CODE\n");
		so->sendLen += sendLen;
		so->s = SO_S_REDV_DATA;
	}else if (so->s == SO_S_RECV_RESP){
		so->s = SO_S_REDV_DATA;
	}else if (so->s == SO_S_REDV_DATA){
		memset(filePkt, '0', sizeof(filePkt));
		readRet = FILE_PAK_LEN;
		while(1){
			sendLen = socket_send(so->fd, filePkt, readRet);
			sendtlen += sendLen;
			if ((sendLen != readRet && errno == 0) || (errno == EAGAIN)){
				so->sendLen += sendtlen;
				return sendtlen;
			}
			if(-1 == sendLen){
				ERR_PRINT("Send File data Failed erron=%d\n", errno);
				goto err0;
			}

		}
	}
	
	return sendLen;
err0:

	return -1;
}

#define SO_S_CHANGE(st) \
	do { \
		MSG_PRINTF("so[fd:%d] status %d --> %d\n", so->fd, so->s, (st)); \
		so->s = (st);\
	} while (0)

static int 
do_download(struct so_info *so)
{
	int  ss = 0;
	int  part_len = 0;
	int  part_start = 0;
	int  part_end = 0;
	int  fd = so->fd;
	int  recvLen = 0;
	char *p = NULL, *p1 = NULL;
	int  recv200 = 0;
	char filePkt[FILE_PAK_LEN + 1 + 512] = {0};
	char httpHeaderBuf[HTTP_BUF_SIZE] = {0};
	MUTI_THRD_BLOCK_INFO *pstMTblkInfo = NULL;

	/*
	static int test_i = 0;
	if (test_i < 100) {
		return 0;
	}
	*/

	//MSG_PRINT("Send Header\n");
	if (so->s == SO_S_SEND_HEADER || so->s == SO_S_SEND_HEADER_PART){
		if (so->s == SO_S_SEND_HEADER) {
			part_len = 10240;
			ss = SO_S_RECV_RESP;
			g_optss.opts_d.enStat = NS_DOWNLOAD_STAT_GETING_FILE_INFO;
		}else if (so->s == SO_S_SEND_HEADER_PART){
			//part_len = 102400;
			pstMTblkInfo = mtb_list_get_block_info();
			if (NULL == pstMTblkInfo) {
				ERR_PRINT("no valid mtb.");
				//goto err0;
				SO_S_CHANGE(SO_S_RECV_RESP_PART_NULL);
				return 0;
			}
			pstMTblkInfo->iIsLoaded = MUTI_THRD_LOAD_STATUS_LOADING;

			part_len = pstMTblkInfo->iLen;
			part_start = pstMTblkInfo->iStartOff;
			part_end = part_start + part_len - 1;
			so->pstMTblkInfo = pstMTblkInfo;

			ss = SO_S_RECV_RESP_PART;
		}

		ERR_PRINT("------recvLen:%d-------------------", so->recvlen);
		if (so->recvlen + part_len > so->filelen) {
			part_len = so->filelen - so->recvlen;
		}
		sprintf(httpHeaderBuf, HTTP_REQ_GET_PART_HEADER, g_optss.opts_d.u_path, g_optss.opts_d.hostName, 
				part_start, part_end/* g_optss.opts_d.filesize */);
			//	so->recvlen, so->recvlen + part_len/* g_optss.opts_d.filesize */);
		if(-1 == socket_send(fd, httpHeaderBuf, strlen(httpHeaderBuf))){
			ERR_PRINT("Send http header Failed.\n");
			goto err0;
		}
		MSG_PRINT("Send Header success!\n =%s=\n", httpHeaderBuf);
		//so->s = ss;
		SO_S_CHANGE(ss);
	}else if (so->s == SO_S_RECV_RESP || so->s == SO_S_RECV_RESP_PART){
		/* get Http resp head */
		//MSG_PRINT("get Http resp head\n");
		while(1){
			memset(filePkt, 0, sizeof(filePkt));
			recvLen = recv(fd, filePkt, sizeof(filePkt), 0);
			if (recvLen > 0){
				//MSG_PRINT("get Http resp head:%s\n", filePkt);
				/* "HTTP/1.1 200 OK\r\n" */
				p = strstr(filePkt, "HTTP");
				if (NULL == p){
					continue;
				}
				p = strstr(p, " ") + 1;
				if (p != NULL){
					p1 = strstr(p, "\r\n");
				}
				if (p1 != NULL && (p1 - p) > 0){
					if(!strncmp(p, "200 OK", p1 - p)){
						recv200 = 1;
					}else if(!strncmp(p, "206 Partial Content", p1 - p)){
						recv200 = 1;
                    }else if (!strncmp(p, "302 Found", p1 - p)) {
                        MSG_PRINT("download url is relocation.\n");
                        if ((p = strstr(p1, "Location: "))) {
                            p += strlen("Location: ");
                            p1 = strstr(p, "\r\n");
                        } else {
                            ERR_PRINT("can not find relocation url.\n");
                            goto err0;
                        }
                        if (p && p1 && ((p1 - p ) > 0)) {
                            if (sizeof(g_optss.opts_d.urlBuf) < (p1 - p + 1)) {
                                ERR_PRINT("relocation url is too long.\n");
                                goto err0;
                            }
                            snprintf(g_optss.opts_d.urlBuf, p1 - p + 1, "%s", p);
                            MSG_PRINT("ReLocation new url:==%s==\n", g_optss.opts_d.urlBuf);
                            so->s = SO_S_SEND_HEADER;
                            ck_url(&g_optss.opts_d);
                            //getHostByName(g_optss.opts_d.hostName, g_optss.opts_d.ipAddr,sizeof(g_optss.opts_d.ipAddr));
                            return NSPT_ERR_ERR;
                        } else {
                            ERR_PRINT("new relocation url is invalid.\n");
                            goto err0;
                        }
					} else {
						snprintf(g_optss.opts_d.err, p1 - p + 2, "%s", p);
						ERR_PRINT("HTTP response failed :%s\n", g_optss.opts_d.err);
						goto err0;
					}
					break;
				}else{
					break;
				}
			}
		}
		
		if (recv200 == 1){
			//MSG_PRINT("Get 200 Respose.\n");
			/*
			 *Content-Range: bytes 0-1024/1042196
			 *Content-Length: 1025
			 */
            char lenbuf[64];
			int file_len = 0;
			int pkt_ask_len = 0;
			int pkt_content_len = 0;

			p = strstr(p, "Content-Range: ");
			if (p == NULL){
				ERR_PRINT("Can not find Content-Range.\n");
				goto err0;
			}
			p += strlen("Content-Range: ");
			p1 = strstr(p, "/");
			if (p == NULL){
				ERR_PRINT("bad Content-Range.\n");
				goto err0;
			}
			//ERR_PRINTF("[%s]\n[%s]\n", p, p1);
            snprintf(lenbuf, p1 - p + 1,"%s", p);
			ERR_PRINTF("response len : [%s]", lenbuf);
			sscanf(lenbuf, "bytes %d-%d", &part_start, &part_end);
			ERR_PRINTF("[%d]-[%d]", part_start, part_end);
			pkt_ask_len = part_end - part_end;

			p = p1 + 1; /* skip '/' */
			p1 = strstr(p, "\r\n");
			if (p == NULL){
				ERR_PRINT("bad Content-Range.\n");
				goto err0;
			}
            snprintf(lenbuf, p1 - p + 1,"%s", p);
			file_len = atoi(p);

			p = strstr(p, "Content-Length: ") + 16;
			if (p == NULL)
			{
				ERR_PRINT("Can not find Content-Length.\n");
				goto err0;
			}
			p1 = strstr(p, "\r\n");
			if (p1 == NULL)
			{
				ERR_PRINT("Can not find Content-Length 2.\n");
				goto err0;
			}

            snprintf(lenbuf, p1 - p + 1,"%s", p);
			pkt_content_len = atoi(p);

			ERR_PRINT("ask len:%d file len:%d content len:%d", 
					pkt_ask_len, file_len, pkt_content_len);
			/* 第一个请求，为 0-0 字节, 返回的range也是 0-0字节，但是长度为1
			 * response "bytes 0-0/1042196"
			 * 所以只有在 part_start > part_end 时才记录。
			 * */
			if ((part_start == part_end) && (part_end == 0) && (pkt_content_len == 1)) {
				recvLen -= pkt_content_len;
				pkt_content_len = 0;
				ERR_PRINT("first response pkt \n\t"
						  "set [content len] -> [%d]\n\t"
						  "set [recvLen len] -> [%d]", 
						  pkt_content_len,
						  recvLen);
			}

			/* first resp head get file len */
			if (so->s == SO_S_RECV_RESP) {
				so->filelen = file_len;
				if (so->filelen <= 0){
					ERR_PRINT("Invaild file length.\n");
					goto err0;
				}
				g_optss.opts_d.filesize = so->filelen;

				if (mtb_list_create(&(g_optss.opts_d)) < 0) {
					ERR_PRINT("mutil block list create failed.\n");
					goto err0;
				}

				/* truncate file size and occupy space */
				if (ftruncate_download_file(&(g_optss.opts_d)) < 0) {
					goto err0;
				}

				MSG_PRINT("File length:%d", so->filelen);
			}

			so->part_len = pkt_content_len;

			p = strstr(p1 + 2, "\r\n\r\n") + 4;
			if (p == NULL)
			{
				ERR_PRINT("Can not find 200 OK data.\n");
				goto err0;
			}

			g_optss.opts_d.enStat = NS_DOWNLOAD_STAT_GETTED_FILE_INFO;

			//MSG_PRINT("Begin Download data.\n");

			//so->recvlen = recvLen; /* this value is different at download flie should be "hdl" */
			if (recvLen > 0){
				so->recvHeadLen = (p - filePkt);
				so->part_recv_len += (recvLen - so->recvHeadLen);
				so->recvll  += recvLen;

				ERR_PRINT("head len:%d", so->recvHeadLen);
				so->recvlen += (recvLen - so->recvHeadLen);
				g_optss.opts_d.recvTotalLen += (recvLen - so->recvHeadLen);
				ERR_PRINT("------recvLen:%d-------------------", so->recvlen);

				/* A PDU frame. will load data */
				if (recvLen > so->recvHeadLen) {
					if (write_download_file(&(g_optss.opts_d), 
								so->pstMTblkInfo->iStartOff, 
								(filePkt + so->recvHeadLen), 
								(recvLen - so->recvHeadLen)) < 0) {
						goto err0;
					}
				}
			}


            char hbuf[4096];
            snprintf(hbuf, so->recvHeadLen, "%s", filePkt);
            printf(hbuf);

			/* first pkt have all data */
			if (so->part_recv_len >= so->part_len){
				MSG_PRINT("part receive over.");
				so->part_len = 0;
				so->part_recv_len = 0;
				/* next step send next part quest */
				SO_S_CHANGE(SO_S_SEND_HEADER_PART);
				return recvLen;
			}

			//so->s = SO_S_REDV_DATA;
			/* next send part only data */
			SO_S_CHANGE(SO_S_REDV_DATA_PART);
		}else{
			ERR_PRINT("Revice Failed.\n");
			goto err0;
		}
	}else if (so->s == SO_S_REDV_DATA || so->s == SO_S_REDV_DATA_PART){
		while(1){		
			
			memset(filePkt, 0, sizeof(filePkt));
			recvLen = recv(fd, filePkt, sizeof(filePkt), 0);
			if (recvLen > 0){
				if (write_download_file(&(g_optss.opts_d), 
							so->pstMTblkInfo->iStartOff + so->part_recv_len,
							filePkt, 
							recvLen) < 0) {
					goto err0;
				}
				so->recvlen += recvLen;
				g_optss.opts_d.recvTotalLen += recvLen;
				so->part_recv_len += recvLen;
				so->recvll  += recvLen;
			}			            
            
			//MSG_PRINT("errno = %d\n.", errno);
            if (errno == EAGAIN && recvLen < 0){
                return NSPT_ERR_EAGAIN; 
            }
			
			/* part receive over */
			MSG_PRINT("[fd:%d] part_recv_len=%d, recvlen = %d filelen = %d , recvLen = %d recvTotalLen = %d.", 
					so->fd, so->part_recv_len, so->recvlen, so->filelen , recvLen, g_optss.opts_d.recvTotalLen);
			if (so->part_recv_len >= so->part_len){
				MSG_PRINT("part receive over.");
				so->part_len = 0;
				so->part_recv_len = 0;
				//so->s = SO_S_SEND_HEADER_PART;
				DBGASSERT (so->pstMTblkInfo != NULL)
				mtb_list_set_block_load(so->pstMTblkInfo, MUTI_THRD_LOAD_STATUS_LOADED);
				so->pstMTblkInfo = NULL;
				//if (so->recvlen >= so->filelen){
				if (g_optss.opts_d.recvTotalLen >= so->filelen){
					/* last part and all data download. */
					g_optss.opts_d.enStat = NS_DOWNLOAD_STAT_DONWLOAD_SUCCESS;
					return NSPT_ERR_CONNABR; 
				} else {
					SO_S_CHANGE(SO_S_SEND_HEADER_PART);
				}
				return recvLen;
			}

			if (so->recvlen >= so->filelen || recvLen == 0){
				//MSG_PRINT("errno = %d\n.", errno);
				MSG_PRINT("recvlen = %d filelen = %d , recvLen = %d.\n", so->recvlen, so->filelen , recvLen);
                so->recvlen = 0;
				so->part_recv_len =0;
				//so->s = SO_S_SEND_HEADER;
				SO_S_CHANGE(SO_S_SEND_HEADER);
                return NSPT_ERR_CONNABR; 
			}

			if (recvLen < 0){
				//so->s = SO_S_SEND_HEADER;
				SO_S_CHANGE(SO_S_SEND_HEADER);
                return NSPT_ERR_ERR; 
			}

		}
	}

	return recvLen;
	
err0:
	return NSPT_ERR_DEAD;
}

struct netspeedtest_rst *rst = NULL;
static pthread_mutex_t rpt_mutex = PTHREAD_MUTEX_INITIALIZER;
static int over = 0;

#define NS_GET_DOWNLOAD_IS_OVER(flag) \
	(((flag) & OPT_FLAG_DOWNLOAD_FILE) == 0)
#define NS_GET_UPLOAD_IS_OVER(flag) \
	(((flag) & OPT_FLAG_UP_FILE) == 0)
#define NS_GET_DONW_UP_IS_OVER(flag) \
	(NS_GET_DOWNLOAD_IS_OVER(flag) && NS_GET_UPLOAD_IS_OVER(flag))
#define NS_GET_JUST_DONW_FILE(flag) \
	(((flag) & OPT_FLAG_NO_TEST) != 0)


static int 
do_get_speed(int duratoin)
{
	struct netspeedtest_rst *t = NULL;
	struct timespec clc = {0, 0};  
	long t_start = 0, t_boot = 0, t_go;
	
	t = malloc(sizeof(struct netspeedtest_rst));
	if (!t){
		ERR_PRINT("malloc error.\n");
		goto exit;
	}
	memset(t, 0, sizeof(struct netspeedtest_rst));	

	pthread_mutex_lock(&rpt_mutex);
	memcpy(t, rst, sizeof(struct netspeedtest_rst));

    /* download file over */
	if (NS_GET_JUST_DONW_FILE(g_optss.flag) && (rst->last == RPT_ACT_DOWNLOAD_FILE_OVER)){	
		t_start = t->rx_start;
		clock_gettime(CLOCK_MONOTONIC, &clc); 
		t_boot = clc.tv_sec * 1000 + clc.tv_nsec / 1000000;
		t_go = t_boot - t_start;
		printf("Total:%d Bytes, %ld Sec, %ld KB/s \n", g_optss.opts_d.filePos, (t_go / 1000), ((g_optss.opts_d.filePos / 1000) / (t_go / 1000)));
        goto over;
    }

	/* if ship do not stop, wait it */
	if (rst->last != RPT_ACT_NONE){	
//		MSG_PRINT("wait ship do with last.\n");
		pthread_mutex_unlock(&rpt_mutex);
		goto exit;
	}
	pthread_mutex_unlock(&rpt_mutex);
	if (t->act == RPT_ACT_NONE){
		if (t->err == RPT_ACT_ERR){
			ERR_PRINT("Speet test error.act = %d\n", t->act);
			goto over;
		}
		goto exit;
	}
	
	/* check if time is out */
	if (t->act == RPT_ACT_DOWNLOAD){
		t_start = t->rx_start;
	}else if (t->act == RPT_ACT_UPLOAD){
		t_start = t->tr_start;
	}
	clock_gettime(CLOCK_MONOTONIC, &clc); 
	t_boot = clc.tv_sec * 1000 + clc.tv_nsec / 1000000;
	
	//MSG_PRINT ("Check (tboot-tstar= %ld ) (duratoin * 1000 = %d) %s >>> %d\n", (t_boot - t_start), (duratoin * 1000), __func__, __LINE__);
	t_go = t_boot - t_start;
	
	if ((t_boot - t_start) >= (duratoin * 1000)){
		pthread_mutex_lock(&rpt_mutex);
		rst->last = rst->act;
		pthread_mutex_unlock(&rpt_mutex);
		t->last = t->act;
        if (t->last == RPT_ACT_DOWNLOAD){
		    g_optss.flag &= ~(OPT_FLAG_DOWNLOAD_FILE);
        }else if(t->last == RPT_ACT_UPLOAD){
		    g_optss.flag &= ~(OPT_FLAG_UP_FILE);
        }
	}
		
	if (t->act == RPT_ACT_DOWNLOAD && t->rx_start == 0){
		goto exit;
	}
	if (t->act == RPT_ACT_DOWNLOAD || t->last == RPT_ACT_DOWNLOAD){
		//DAT_PRINT("%lld,%ld,%d\n", t->rx_bytes, t->rx_start, t->last);
		t->last = RPT_ACT_NONE;
	}
	if (t->act == RPT_ACT_UPLOAD   && t->tr_start == 0) {
		goto over;
	}
	if(t->act == RPT_ACT_UPLOAD || t->last == RPT_ACT_UPLOAD){
		//DAT_PRINT("%lld,%ld,%d\n", t->tr_bytes, t->tr_start, t->last);
	}
	
	if (t->err == RPT_ACT_ERR){
		ERR_PRINT("Speet test error.act = %d\n", t->act);
		printf("_err_: %s\n", g_optss.err);
		goto over;
	}

	if (t->act == RPT_ACT_DOWNLOAD || t->last == RPT_ACT_DOWNLOAD){
		MSG_PRINT("flag=%d t:%-10ld %-18lld", g_optss.flag, t_go, t->rx_bytes);
		//DAT_PRINT("%lld,%ld,%d\n", t->rx_bytes, t->rx_start, t->last);
	}
	if(t->act == RPT_ACT_UPLOAD || t->last == RPT_ACT_UPLOAD){
		MSG_PRINT("flag=%d t:%-10ld %-18lld", g_optss.flag, t_go, t->tr_bytes);
	}

	if (NS_GET_DONW_UP_IS_OVER(g_optss.flag)){
		goto over;
	}

	fflush(stdout);
exit:	
	free(t);
	return 0;

over:
	free(t);
	return 1;
}

static void *
get_speed_th(void *arg)
{
	int period = g_optss.period;
	
	while(1){
		usleep(period * 1000 * 1000);
	
		if(do_get_speed(g_optss.duration) == 1){
			break;
		}
	}

	MSG_PRINT("GET ALL Over.");
	
	return (void *)0;
}

static int mutil_sock_open(G_OPT_S *opts)
{
	int fd = 0;

	fd = socket_open(opts->ipAddr, opts->port);
	if (fd < 0 ){
		ERR_PRINT("Connect %s:%d Failed!", opts->ipAddr, opts->port);
		goto err0;
	}

	return fd;
err0:
	return -1;
}

static int 
ship(char n_act, G_OPT_S *opts)
{
	int i =0, sret = 0;
	fd_set fds_write, fds_read; 
	int maxsock = 0;
	int read_len = 0, write_len = 0;
	int err = -1, ret = 0;
	long t_boot = 0;
	struct so_info sos[SOCKET_MAX_NUM] = {{0}};
	int sock_num = 1;
	
	struct timespec clc = {0, 0};  
	struct timeval  tmo = {0};
    clock_gettime(CLOCK_MONOTONIC, &clc); 
	t_boot = clc.tv_sec*1000 + clc.tv_nsec/1000000;
    
	pthread_mutex_lock(&rpt_mutex);
	if (n_act == RPT_ACT_DOWNLOAD){
		rst->rx_start = t_boot; 
	}else if (n_act == RPT_ACT_UPLOAD){
		rst->tr_start = t_boot;
	}
	rst->act = n_act;
	pthread_mutex_unlock(&rpt_mutex);

	while(1){
		maxsock = 0;
		write_len = 0;
		read_len = 0;
		FD_ZERO(&fds_write);
		FD_ZERO(&fds_read);

		if ((g_optss.opts_d.enStat == NS_DOWNLOAD_STAT_INIT) ||
				(g_optss.opts_d.enStat == NS_DOWNLOAD_STAT_GETING_FILE_INFO)) {
			sock_num = 1;
		} else if (g_optss.opts_d.enStat == NS_DOWNLOAD_STAT_GETTED_FILE_INFO) {
		    g_optss.opts_d.enStat = NS_DOWNLOAD_STAT_MUTIL_DONWLOAD;
			sock_num = MUTI_DOWNLOAD_SOCKET_NUM;
		} else if (g_optss.opts_d.enStat == NS_DOWNLOAD_STAT_MUTIL_DONWLOAD) {
			sock_num = MUTI_DOWNLOAD_SOCKET_NUM;
		}

		for (i = 0; i < sock_num; i++){

			if (sos[i].s == SO_S_RECV_RESP_PART_NULL) {
				if (sos[i].fd > 0) {
					MSG_PRINT("close sock fd:%d", sos[i].fd);
					close(sos[i].fd);
					sos[i].fd = -1;
				}
				continue;
			}

			if (sos[i].fd <= 0){
				sos[i].fd = mutil_sock_open(opts);
				if (sos[i].fd <= 0) {
					ERR_PRINT("open sock failed.");
					goto err1;
				}

				/* this sock is start after get file info(file size) so just do partial download */
				if (g_optss.opts_d.enStat == NS_DOWNLOAD_STAT_MUTIL_DONWLOAD) {
					sos[i].s = SO_S_SEND_HEADER_PART;
					sos[i].filelen = g_optss.opts_d.filesize;
				}
			}

			if (sos[i].s == SO_S_SEND_HEADER || sos[i].s == SO_S_SEND_HEADER_PART || n_act == RPT_ACT_UPLOAD){
				FD_SET(sos[i].fd, &fds_write);
			}else{
				if (n_act == RPT_ACT_DOWNLOAD){
					FD_SET(sos[i].fd, &fds_read);
				}else if (n_act == RPT_ACT_UPLOAD){
					FD_SET(sos[i].fd, &fds_write);
				}
			}
			maxsock = maxsock < sos[i].fd ? sos[i].fd : maxsock;
		}

		if (!sret){
			tmo.tv_usec = 1000 * 100;
		}

			
		sret = select((maxsock + 1), &fds_read, &fds_write, NULL, &tmo);
		//ERR_PRINT("fd change sret = %d\n", sret);
		if (sret < 0){
			ERR_PRINT("select error.");
			goto err1;
		}else if (sret == 0){
		}
			
		for (i = 0; i < SOCKET_MAX_NUM; i++){
			if (FD_ISSET(sos[i].fd, &fds_write)){
				if (n_act == RPT_ACT_DOWNLOAD){
					ret = do_download(&sos[i]);
					if (ret == NSPT_ERR_CONNABR || ret == NSPT_ERR_ERR){
						MSG_PRINT("Reconnect and redonwload .\n");
                        if (sos[i].fd){
                            close(sos[i].fd);
                        }
						if ((sos[i].fd = socket_open(opts->ipAddr, opts->port)) < 0){
						    ERR_PRINT("Connect %s:%d Failed!", opts->ipAddr, opts->port);
						}
                        /*
                           } else if (ret < 0){
                           ERR_PRINT("do download failed.\n");
                           goto err1;
                           */
                    } else if (ret == NSPT_ERR_DEAD) {
                        goto err1;
                    }
				}else if (n_act == RPT_ACT_UPLOAD){
					write_len = do_upload(&sos[i]);
					if (write_len < 0){
						ERR_PRINT("Send Upload http data failed.\n");
						goto err1;
					}
				}
			}
			
			if (FD_ISSET(sos[i].fd, &fds_read)){
				if (n_act == RPT_ACT_DOWNLOAD){
//__do_download:
					ret = do_download(&sos[i]);
					if (ret == NSPT_ERR_CONNABR || ret == NSPT_ERR_ERR){
						if (NS_GET_JUST_DONW_FILE(g_optss.flag)) {
							/* donwload file */
							//g_optss.flag != ;
							rst->last = RPT_ACT_DOWNLOAD_FILE_OVER;
							MSG_PRINT("Download Over .");
						} else {
							/* speed test */
							MSG_PRINT("Reconnect and redonwload .\n");
							if (sos[i].fd){
								close(sos[i].fd);
							}
							if ((sos[i].fd = socket_open(opts->ipAddr, opts->port)) < 0){
								ERR_PRINT("Connect %s:%d Failed!", opts->ipAddr, opts->port);
							}
						}
                    } else if (ret == NSPT_ERR_DEAD) {
                        goto err1;
                    }
					/* get file size success.and decide use how many thread to do it */
					if (sos[i].s == SO_S_SEND_HEADER_PART) {
						MSG_PRINT("part donwload again.\n");
						/* close old link */
						/*
						MSG_PRINT("Reconnect and part donwload .\n");
						if (sos[i].fd){
							close(sos[i].fd);
						}
						if ((sos[i].fd = socket_open(opts->ipAddr, opts->port)) < 0){
							ERR_PRINT("Connect %s:%d Failed!", opts->ipAddr, opts->port);
						}
						*/
						//TODO: why part download over. select whill not get event!!!
						//fix TODO: forget to FD_SET for SO_S_SEND_HEADER_PART
						//goto __do_download;
					}
				}else if (n_act == RPT_ACT_UPLOAD){
					/* now we should not run to here. */
					read_len = do_upload(&sos[i]);
					if (read_len < 0){
						ERR_PRINT("Recv Upload Response http failed.\n");
						goto err1;
					}
				}
			}

			pthread_mutex_lock(&rpt_mutex);
			if (ret != NSPT_ERR_ERR && n_act == RPT_ACT_DOWNLOAD){
				rst->rx_bytes += sos[i].recvll;
				sos[i].recvll = 0;
			}
			if (write_len &&  n_act == RPT_ACT_UPLOAD ){
				rst->tr_bytes += write_len;
			}

            if (NS_GET_JUST_DONW_FILE(g_optss.flag) && (rst->last == RPT_ACT_DOWNLOAD_FILE_OVER)){	
				err = 0;
				pthread_mutex_unlock(&rpt_mutex);
				goto over;
			}
			if ((rst->last != RPT_ACT_NONE) && (rst->last == rst->act)){
				rst->last  = RPT_ACT_NONE;
				rst->act   = RPT_ACT_NONE;
				err = 0;
				//MSG_PRINT("do last transmit.\n");
				pthread_mutex_unlock(&rpt_mutex);
				goto over;
			}
			pthread_mutex_unlock(&rpt_mutex);
			
		}
//flush_fd:
        //continue;
	}

over:
	MSG_PRINT("over.\n");

err1:
	for (i = 0; i < SOCKET_MAX_NUM; i++){
		socket_close(sos[i].fd);
	}
	
//err0:	
	if (err){
		pthread_mutex_lock(&rpt_mutex);
		rst->err = RPT_ACT_ERR;
		pthread_mutex_unlock(&rpt_mutex);
	}
	
	return err;
}

static void 
someone_killed_me(int signo){
	ERR_PRINT("O, someone kelled me!.\n");

	exit(0);
	return;
}

void ntsp_alrm(int sig)
{
    if (sig == SIGALRM) {
		ERR_PRINT("alrm time out(want %ds , Run %ds), speed will exit.\n",
                g_optss.duration, g_optss.duration * 2);
        exit(-1);
    }
    return;
}

#ifdef APP_NS_GET_INDEPENDENT
int ns_get_main(int argc, char *argv[])
#else
int ns_get_main(G_OPT_SS *pstOptss)
#endif
{
	int ret = 0;
	pthread_t pid;


	signal(SIGUSR1, someone_killed_me);

	signal(SIGPIPE,SIG_IGN); 
    //TODO: need a another way
#ifdef APP_NS_GET_INDEPENDENT
    signal(SIGALRM, ntsp_alrm);
#endif

	rst = malloc(sizeof(struct netspeedtest_rst));
	if (rst == NULL){
		ERR_PRINT("Malloc rst Failed.\n");
	}
	memset(rst, 0, sizeof(sizeof(struct netspeedtest_rst)));
	
	rst->act  = RPT_ACT_NONE;
	rst->last = RPT_ACT_NONE;
	rst->last = RPT_ACT_NONE;
	rst->rx_bytes = 0;
	rst->tr_bytes = 0;
	rst->rx_start = 0;
	rst->tr_start = 0;
	
#ifdef APP_NS_GET_INDEPENDENT
	if (opt_ckeck(argc, argv) != 0){
		ERR_PRINT("Opt check Failed!\n");
		goto err0;
	}
#else
	memcpy(&g_optss, pstOptss, sizeof(g_optss));
	
	if (opt_ckeck(0, NULL) != 0){
		ERR_PRINT("Opt check Failed!\n");
		goto err0;
	}
#endif
    if (g_optss.duration == 0) {
        g_optss.duration = 30000;
    }
    g_optss.opts_d.timerOut = g_optss.duration;
    g_optss.opts_u.timerOut = g_optss.duration;
    if (g_optss.period) {
        g_optss.period = 1;
    }
    g_optss.opts_d.period   = g_optss.period;
    g_optss.opts_u.period   = g_optss.period;

#ifdef APP_NS_GET_INDEPENDENT
    alarm(g_optss.duration * 2);
#endif

	pthread_create(&pid, NULL, get_speed_th, NULL);

	if (g_optss.flag & OPT_FLAG_DOWNLOAD_FILE){
		over = 0;

		if (g_optss.flag & OPT_FLAG_NO_TEST){
			if (create_open_donwload_file(&(g_optss.opts_d)) < 0) {
				goto err0;
			}
		}
		ret = ship(RPT_ACT_DOWNLOAD, &(g_optss.opts_d));
        if (ret < 0){
            ERR_PRINT("Download speed test error.\n");
            goto err0;
        }
		if (g_optss.flag & OPT_FLAG_NO_TEST){
			close_download_file(&(g_optss.opts_d));
		}
	} else {
        MSG_PRINT("Download speed test SUCCESS.");
    }
    

	if (g_optss.flag & OPT_FLAG_UP_FILE){
		over = 0;
		ret = ship(RPT_ACT_UPLOAD, &(g_optss.opts_u));
        if (ret < 0){
            ERR_PRINT("Upload speed test error.\n");
            goto err0;
        }
	}
    MSG_PRINT("Download speed test SUCCESS.");

	pthread_join(pid, NULL);

err0:	
	free(rst);
	close_download_file(&(g_optss.opts_d));
	return -1;
}


