//this way not good will use xml or json and socket
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ns_base.h>
#include <ns_config.h>

typedef enum tag_cfg_index
{
	CFG_INDEX_LOCAL_IP,
	CFG_INDEX_SERVER_IP,
	CFG_INDEX_SERVER_PORT,
	CFG_INDEX_BIND_IP,

	CFG_INDEX_MAX,
}CFG_INDEX_EN;

typedef enum tag_cfg_itype
{
	CFG_ITYPE_INT,
	CFG_ITYPE_STRING,
	CFG_ITYPE_BOOL,

	CFG_ITYPE_MAX,
}CFG_ITYPE_EN;

typedef struct tag_cfg_entry
{
	CFG_INDEX_EN enCfgIndex;	
	const char  *pcCfgName;
	CFG_ITYPE_EN enCfgIType;
	INT          iSeted; /* = 1:use set, = 0: use default */
}CFG_ENTRY_ST;

typedef struct tag_cfg
{
	UINT uiLocalIPAddr;
	UINT uiSrvIPAddr;
	UINT uiSrvPort;
	UINT uiBindIPAddr;

	pthread_mutex_t cfg_mutex;
}CFG_ST;

STATIC CFG_ST g_cfg;

#define _CFG_LK \
	do {	\
		pthread_mutex_unlock(&(g_cfg.cfg_mutex)); \
	}while(0)
#define _CFG_UNLK \
	do {	\
		pthread_mutex_lock(&(g_cfg.cfg_mutex)); \
	}while(0)

//TODO:can add :1,para check condition. and so on
STATIC CFG_ENTRY_ST g_cfg_entry_list[CFG_INDEX_MAX] = {
	{CFG_INDEX_LOCAL_IP,	"local_ip",		CFG_ITYPE_STRING},
	{CFG_INDEX_SERVER_IP,	"server_ip",	CFG_ITYPE_STRING},
	{CFG_INDEX_SERVER_PORT,	"server_port",	CFG_ITYPE_STRING},
	{CFG_INDEX_BIND_IP,		"bind_ip",		CFG_ITYPE_STRING},
};

STATIC INT cfg_get_index_by_name(IN const CHAR *pcCfgName)
{
	INT iIndex = 0;
	for (iIndex = 0; iIndex < ARRAY_SIZE(g_cfg_entry_list); iIndex++) {
		if (strncmp(pcCfgName, g_cfg_entry_list[iIndex].pcCfgName, sizeof(g_cfg_entry_list[iIndex].pcCfgName))) {
			return iIndex;
		}
	}

	return -1;
}

ULONG cfg_set(IN const CHAR *pcCfgName, IN const CHAR *pcCfgValue, CFG_INDEX_EN enCfgIndex)
{
	INT i = -1;

	i = cfg_get_index_by_name(pcCfgName);
	if (i < 0) {
		ERR_PRINTF("invalid cfg : %s", pcCfgName);
		return ERROR_FAILE;
	}

	switch (g_cfg_entry_list[i].enCfgIndex) {
		case CFG_INDEX_LOCAL_IP:
			g_cfg.uiLocalIPAddr = tc_str2ip(pcCfgValue);
		case CFG_INDEX_SERVER_IP:
			g_cfg.uiSrvIPAddr = tc_str2ip(pcCfgValue);
		case CFG_INDEX_SERVER_PORT:
			g_cfg.uiSrvPort = atoi(pcCfgValue);
			break;
		default:
			ERR_PRINTF("not support type %d, now", g_cfg_entry_list[i].enCfgIndex);
	}

	return ERROR_SUCCESS;
}

ULONG cfg_line_parse(INOUT CHAR *pcLine, IN INT iBufLen, IN INT iLineNo)
{
	CHAR *pcCfgName = NULL;
	CHAR *pcCfgValue = NULL;

	pcCfgValue = strstr(pcLine, "=");
	if (NULL == pcCfgValue) {
		ERR_PRINTF("invalid format, in line %d : %s", iLineNo, pcLine);
		return ERROR_FAILE;
	}
	*pcCfgValue = '\0';
	pcCfgValue ++;

	pcCfgName = pcLine;
	pcCfgName  = stripwhite(pcCfgName);
	pcCfgValue = stripwhite(pcCfgValue);

	MSG_PRINTF("%s = %s", pcCfgName, pcCfgValue);

	cfg_set(pcCfgName, pcCfgValue, 0);

	return ERROR_SUCCESS;
}


ULONG cfg_read_cfg_file(const char *path)
{
	FILE *fp = NULL;
	INT  iLineNo = 0;
	CHAR aucBuf[1024];

	fp = fopen(path, "r");
	if (fp == NULL) {
		ERR_PRINTF("open cfg file[%s] failed", path);
		return ERROR_FAILE;
	}

	while (fgets(aucBuf, sizeof(aucBuf), fp)) {
		iLineNo ++;
		if (ERROR_FAILE == cfg_line_parse(aucBuf, sizeof(aucBuf), iLineNo)) {
			goto err_get;
		}
	}

	fclose(fp);
	fp = NULL;

	return ERROR_SUCCESS;
err_get:
	fclose(fp);
	fp = NULL;
	return ERROR_FAILE;
}

INT cfg_init(VOID)
{
	pthread_mutex_init(&(g_cfg.cfg_mutex), NULL);

	MSG_PRINTF("start read config file.");
	if (ERROR_FAILE == cfg_read_cfg_file("ns.cfg")) {
		ERR_PRINTF("read config file failed.");
		return ERROR_FAILE;
	}
	MSG_PRINTF("read config file success.");

    return ERROR_SUCCESS;
}

/**/
UINT cfg_get_remote_ipaddr(void) 
{
	UINT ipaddr = 0;
	_CFG_LK;
	ipaddr = g_cfg.uiSrvIPAddr;
	_CFG_UNLK;

	return ipaddr;
}

UINT cfg_get_remote_port(void) 
{
	UINT iport = 0;
	_CFG_LK;
	iport = g_cfg.uiSrvPort;
	_CFG_UNLK;

	return iport;
}
