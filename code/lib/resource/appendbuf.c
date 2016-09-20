#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <ns_base.h>
#include <ns_ab.h>

DCL_HEAD_S g_stABListHead;

ULONG AB_Init(VOID)
{
	DCL_Init(&g_stABListHead);

	return ERROR_SUCCESS;
}

ULONG AB_AppendString(AB_INFO_S *pstAB, const CHAR *pcFmt, ...)
{
	INT iSptLen;
	va_list args;

	va_start(args, pcFmt);
	
	iSptLen = vsprintf((CHAR *)(pstAB->pAB + pstAB->ulABCurrentEnd), pcFmt, args);
	pstAB->ulABCurrentEnd += iSptLen;
	if (pstAB->ulABCurrentEnd > pstAB->ulABLen)
	{
		ERR_PRINTF("Append Failed!");
		return ERROR_FAILE;
	}

	va_end(args);
	
	return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : AB_Create
 Description  : 创建 append 资源
 Input        : ULONG ulNABlen  缓冲长度
                VOID *pNAB      缓冲开始
 Output       : None
 Return Value : AB_INFO_S
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/30
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
AB_INFO_S *AB_Create(ULONG ulNABlen, VOID *pNAB)
{
	AB_INFO_S *pstAB;

	pstAB = (AB_INFO_S *)mem_alloc(sizeof(AB_INFO_S));
	if (NULL == pstAB)
	{
		return NULL;
	}

	pstAB->pAB = pNAB;
	pstAB->ulABLen = ulNABlen;
	pstAB->ulABCurrentEnd = 0;
	//pstAB->pfAB_AppendString = AB_AppendString;

	DCL_AddHead(&g_stABListHead, &(pstAB->stNode));

	return pstAB;
}

VOID AB_Destroy(AB_INFO_S *pstAB)
{
	DBGASSERT(NULL != pstAB);

	DCL_Del(&(pstAB->stNode));

	free(pstAB);
	
	return;
}



