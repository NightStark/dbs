/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : bitmap.c
  Version       : Initial Draft
  Author        : Night Stark [lyj051031448@163.com]
  Created       : 2013/12/28
  Last Modified :
  Description   : bitmap 纯C语言实现
  Function List :
  History       :
  1.Date        : 2013/12/28
    Author      : Night Stark [lyj051031448@163.com]
    Modification: Created file
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>
#include <ns_bitmap.h>

STATIC DCL_HEAD_S g_astBitMapListHead = {};

STATIC LONG g_acBitWid[5] = {0x00, 0xFF , /* 0xFFFF, 0xFFFFFF, 0xFFFFFFFF */};
STATIC CHAR g_acBitSetMap[8] = {
	0x1,      0x3,      0x7,      0xF,      0x1F,      0x3F,      0x7F,      0xFF,
	/* 可扩展至以LONG型记录 
	0x1FF,    0x3FF,    0x7FF,    0xFFF,    0x1FFF,    0x3FFF,    0x7FFF,    0xFFFF,
	0x1FFFF,  0x3FFFF,  0x7FFFF,  0xFFFFF,  0x1FFFFF,  0x3FFFFF,  0x7FFFFF,  0xFFFFFF,
	0x1FFFFFF,0x3FFFFFF,0x7FFFFFF,0xFFFFFFF,0x1FFFFFFF,0x3FFFFFFF,0x7FFFFFFF,0xFFFFFFFF,
	*/ };

typedef struct tag_BitMapInfo
{
	LONG   lBitMapID;			/* 系统时间 */
	UINT  uiBitMapSize;    /* 以位记录 */
	CHAR *pcBitMapList;	   /* 以字节映像 */
	DCL_NODE_S stNode;
}BIT_MAP_INFO_S;

STATIC BIT_MAP_INFO_S * find_bitmapinfo_node(LONG lBitMapID)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;

	DCL_FOREACH_ENTRY(&(g_astBitMapListHead), pstBitMapinfoNode, stNode)
	{
		if (lBitMapID == pstBitMapinfoNode->lBitMapID)
		{
			break;
		}
	}

	return pstBitMapinfoNode;
}

STATIC BIT_MAP_INFO_S * alloc_bitmapinfo_node(UINT uiMapSize)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;

	/* create node */
	pstBitMapinfoNode = mem_alloc(sizeof(BIT_MAP_INFO_S));
	if (NULL == pstBitMapinfoNode)
	{
		return NULL;
	}
	
	pstBitMapinfoNode->lBitMapID = time(NULL);
	pstBitMapinfoNode->uiBitMapSize = uiMapSize;
	pstBitMapinfoNode->pcBitMapList = mem_alloc((uiMapSize / 8) + 1);
	if (NULL == pstBitMapinfoNode->pcBitMapList)
	{
		free(pstBitMapinfoNode);
		return NULL;
	}

	return  pstBitMapinfoNode;
}

STATIC VOID free_bitmapinfo_node(BIT_MAP_INFO_S *pstBitMapinfoNode)
{	
	DBGASSERT(NULL != pstBitMapinfoNode);
		
	free(pstBitMapinfoNode->pcBitMapList);
	free(pstBitMapinfoNode);

	return;
}

LONG CreateBitMap(UINT uiMapSize)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;

	/* create node */
	pstBitMapinfoNode =alloc_bitmapinfo_node(uiMapSize);
	if (NULL == pstBitMapinfoNode)
	{
		return BIT_MAP_INVALID_ID;
	}
	
	DCL_AddHead(&g_astBitMapListHead, &pstBitMapinfoNode->stNode);

	return pstBitMapinfoNode->lBitMapID;
}

VOID DestroyBitMap(LONG lBitMapID)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return;
	}

	DCL_Del(&pstBitMapinfoNode->stNode);

	free_bitmapinfo_node(pstBitMapinfoNode);

	return;
}


/*
	  	-------------->XPos
	  	0 1 2 3 4 5 6 7
	| 0 . . . . . . . .
	| 1 . . . . . . . .
	| 2 . . . . . . . .
	| 3 . . . . . . . .
	| 4 . . . . . . . .
	V 5 . . . . . . . .
   YPos	  .
	  	  .
	       .
*/

#define get_bit_xyPos(uiPos, uiXPos, uiYPos)	\
			uiXPos = (uiPos % 8);				\
			uiYPos = (uiPos / 8);


ULONG BitMap_SetBit(LONG lBitMapID, UINT uiStartPos, UINT uiEndPos)
{
	UINT uiSXPos = 0;
	UINT uiSYPos = 0;
	UINT uiEXPos = 0;
	UINT uiEYPos = 0;

	CHAR *pcSLine = NULL;
	CHAR *pcELine = NULL;
	CHAR *pcPLine = NULL;

	UINT uiIndex = 0;
	
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return ERROR_BIMAP_NOT_EXIST;
	}

	if (uiStartPos < 0 || uiEndPos >= pstBitMapinfoNode->uiBitMapSize)
	{
		return ERROR_BIMAP_OUTOF_RANGE;
	}

	get_bit_xyPos(uiStartPos, uiSXPos, uiSYPos);
	get_bit_xyPos(uiEndPos,   uiEXPos, uiEYPos);

	pcSLine = pstBitMapinfoNode->pcBitMapList + uiSYPos;
	pcELine = pstBitMapinfoNode->pcBitMapList + uiEYPos;

	if (uiSYPos == uiEYPos)
	{
		/* 在一行 */
		(*pcSLine) |= (g_acBitSetMap[uiEXPos - uiSXPos] & g_acBitWid[1]) << uiSXPos;
	}
	else
	{
		/* 多于一行时 */
		/* 首行 */
		(*pcSLine) |= g_acBitSetMap[8 - uiSXPos - 1] << uiSXPos;	
		/* 中间行(==2时不执行) */
		for (uiIndex = 1; uiIndex < (uiEYPos - uiSYPos); uiIndex++)
		{
			pcPLine = pcSLine + uiIndex;
			*(pcPLine) |= (g_acBitSetMap[7] & g_acBitWid[1]);
		}
		/* 尾行 */
		(*pcELine) |= (g_acBitSetMap[uiEXPos] & g_acBitWid[1]);
	}

	return ERROR_SUCCESS;
}

ULONG BitMap_SetBitALL(LONG lBitMapID)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return ERROR_BIMAP_NOT_EXIST;
	}

	return BitMap_SetBit(lBitMapID,0,pstBitMapinfoNode->uiBitMapSize - 1);
}

ULONG BitMap_ClearBit(LONG lBitMapID,UINT uiStartPos,UINT uiEndPos)
{
	UINT uiSXPos = 0;
	UINT uiSYPos = 0;
	UINT uiEXPos = 0;
	UINT uiEYPos = 0;

	CHAR *pcSLine = NULL;
	CHAR *pcELine = NULL;
	CHAR *pcPLine = NULL;

	UINT uiIndex = 0;
	
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return ERROR_BIMAP_NOT_EXIST;
	}

	if (uiStartPos < 0 || uiEndPos > pstBitMapinfoNode->uiBitMapSize)
	{
		return ERROR_BIMAP_OUTOF_RANGE;
	}

	get_bit_xyPos(uiStartPos, uiSXPos, uiSYPos);
	get_bit_xyPos(uiEndPos,   uiEXPos, uiEYPos);

	pcSLine = pstBitMapinfoNode->pcBitMapList + uiSYPos;
	pcELine = pstBitMapinfoNode->pcBitMapList + uiEYPos;

	if (uiSYPos == uiEYPos)
	{
		/* 在一行 */
		(*pcSLine) &= ~((g_acBitSetMap[uiEXPos - uiSXPos] & g_acBitWid[1]) << uiSXPos);
	}
	else
	{
		/* 多于一行时 */
		/* 首行 */
		(*pcSLine) &= ~(g_acBitSetMap[8 - uiSXPos - 1] << uiSXPos);	
		/* 中间行(==2时不执行) */
		for (uiIndex = 1; uiIndex < (uiEYPos - uiSYPos); uiIndex++)
		{
			pcPLine = pcSLine + uiIndex;
			*(pcPLine) &= ~(g_acBitSetMap[7] & g_acBitWid[1]);
		}
		/* 尾行 */
		(*pcELine) &= ~(g_acBitSetMap[uiEXPos] & g_acBitWid[1]);
	}

	return ERROR_SUCCESS;
}

ULONG BitMap_ClearBitALL(LONG lBitMapID)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return ERROR_BIMAP_NOT_EXIST;
	}

	return BitMap_ClearBit(lBitMapID,0,pstBitMapinfoNode->uiBitMapSize - 1);
}

BIT_T  BitMap_GetBitVal(LONG lBitMapID, UINT uiPos)
{
	UINT uiXPos = 0;
	UINT uiYPos = 0;
	CHAR *pcPLine = NULL;
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return -1;
	}

	get_bit_xyPos(uiPos, uiXPos, uiYPos);
	
	pcPLine = pstBitMapinfoNode->pcBitMapList + uiYPos;

	if (0 != ((*pcPLine) & (1 << uiXPos)))
	{
		return BIT_1;
	}
	else
	{
		return BIT_0;
	}	
}

/*
	获取bitV(0 or 1)在 uiStartPos 到 uiEndPos 第一次出现的位置
*/
INT  BitMap_GetPos(LONG lBitMapID, UINT uiStartPos, UINT uiEndPos, BIT_T bitV)
{
	UINT uiPos = 0;
	
	for (uiPos = uiStartPos; uiPos <= uiEndPos; uiPos++)
	{
		if (bitV == BitMap_GetBitVal(lBitMapID, uiPos))
		{
			return uiPos;
		}
	}

	return -1;
}

UINT BitMap_GetSize(LONG lBitMapID)
{
	BIT_MAP_INFO_S *pstBitMapinfoNode = NULL;
	
	pstBitMapinfoNode = find_bitmapinfo_node(lBitMapID);
	if (NULL == pstBitMapinfoNode)
	{
		return ERROR_BIMAP_NOT_EXIST;
	}

	return pstBitMapinfoNode->uiBitMapSize;
}

STATIC VOID bitMapTest(VOID)
{
	INT pos = 0;
	LONG lTBitMap = CreateBitMap(256);
	BitMap_SetBit(lTBitMap,20,40);
	BitMap_ClearBit(lTBitMap,25,26);
	
	pos = BitMap_GetPos(lTBitMap,20,100, 0);

	if (25 == pos)
	{
		MSG_PRINTF("BitMap Test Success!");
	}

	DestroyBitMap(lTBitMap);
}

VOID BitMap_Init(void)
{
	DCL_Init(&g_astBitMapListHead);
	bitMapTest();
	return;
}

