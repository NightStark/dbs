#ifndef __MSG_SEND_C__
#define __MSG_SEND_C__

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

ULONG MSG_normal_send(INT iSockFd, VOID *pBuf, SIZE_T ssSendBuflen)
{
	SSIZE_T ssSentLen = 0;

	DBGASSERT(NULL != pBuf);

	ssSentLen = send(iSockFd, pBuf, ssSendBuflen, 0);
	if (-1 == ssSentLen || ssSentLen != ssSendBuflen)
	{
		return ERROR_FAILE;
	}
	else
	{
		return ERROR_SUCCESS;
	}
}

INT MSG_SSL_send(IN SSL *pstSSL, IN VOID *pBuf, IN SIZE_T ssBufLen)
{
	int iLen = 0;
	iLen = SSL_write(pstSSL, pBuf, ssBufLen); 
	if (iLen <= 0) {
		ERR_PRINTF("SSL writer Failed,errno=%d, errmsg='%s'\n",
				errno, strerror(errno));
	} 

	return iLen;
}

INT SERVER_MSG_send(IN MSG_SRV_LINK_ST *pstSrvLink, IN VOID *pBuf, IN SIZE_T ssSendBuflen)
{
	UINT uiRet = -1;

	DBGASSERT(NULL != pstSrvLink);

	if (pstSrvLink->pstSSL != NULL) {
		uiRet = MSG_SSL_send(pstSrvLink->pstSSL, pBuf, ssSendBuflen);	
	}

	return uiRet;
}

INT CLIENT_MSG_send(IN MSG_CLT_LINK_ST  *pstCltLink, IN VOID *pBuf, IN SIZE_T ssSendBuflen)
{
	INT iRet = -1;

	DBGASSERT(NULL != pstCltLink);

	if (pstCltLink->pstSSL != NULL) {
		iRet = MSG_SSL_send(pstCltLink->pstSSL, pBuf, ssSendBuflen);	
	}

	if (iRet != ssSendBuflen) {
		ERR_PRINTF("Client send msg error.");
	}

	return iRet;
}

#endif //__MSG_SEND_C__

