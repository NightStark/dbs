/* 这个线程会监听socket并发送可以发送的数据。
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ns_base.h>
#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_log.h>
#include <ns_sm.h>
#include <ns_sm_client.h>
#include <ns_msg_client_link.h>
#include <ns_msg_server_link.h>
#include <ns_msg.h>

typedef union link_info 
{
    INT              iSockFd;
    MSG_SRV_LINK_ST *pstSrvLink;
    MSG_CLT_LINK_ST *pstMsgCltLink;
}LINK_INFO_UN;
typedef struct msg_send_info
{
	INT              iThreadID; /* thread id */
    LINK_INFO_UN     unLinkInfo; 
}MSG_SEND_INFO;
