/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : id.c
  Version       : Initial Draft
  Author        : Night Stark [lyj051031448@163.com]
  Created       : 2013/12/31
  Last Modified :
  Description   : 可用于各种ID的分配
  Function List :
  History       :
  1.Date        : 2013/12/31
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

LONG CreateIDPool(INT uiIDPoolSize)
{
	return CreateBitMap(uiIDPoolSize);
}

VOID DestroyIDPool(LONG lIDPool)
{
	DestroyBitMap(lIDPool);
}

INT AllocID(LONG lIDPool, UINT uiIDStart)
{
	UINT uiBitSize = 0;
	UINT uiBit0_Pos = 0;
	uiBitSize = BitMap_GetSize(lIDPool);
	if (ERROR_BIMAP_NOT_EXIST == uiBitSize)
	{
		return -1;
	}
	uiBit0_Pos = BitMap_GetPos(lIDPool, uiIDStart, uiBitSize - 1, BIT_0);
	if (-1 == uiBitSize)
	{
		return -1;
	}
	BitMap_SetBit(lIDPool, uiBit0_Pos, uiBit0_Pos);

	return uiBit0_Pos;
}

VOID DeleteID(LONG lIDPool, INT uiID)
{
	ULONG ulRet = 0;
	ulRet = BitMap_ClearBit(lIDPool, uiID, uiID);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Something is Wrong In BitMap_ClearBit!");
	}
	
	return;
}

