#ifndef __CLIENT_SEND_C__
#define __CLIENT_SEND_C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <ns_base.h>
#include <ns_table.h>
#include <ns_opdata.h>

ULONG CLIRNT_msg_send(INT iSockFd, VOID *pBuf, SIZE_T ssSendBuflen)
{
	SSIZE_T ssSentLen = 0;

	DBGASSERT(NULL != pBuf);

	ssSentLen = send(iSockFd, pBuf, ssSendBuflen, 0);
	
	DBG_PRINT_LOG("MSG.bat", pBuf, ssSentLen);
	MSG_PRINTF("client buf send len %d", ssSendBuflen);
	MSG_PRINTF("client msg send len %d", ssSentLen);
	
	if (-1 == ssSentLen || ssSentLen != ssSendBuflen)
	{
		return ERROR_FAILE;
	}
	else
	{
		return ERROR_SUCCESS;
	}
}


#endif //__CLIENT_SEND_C__

