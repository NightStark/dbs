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

#define RAND_TABLE_FILE_NAME "rand_table.dat"
#define RAND_TABLE_MOD           (0xFFFFFFFF)
#define RAND_TABLE_OFF_MOD       (32 - 10 - 1)
#define RAND_TABLE_TEST_MOD		 (1024)

STATIC UINT *g_puiRandTable = NULL;

STATIC ULONG rand_table_WtiteFile(IN UINT *puiRandTable, IN UINT uiTableLen)
{
	INT uiRTFD = 0;
	INT uiWriteRet = 0;
	uiRTFD = open(RAND_TABLE_FILE_NAME, O_CREAT|O_TRUNC|O_RDWR, 664);
	if(-1 == uiRTFD)							
	{										
		ERR_PRINTF("open log file failed!");
	}		

	/* 写入表的长度 */
	write(uiRTFD, (CHAR *)&uiTableLen, sizeof(uiTableLen));	
	/* 写入表的数据 */
	uiWriteRet = write(uiRTFD, (CHAR *)puiRandTable, uiTableLen * sizeof(UINT));		
	close(uiRTFD);	

	if (uiTableLen != uiWriteRet)
	{
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;	
}

STATIC ULONG rand_table_ReadFile(void)
{
	INT uiRTFD = 0;
	INT uiReadRet = 0;
	IN UINT uiTableLen = 0;
	uiRTFD = open(RAND_TABLE_FILE_NAME, O_RDONLY);
	if(-1 == uiRTFD)							
	{										
		ERR_PRINTF("open log file failed!");
	}		

	/* 读取表的长度 */
	uiReadRet = read(uiRTFD, (CHAR *)&uiTableLen, sizeof(UINT));	
	if(-1 == uiReadRet)
	{
		ERR_PRINTF("read file failed!");
		return ERROR_FAILE;
	}
	g_puiRandTable = mem_alloc(uiTableLen * sizeof(UINT));
	
	/* 读取表的数据 */
	uiReadRet = read(uiRTFD, (CHAR *)g_puiRandTable, uiTableLen * sizeof(UINT));		
	close(uiRTFD);	

	if (uiTableLen * sizeof(UINT) != uiReadRet)
	{
		ERR_PRINTF("read file failed!");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;	
}

UINT *RAND_TABLE_Get(VOID)
{
	return (g_puiRandTable);
}

ULONG RAND_TABLE_Init(VOID)
{
	return rand_table_ReadFile();
}

VOID RAND_TABLE_Fint(VOID)
{
	free(g_puiRandTable);

	return;
}
VOID RAND_table_create(IN UINT uiTableLen)
{
	UINT *puiRandTable = NULL;
	UINT  uiI = 0;
	UINT  uiTemp = 0;

	puiRandTable = mem_alloc(uiTableLen * sizeof(UINT) + 1);

	srand(time(0));
	while (1)
	{
		mem_set0(puiRandTable, uiTableLen * sizeof(UINT) + 1);
		uiTemp = 0;
		for (uiI = 0; uiI < uiTableLen; uiI++)
		{
			/* 创建随机数 */
			puiRandTable[uiI] = rand() % RAND_TABLE_MOD;

			/* 用于平均检查 
			 * 随机(利用上一个随机数)移动位置，取低十位求和在求平均数
			 */
			uiTemp += ((puiRandTable[uiI]) >> (puiRandTable[uiI - 1] % RAND_TABLE_OFF_MOD)) % RAND_TABLE_TEST_MOD;
		}

		uiTemp = uiTemp / uiTableLen;

		/* 平均数为1/2的认为是较好的随机分布，写入随机文件 */
		if (RAND_TABLE_TEST_MOD / 2 == uiTemp)
		{
			ERR_PRINTF("T = %d", uiTemp);
			break;
		}

	}
	rand_table_WtiteFile(puiRandTable, uiTableLen);
	
	return ;
}
