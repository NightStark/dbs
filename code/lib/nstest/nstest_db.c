/* 待升级为 树实现 为多级，现在一级 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>

#include <ns_test.h>

#define NSTEST_ELE_NAME_MAX_LEN (32)

DCL_HEAD_S g_stNSTestDBHead = {0};

typedef struct tag_NSTestEleInfo
{
	UCHAR 		szNSTestEleName[NSTEST_ELE_NAME_MAX_LEN];
	pfNSTestEle pfNSTestEleCallBack;
	VOID 	   *arg;
	
	DCL_NODE_S  stNode;
}NSTEST_ELE;

ULONG NSTEST_Init(void)
{
	DCL_Init(&g_stNSTestDBHead);

	return ERROR_SUCCESS;
}

VOID NSTEST_Fint(VOID)
{
	DCL_Fint(&g_stNSTestDBHead);

	return;
}

ULONG NSTEST_CreateNSTestEle(IN const UCHAR *szName, 
									 IN pfNSTestEle pf,
									 IN VOID *arg)
{
	NSTEST_ELE *pstNsTestNode = NULL;

	DBGASSERT(NULL != szName);
	DBGASSERT(NULL != pf);

	pstNsTestNode = (NSTEST_ELE *)mem_alloc(sizeof(NSTEST_ELE));
	if (NULL == pstNsTestNode)
	{
		return ERROR_SUCCESS;
	}

	str_safe_cpy(pstNsTestNode->szNSTestEleName,
				 szName,
				 sizeof(pstNsTestNode->szNSTestEleName));

	pstNsTestNode->pfNSTestEleCallBack = pf;
	pstNsTestNode->arg = arg;

	DCL_AddTail(&g_stNSTestDBHead, &(pstNsTestNode->stNode));

	return ERROR_SUCCESS;
}

ULONG NSTEST_Reg(IN const UCHAR *szName, 
 					  IN pfNSTestEle pf,
 					  IN VOID *arg)
{
	ULONG ulRet;
	
	ulRet = NSTEST_CreateNSTestEle(szName,pf,arg);

	return ulRet;
}

ULONG NSTEST_Run(VOID)
{
	ULONG ulRet;
	NSTEST_ELE *pstNsTestNode = NULL;

	DCL_FOREACH_ENTRY(&g_stNSTestDBHead,pstNsTestNode,stNode)
	{
		if (NULL != pstNsTestNode->pfNSTestEleCallBack)
		{
			printf("\033[0;31m[ * Start %s *]\033[0m \n", 
					pstNsTestNode->szNSTestEleName + 8);
			
			pstNsTestNode->pfNSTestEleCallBack(pstNsTestNode->arg);

			printf("\033[0;31m[ * End %s * ]\033[0m \n", 
					pstNsTestNode->szNSTestEleName + 8);
		}
	}

	return ulRet;
}

VOID NSTEST_DestroyNSTestEleALL(VOID)
{
	NSTEST_ELE *pstNsTestNode = NULL;

	DCL_FOREACH_ENTRY(&g_stNSTestDBHead,pstNsTestNode,stNode)
	{
		DCL_Del(&(pstNsTestNode->stNode));

		free(pstNsTestNode);
	}
}


ULONG NSTEST_ReleaseAll(VOID)
{
	NSTEST_DestroyNSTestEleALL();
}


