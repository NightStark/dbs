#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_base.h>

#include <ns_trigger.h>


ULONG Trigger_thread_QueMsgRecv(IN INT iThrdId, 
		 						IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg, 
		 						IN QUEMSG_TRIGGER_FUN *pfQueMsgTFunList)
{
	DBGASSERT(NULL != pstThrdQueMsg);
	DBGASSERT(NULL != pfQueMsgTFunList);

	/* 可用AVL树去实现，加快挂接消息类型的处理 */

	if (NULL != pfQueMsgTFunList[pstThrdQueMsg->uiQueMsgType])
	pfQueMsgTFunList[]();
	

	return ERROR_SUCCESS;
}

