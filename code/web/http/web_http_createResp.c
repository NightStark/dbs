#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <ns_base.h>
#include <ns_ab.h>
#include <ns_web_server.h>

typedef enum tag_HttpStatus
{
	HTTP_STATUS_200 = 0,
	HTTP_STATUS_302,
	HTTP_STATUS_404,
	HTTP_STATUS_500,

	HTTP_STATUS_MAX,
}HTTP_STATUS_E;

const CHAR *g_aucHttpStatusList[HTTP_STATUS_MAX] = 
{
	[HTTP_STATUS_200] = "HTTP/1.1 200 OK\r\n",
	[HTTP_STATUS_302] = "HTTP/1.1 302 OK\r\n",
	[HTTP_STATUS_404] = "HTTP/1.1 404 NOT FOUND\r\n",
	[HTTP_STATUS_500] = "HTTP/1.1 500 ERROR\r\n",
};

STATIC const CHAR *web_http_GetHttpStatus(IN INT status)
{
	const CHAR *pcStatus = NULL;

    switch(status){
            case 200:
	            pcStatus = g_aucHttpStatusList[HTTP_STATUS_200];
	            break;
            case 302:
	            pcStatus = g_aucHttpStatusList[HTTP_STATUS_302];
	            break;
            case 404:
	            pcStatus = g_aucHttpStatusList[HTTP_STATUS_404];
	            break;
            case 500:
	            pcStatus = g_aucHttpStatusList[HTTP_STATUS_500];
	            break;
            default:
            	pcStatus = NULL;
	            break;
    }       
	return pcStatus;
}

/* 设置打包要返回数据的数据头 */
LONG WEB_http_SetHeadInfo(INOUT CHAR *headStr,
						  IN    ULONG ulHeadBufLen,
						  IN    INT   status, 
						  IN    INT   length, 
						  IN    CHAR *type)
{
    int day;
	time_t timep;
    char temp[50];
    AB_INFO_S *pstAB = NULL;
    char time_str[25];
    char mon[4],week[4],timestr[9],year[5];
    LONG lHeadLen = 0;

    /* 构造时间 */
    time(&timep);
    sprintf(time_str, (CHAR *)ctime(&timep));
    sscanf(time_str, "%s %s %d %s %s", week, mon, &day, timestr, year);
    sprintf(temp, "Date:%s, %02d %s %s %s GMT\r\n", week, day, mon, year, timestr );
    
    pstAB = AB_Create(ulHeadBufLen, (VOID *)headStr);
    if (NULL == pstAB)
    {
		ERR_PRINTF("AB Create Failed!");
		return -1;
    }

    AB_AppendString(pstAB, web_http_GetHttpStatus(status));
    AB_AppendString(pstAB, "SERVER:NSWebServer\r\n");
	AB_AppendString(pstAB, temp);
	AB_AppendString(pstAB, "Content-Length:%d\r\n", length);
	AB_AppendString(pstAB, "Keep-Alive:timeout=5,max=100\r\n");
	AB_AppendString(pstAB, "Connection:keep-alive\r\n");
	AB_AppendString(pstAB, "Accept-Ranges:bytes\r\n");
	AB_AppendString(pstAB, "Cache-Control:no-cache\r\n"); /* 禁止浏览器缓存网页 */
	AB_AppendString(pstAB, "Content-Type:%s\n\n", type);
    
	DBG_PRINT_LOG("respHeadStr.dat", headStr,strlen(headStr));

	lHeadLen = pstAB->ulABCurrentEnd;

	AB_Destroy(pstAB);
	
	return lHeadLen;
}

