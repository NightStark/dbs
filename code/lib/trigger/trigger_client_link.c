#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_base.h>

#include <ns_trigger.h>

VOID quemsg_trigeer_cl_AddData(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	

	return;
}

QUEMSG_TRIGGER_FUN g_TriggerClientLinkList[THREAD_QUEMSG_CLIENT_LINK_TYPE_MAX] = 
{
	[TQ_C_LK_TYPE_ADD_DATA] = quemsg_trigeer_cl_AddData,
}

QUEMSG_TRIGGER_FUN g_TriggerActionList[] = 
{
	[] = quemsg_trigeer_cl_AddData,
}

THREAD_TYPE_MAIN_SERVER,
THREAD_TYPE_WORK_SERVER,
THREAD_TYPE_WORK_DISPATCH,

THREAD_TYPE_LINK,
THREAD_TYPE_WORK,
THREAD_TYPE_WEB_SERVER,
THREAD_TYPE_WEB_SERVER_WROK,

THREAD_TYPE_WEB_ACTION,


