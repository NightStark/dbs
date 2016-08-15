#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ns_base.h>
#include <ns_websetting.h>
#include <ns_msg_client_init.h>

STATIC INT create_setting_http_list( IN CHAR *pcBuf, 
									 IN UINT  uiNo,
								     IN CHAR *pcPara,
								     IN CHAR *pcValue)
{
	INT iSprintLen = 0;

	iSprintLen = sprintf(pcBuf, 
	"<ul class=\"table_set_list_elment\">"
		"<li class=\"name_no\">%d</li>"			/* %d : NO. */
		"<li class=\"name_para\">%s</li>"		/* %s : 参数项 */
		"<li class=\"name_value\">"
			"<div class=\"set_value\">"
	    		"<input name=\"v_ver\" type=\"text\" value=\"%s\" maxlength=\"64\" readonly=\"readonly\">"	/* %s:参数值 */
	    	"</div>"
		"</li>"
	"</ul>",
	uiNo, pcPara, pcValue);

	return iSprintLen;
}

#define CSF_TMP_BUF_SIZE (64)
UINT Create_setting_ForHttp(CHAR *pcBuf)
{
	UINT  uiNo = 0;
	INT iSprintLen = 0;
	CHAR cTmpBuf[CSF_TMP_BUF_SIZE];
	
	WEB_SETTING_SYS_S *pstWebSetSys = NULL;

	DBGASSERT(NULL != pcBuf);

	pstWebSetSys = WebSetting_Get();
	DBGASSERT(NULL != pstWebSetSys);

	mem_set0(cTmpBuf, CSF_TMP_BUF_SIZE);
	sprintf(cTmpBuf, "%d.%d.%d", 	pstWebSetSys->uiWebMainVersion, 
									pstWebSetSys->uiWebSubVersion, 
									pstWebSetSys->uiWebStepVersion);
									
	iSprintLen += create_setting_http_list(pcBuf + iSprintLen, uiNo, "Web Server Version", cTmpBuf);
    uiNo++;
    
	iSprintLen += create_setting_http_list(pcBuf + iSprintLen, uiNo, "Web Server Name", pstWebSetSys->szWebServerName);
    uiNo++;
    
	iSprintLen += create_setting_http_list(pcBuf + iSprintLen, uiNo, "Web Server Start Time", ctime(&(pstWebSetSys->WebStartTime)));
    uiNo++;
    
	iSprintLen += create_setting_http_list(pcBuf + iSprintLen, uiNo, "Web Server Run Time", ctime(&(pstWebSetSys->WebRunTime)));
	
	
	return iSprintLen;
}

#define DATABASE_CONNECT_STATUS_DIS (0)
#define DATABASE_CONNECT_STATUS_CON (1)

ULONG g_ulDataBaseConnectStatus = DATABASE_CONNECT_STATUS_DIS;
UINT Create_control_ForHttp(CHAR *pcBuf, LONG lValue)
{
	INT iSprintLen = 0;

	if (1 == lValue)
	{
		if (DATABASE_CONNECT_STATUS_CON == g_ulDataBaseConnectStatus)
		{
			MSG_PRINTF("Is alreay Connection to Server!");
			iSprintLen = sprintf(pcBuf, "connecting [Is alreay Connection to Server]");
			goto exit_c_c_f;
		}
		
		DataBase_client_init();
		iSprintLen = sprintf(pcBuf, "connecting ...");
		g_ulDataBaseConnectStatus = DATABASE_CONNECT_STATUS_CON;
	}
	else
	{
		if (DATABASE_CONNECT_STATUS_DIS == g_ulDataBaseConnectStatus)
		{
			MSG_PRINTF("Is alreay DisConnection to Server!");
			iSprintLen = sprintf(pcBuf, "connecting [Is alreay DisConnection to Server]");
			goto exit_c_c_f;
		}
		iSprintLen = sprintf(pcBuf, "dis  connecting ...");
		DataBase_client_Fini();
		g_ulDataBaseConnectStatus = DATABASE_CONNECT_STATUS_DIS;
	}
	
exit_c_c_f:
	return iSprintLen;
}


