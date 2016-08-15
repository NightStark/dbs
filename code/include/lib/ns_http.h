/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : ns_http.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/8/31
  Last Modified :
  Description   : http处理相关的函数
  Function List :
  History       :
  1.Date        : 2014/8/31
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#ifndef __HTTP_H__
#define __HTTP_H__

ULONG WEB_http_HandleHttpReq(IN INT    connfd, 
 							 IN CHAR *pucBuf, 
 							 IN ULONG  ulHttpHeadSize);
ULONG WEB_http_DecHttpReqMsg(IN CHAR *pucBuf, 
                             IN ULONG ulBufLen, 
                             IN WEB_HTTP_REQMSGINFO_S *pstHttpReqMsgInfo);
ULONG WEB_http_GetPurenessFileData(IN CHAR    *pucFilePath, 
								   IN CHAR    *pcRespBUF,
								   INOUT UINT *puiReapBufLen);
ULONG WEB_http_HandleEvent(IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo, 
						   INOUT CHAR *pcRespBUF, 
						   INOUT UINT *puiReapBufLen);

LONG WEB_http_SetHeadInfo(INOUT CHAR *headStr,
						  IN    ULONG ulHeadBufLen,
						  IN    INT   status, 
						  IN    INT   length, 
						  IN    CHAR *type);
ULONG WEB_http_ReadReqFile(IN CHAR *pucFilePath, 
 						   IN CHAR *pcFileBuf, 
 						   INOUT ULONG *pulReqFileSize);

#endif //__HTTP_H__

