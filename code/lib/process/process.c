#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ns_base.h>
#include <ns_process.h>

/*****************************************************************************
 Prototype    : P_GetPID
 Description  : 获取某个进程的PID
 Input        : const CHAR *ProcessName  进程的名字
 Output       : None
 Return Value : INT 进程ID
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/10/6
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
INT P_GetPID(const CHAR *ProcessName)
{
	CHAR  *pP      = NULL;
	INT   iGetPid  = 0;
	FILE *fpOnly   = NULL;
	CHAR  acBuf[DBG_PROC_ONLY_BUF_SIZE] = {0};

	/*
	INT   iRetLen  = 0;
	CHAR  ucPidBuf[GET_PID_BUF_LEN];
	CHAR  ucPNameBuf[GET_PNAME_BUF_LEN];
	memset(ucPNameBuf, 0 ,sizeof(GET_PNAME_BUF_LEN));
	sprintf(ucPNameBuf, "pgrep %s", ProcessName);
	fpPGrep = popen(ucPNameBuf, "r");
	if (NULL == fpPGrep)
	{
		ERR_PRINTF("popen command [%d] Failed!", ucPNameBuf);
		
		return -1;
	}
	
	memset(ucPidBuf, 0 ,sizeof(GET_PID_BUF_LEN));
	iRetLen = fread(ucPidBuf, sizeof(CHAR), GET_PID_BUF_LEN, fpPGrep);
	if (iRetLen == 0)
	{
		ERR_PRINTF("read debug PID Failed!");
		return -1;
	}
	*/

	fpOnly = fopen(DBG_PROC_ONLY_FILE, "r");
	if (NULL == fpOnly){
		ERR_PRINTF("open debug PID file Failed!, Maybe Debug is not start.");
		return -1;
	}

	if (!fgets(acBuf, sizeof(acBuf), fpOnly)){
		fclose(fpOnly);
		ERR_PRINTF("read debug PID file Failed!, Maybe Debug is not start.");
		return -1;
	}

	pP = strstr(acBuf, ":");
	if (NULL == pP){
		fclose(fpOnly);
		ERR_PRINTF("check debug PID file Failed!, Maybe Debug is not start.");
		return -1;
	}

	iGetPid = atoi(pP + 1);
	
	MSG_PRINTF("PID of %s = %d", ProcessName, iGetPid);

	fclose(fpOnly);
	return iGetPid;
}

