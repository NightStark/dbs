#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>

#include <ns_test.h>

VOID ns_test_printf(ULONG ulPV)
{
	if (ERROR_FAILE == ulPV)
	{
		printf("\033[1m\033[0;31m[ FAILED  ]\033[0m\n");
	}
	else if (ERROR_SUCCESS == ulPV)
	{
		printf("\033[1m\033[0;32m[ SUCCESS ]\033[0m\n");
	}
	else
	{
		printf("\033[0;32m[ fuck what U want Print! ]\033[0m\n");
	}

	return;
}


CHAR g_acTlistSymbol[128][64] = {0};

STATIC BOOL_T ns_test_IsRepeat(const CHAR *c, int len)
{
	int i = 0;
	for(i = 0; i < 128; i++)
	{
		if (0 == mem_safe_cmp(g_acTlistSymbol[i], c, len))
		{
			return BOOL_TRUE;
		}
	}

	return BOOL_FALSE;
}

STATIC UINT ns_test_ParseTlist(CHAR *c, int len)
{
	INT iP = 0;
	INT i = 1;
	CHAR acTlistTag[] = "ns_TEST";
	CHAR aucTistEnd[4] = {0};
	for(i = 1; -1 != iP; i++)
	while(1)
	{
		iP = mem_pois_find(c, len, acTlistTag, strlen(acTlistTag),i);
		if (-1 == iP)
		{
			break;
		}

		if (ns_test_IsRepeat(c + iP, strlen(c + iP)))
		{
			break;
		}
		sprintf(g_acTlistSymbol[i - 1], c + iP);
	}

	return i;
}

VOID* ns_test_RegTList(VOID)
{
	int i = 1;
	VOID *pDLT = NULL; 
	ULONG ulRet;

	pfNSTestEle pfNSTestTTmp = NULL;

	pDLT = dlopen("libTestT.so", RTLD_NOW);
	if (NULL == pDLT)
	{
		return;
	}
	
	i = 1;
	while(1)
	{
		if (*g_acTlistSymbol[i - 1] == 0)
		{
			break;
		}
		
		pfNSTestTTmp = (pfNSTestEle)dlsym(pDLT, g_acTlistSymbol[i - 1]);
		if (NULL == pfNSTestTTmp)
		{
			ERR_PRINTF("NSTEST Reg Failed At Function:[ %s() ]", g_acTlistSymbol[i - 1]);
			continue;
		}

		ulRet = NSTEST_Reg(g_acTlistSymbol[i - 1],pfNSTestTTmp,(VOID *)i);
		if(ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSTEST Reg Failed At Function:[ %s() ]", g_acTlistSymbol[i - 1]);
			return NULL;
		}

		i++;
	}

	return pDLT;
}

VOID ns_test_UnRegTList(VOID *pDLT)
{
	dlclose(pDLT);

	return;
}

VOID ns_test_getTList(VOID)
{
	INT iTestCnt = 0;
	INT iTListFd = 0;
	INT iTLsttStatFd = 0;
	INT iReadLen = 0;
	struct stat stTListStat = {0};
	CHAR *cTListBuf = NULL;
	
	iTListFd = open("libTestT.so", 0666);
	if (-1 == iTListFd)
	{
		MSG_PRINTF("Open libTestT.so Failed!");
		return;
	}
	iTLsttStatFd = fstat(iTListFd, &stTListStat);
	if (-1 == iTLsttStatFd)
	{
		MSG_PRINTF("Get libTestT.so File Stat Failed!");
		return;
	}

	cTListBuf = mem_alloc(stTListStat.st_size);
	if (NULL == cTListBuf)
	{
		MSG_PRINTF("Mem_alloc Failed!");
		return;
	}

	iReadLen = read(iTListFd, cTListBuf, stTListStat.st_size);
	if (-1 == iReadLen)
	{
		MSG_PRINTF("Read .so file Failed!");
		return;
	}

	close(iTListFd);
	
	iTestCnt = ns_test_ParseTlist(cTListBuf,iReadLen);
	MSG_PRINTF("Find Test Count: %d", iTestCnt);
	
	free(cTListBuf);
	
	return;
}
