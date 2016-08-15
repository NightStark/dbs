#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_rand_table.h>

#define HASH_KEY_LEN  (8)

UINT g_uiHashKey[HASH_KEY_LEN] = {
	0x94829847,
	0x94048983,
	0x93483980,
	0xbf234424,
	0xdfb34523,
	0x11111111,
	0xbfbf45ac,
	0xcccccccc,
};

UINT HASH_create_String(UCHAR *pucStr)
{
	UINT *puiRandTable = NULL;

	UINT uiTemp = 0;
	UINT uiI = 0;
	//UINT uiPos = 0;
	UINT uiKey = 0;

	DBGASSERT(NULL != pucStr);

	//uiPos = *pucStr;
	puiRandTable = RAND_TABLE_Get();
	uiTemp = puiRandTable[512];
	while ('\0' != *(pucStr++))
	{
		uiTemp *= puiRandTable[*pucStr];
		for (uiI = 0; uiI < HASH_KEY_LEN; uiI++)
		{
			
		/*
			uiTemp  = puiRandTable[uiPos];
			uiTemp *= puiRandTable[*pucStr];
			//uiTemp ^= g_uiHashKey[uiI];
			uiKey   = uiTemp;
			uiTemp  = uiTemp >> (*pucStr % 21); 
			uiPos   = uiTemp % 1024; 
		*/
		}
	}

	return uiKey;
}

