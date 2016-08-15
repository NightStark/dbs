#ifndef __WEB_SETTING_H__
#define __WEB_SETTING_H__

typedef enum tag_PRUNTYPE
{
     PRUNTYPE_NORMAL,
     PRUNTYPE_DAEMON,
}PRUNTYPE_E;

typedef struct tag_WebSettingSystem
{
	UINT uiWebMainVersion;
	UINT uiWebSubVersion;
	UINT uiWebStepVersion;
	CHAR szWebServerName[64];
	time_t WebStartTime;
	time_t WebRunTime;

	UCHAR ucPRunType; 	/* 进程是否以守护进程方式运行 : PRUNTYPE_E */
}WEB_SETTING_SYS_S;

ULONG WebSetting_Init(PRUNTYPE_E enPRunType);
WEB_SETTING_SYS_S *WebSetting_Get(VOID);
VOID WEB_SETTING_PRunType(PRUNTYPE_E enPRunType);

UINT Create_setting_ForHttp(CHAR *pcBuf);
UINT Create_control_ForHttp(CHAR *pcBuf, LONG lValue);

#endif //__WEB_SETTING_H__
