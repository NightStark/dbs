#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/lib/ns_lilist.h"
#include "include/lib/ns_type.h"

typedef struct ta
{
	unsigned int id;
	unsigned int index;
	DCL_NODE_S stNode;
}TA_S;

DCL_HEAD_S stHead = {};
DTQ_TAIL_S stTail = {};

int add(DCL_HEAD_S *pstHead, TA_S *pstTa)
{
	DCL_NODE_S *pstLiliNode = NULL;
	TA_S *pstTaEntry = NULL;

	DCL_FOREACH_ENTRY(pstHead,pstTaEntry,stNode)
	{
		if (pstTa->id < pstTaEntry->id)
		{
			break;
		}
	}

	if(NULL == pstTaEntry)
	{
		DCL_AddTail(pstHead, &pstTa->stNode);
	}
	else
	{
		DCL_AddBefore(&pstTaEntry->stNode,&pstTa->stNode);
	}
}

TA_S * find(DCL_HEAD_S *pstHead)
{
	DTQ_NODE_S *pstLiliNode = NULL;
	TA_S *pstTaEntry = NULL;

	DCL_FOREACH_ENTRY(pstHead,pstTaEntry,stNode)
	{
		if (55 == pstTaEntry->id)
		{
			break;
		}
	}

	return pstTaEntry;
}

int show(DCL_HEAD_S *pstHead)
{
	TA_S *pstTa  = NULL;
	DTQ_NODE_S *pstLiliNode = NULL;
	int i = 0;

	DCL_FOREACH_ENTRY(pstHead,pstTa,stNode)
	{
		printf("[%3d]--%3d--[%3d]--\n",i++, pstTa->id,pstTa->index);
		
	}
}

int main(void)
{
	int i = 0;
	TA_S *pstTa  = NULL;
	TA_S stTa[100];
	DCL_HEAD_S *pstHead = &stHead;
	DTQ_TAIL_S *pstTail = &stTail;

	DCL_NODE_S *pstLiliNode = NULL;
	
	DCL_Init(&stHead);
	srand(time(0));
	for(i = 0; i < 100; i++)
	{
		stTa[i].id = i;
		stTa[i].index= i;
		add(pstHead, &stTa[i]);
		//LILI_AddTail(&stTail, &stTa[i].stNode);
	}

	pstTa = find(pstHead);
	//show(pstHead, pstTail);

	
	for(i = 0; i < 788; i++)
	{
		DCL_DelLast(pstHead, NULL);
	}
	
	/*	LILI_DelAfter(&(pstTa->stNode), NULL);
	LILI_DelAfter(&(pstTa->stNode), NULL);
	LILI_DelAfter(&(pstTa->stNode), NULL);
	LILI_DelAfter(&(pstTa->stNode), NULL);
	LILI_DelAfter(&(pstTa->stNode), NULL);

	
	LILI_DelBefore(&(pstTa->stNode), NULL);
	LILI_DelBefore(&(pstTa->stNode), NULL);
	LILI_DelBefore(&(pstTa->stNode), NULL);
	LILI_DelBefore(&(pstTa->stNode), NULL);
	LILI_DelBefore(&(pstTa->stNode), NULL);
	*/

	
	show(pstHead);
	
	DCL_Fint(&stHead);
	
	return 0;
}
