#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ns_base.h>
#include <ns_websetting.h>


WEB_SETTING_SYS_S g_stWebSetSys;

ULONG WebSetting_Init(PRUNTYPE_E enPRunType)
{
	CLEAR_STRUCT(g_stWebSetSys);

	g_stWebSetSys.uiWebMainVersion = 0; 
	g_stWebSetSys.uiWebMainVersion = 2;
	g_stWebSetSys.uiWebSubVersion  = 3;
	g_stWebSetSys.ucPRunType       = enPRunType;

	sprintf(g_stWebSetSys.szWebServerName, "Night Start Emb Server.");

	g_stWebSetSys.WebStartTime = time(0);

	return ERROR_SUCCESS;
}

WEB_SETTING_SYS_S *WebSetting_Get(VOID)
{
	g_stWebSetSys.WebRunTime = time(0);
	return &g_stWebSetSys;
}

VOID WEB_SETTING_PRunType(PRUNTYPE_E enPRunType)
{
	g_stWebSetSys.ucPRunType = enPRunType;
	
	return;
}

