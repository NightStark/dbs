#ifndef __NS_GET_H__
#define __NS_GET_H__

/* ./t -d -D http://dl.quickbird.com/speedtest20M.zip  -U uploadtest.quickbird.com:60080 -f ./haozip_v2.8_tiny.exe -T 10 -p 1 */
#define FILE_PAK_LEN     (2048)
#define HTTP_BUF_SIZE    (1024)
#define URL_LEN			 (512)
#define FILE_NAME_LEN	 (512)

#define IP_ADDR_BUF_LEN	 (16)
#define HOSTNAME_BUF_LEN (512)
#define WEB_URL_BUF_LEN	 (512)
#define ERR_MSG_BUF_LEN	 (64)
#define NET_SPEED_PID_FILE	"/tmp/.net_speed_test.pid"

#define NSPT_ERR_OK      (0)
#define NSPT_ERR_EAGAIN  (-1)
#define NSPT_ERR_ERR     (-2)
#define NSPT_ERR_CONNABR (-3)
#define NSPT_ERR_DEAD    (-4)

typedef enum tag_ns_download_stat
{
	NS_DOWNLOAD_STAT_INIT = 0,
	
	NS_DOWNLOAD_STAT_GETING_FILE_INFO,
	NS_DOWNLOAD_STAT_GETTED_FILE_INFO,
	NS_DOWNLOAD_STAT_MUTIL_DONWLOAD,
	NS_DOWNLOAD_STAT_DONWLOAD_SUCCESS,
	NS_DOWNLOAD_STAT_DONWLOAD_FAILED,

	NS_DOWNLOAD_STAT_MAX,
}NS_DOWNLOAD_STAT_EN;

typedef struct tag_g_opt{
	char urlBuf[URL_LEN];
	char ipAddr[IP_ADDR_BUF_LEN];
	char hostName[HOSTNAME_BUF_LEN];
	char u_path[WEB_URL_BUF_LEN];
	int  flag;
	int  port;
	int  timerOut; /* transmit duration */
	int  period;   
	char filePath[FILE_NAME_LEN];
	int  filesize;
	int  recvTotalLen;
	int  filePos;
	FILE *fp;
	DCL_HEAD_S stMHead; /* muti-thread load block list */
	NS_DOWNLOAD_STAT_EN enStat;

	char err[ERR_MSG_BUF_LEN];
}G_OPT_S;

typedef struct tag_g_opss{
	unsigned int flag;
	int period; /* period to show download/upload info */
	int duration; /* max run time */
	char err[1024];
	int err_p;
	G_OPT_S opts_d;
	G_OPT_S opts_u;	
}G_OPT_SS;

enum rpt_act{
	RPT_ACT_ERR = -1,
	RPT_ACT_NONE = 0,
	RPT_ACT_DOWNLOAD,
	RPT_ACT_UPLOAD,
	RPT_ACT_NO_TEST,
	RPT_ACT_DOWNLOAD_FILE_OVER,
	RPT_ACT_MAX,
};

#define OPT_FLAG_UP_FILE 	   (0x01 << (RPT_ACT_DOWNLOAD))
#define OPT_FLAG_DOWNLOAD_FILE (0x01 << (RPT_ACT_UPLOAD))
#define OPT_FLAG_NO_TEST	   (0x01 << (RPT_ACT_NO_TEST))  /* no test will real donw a file */
#define SOCKET_MAX_NUM (32)
#define MUTI_DOWNLOAD_SOCKET_NUM (16)


int ns_get_main(G_OPT_SS *pstOptss);
int m_download_test(void);

#endif /* __NS_GET_H__ */
