#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>
#include <ns_web_server.h>
#include <ns_thread.h>
#include <ns_web_action.h>

STATIC CHAR const *g_WebActionIdInfoList[]={
	[WEB_ACTION_ID_SYSINTO] = "sys_info",
};

STATIC inline UINT web_action_DecInfoList(IN CHAR const *pcInfoList[], 
	 									  IN CHAR *pcCkBuf,
	 									  IN UINT uiINone,
	 									  IN UINT uiIMax)
{
	INT  iRet = 0;
	UINT uiIndex;
	for (uiIndex = uiINone + 1; uiIndex < uiIMax; uiIndex++)
	{
		iRet = str_safe_cmp(pcInfoList[uiIndex],
	   						pcCkBuf,
	   						strlen(pcInfoList[uiIndex]) + 1);
	   	if (0 == iRet)
	   	{
			return uiIndex;
	   	}
	}
	return uiINone;
}

UINT WEB_action_GetActionIdFromStr(IN CHAR *pcCkBuf)
{
	UINT uiActionId;
	uiActionId = web_action_DecInfoList(g_WebActionIdInfoList,
 									    pcCkBuf,
 									    WEB_ACTION_ID_NONE,
 									    WEB_ACTION_ID_MAX);

	if (uiActionId >= WEB_ACTION_ID_NONE)
	{
		ERR_PRINTF("Action Id is invalid!");
		return -1;
	}

	return uiActionId;
}


