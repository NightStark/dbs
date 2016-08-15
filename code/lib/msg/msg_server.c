#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>

ULONG MSG_server_AskServerResp(IN INT iClientfd, IN UINT uiRespCode)
{
	ULONG ulRet = 0;	
	UINT  uiMsgLen = 0;
	CHAR  aucDataBuf[NSDB_MSG_LEN_MAX];
	MSG_SERVER_RESP_CODE_S stServerRespCode;

	mem_set0(aucDataBuf, NSDB_MSG_LEN_MAX);
	mem_set0(&stServerRespCode, sizeof(MSG_SERVER_RESP_CODE_S));

	stServerRespCode.uiRespCode = uiRespCode;
	
	uiMsgLen = MSG_ec_EncMsg( MSG_TYPE_NSDB_ASK_SERVICE_RESP,
							  MSG_DATA_TYPE_ASK_SERVER_RESP_CODE,
							  &stServerRespCode,
							  sizeof(stServerRespCode),
							  aucDataBuf);

	ulRet = send(iClientfd, aucDataBuf, uiMsgLen, 0);
	
	if(-1 == ulRet)
	{
		ERR_PRINTF("sent error!");

		return ERROR_FAILE;
	}
	
	MSG_PRINTF("send len %d", uiMsgLen);
	MSG_PRINTF("msg len %lu", ulRet);
	DBG_PRINT_LOG("MSG_AskServerResp.bat", aucDataBuf, ulRet);

	return ERROR_SUCCESS;
}


