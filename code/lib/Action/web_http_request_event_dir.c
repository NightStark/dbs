/* Copyright (C) 
* 2016 - langyj, lyj051031448@163.com
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/
/**
* @file web_http_request_event_dir.c
* @Synopsis  
* @author langyj, lyj051031448@163.com
* @version 1.2.3
* @date 2016-09-19
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/sha.h> 
#include <openssl/crypto.h>  // OPENSSL_cleanse

#include <ns_base.h>
#include <ns_web_event.h>

STATIC DCL_HEAD_S g_stHttpReq_EvtDir_ListHead;

STATIC VOID __printHash(IN UCHAR *md, IN INT len)
{
    INT i = 0;
    for (i = 0; i < len; i++)
    {
        printf("%02x", md[i]);
    }

    printf("\n");

    return;
}


/* ================================================================================ */
/**
* @Synopsis httpReq_EvtDir_Hash build hash of evt-dir-string
*
* @Param pcDir
* @Param iLen : length of @pcDir
* @Param pucDirHash
*
* @Returns 
* @author langyj, lyj051031448@163.com
* @date 2016-09-19
*/
/* ================================================================================ */
STATIC ULONG httpReq_EvtDir_Hash(IN CHAR *pcDir, IN INT iLen, OUT UCHAR *pucDirHash)
{
    DBGASSERT(NULL != pcDir);
    DBGASSERT(0    != iLen);
    DBGASSERT(NULL != pucDirHash);

    SHA1((UCHAR *)pcDir, iLen, pucDirHash);
    __printHash(pucDirHash, SHA_DIGEST_LENGTH);

    /*
       SHA_CTX c;
       SHA1_Init(&c);
       SHA1_Update(&c, orgStr, strlen(orgStr));
       SHA1_Final(md, &c);
       OPENSSL_cleanse(&c, sizeof(c));
       printHash(md, SHA_DIGEST_LENGTH);
       */
    return ERROR_SUCCESS;
}

/* ================================================================================ */
/**
* @Synopsis HTTPREQ_EvtDir_Add 
* @Param pcDir :url-dir string
* @Returns 
* @author langyj, lyj051031448@163.com
* @date 2016-09-19
*/
/* ================================================================================ */
HTTPREQ_EVTDIR_INFO_ST *HTTPREQ_EvtDir_Add(IN CHAR *pcDir, IN INT iLen)
{
    HTTPREQ_EVTDIR_INFO_ST *pstHttpReqEvtDirInfo = NULL;

    DBGASSERT(NULL != pcDir);

    if (iLen >= sizeof(pstHttpReqEvtDirInfo->acDir)) {
        ERR_PRINTF("dir is too long, max is : %d.", 
                sizeof(pstHttpReqEvtDirInfo->acDir));
        return NULL;
    }

    pstHttpReqEvtDirInfo = malloc(sizeof(HTTPREQ_EVTDIR_INFO_ST));
    if (NULL == pstHttpReqEvtDirInfo) {
        return NULL;
    }

    memset(pstHttpReqEvtDirInfo, 0, sizeof(HTTPREQ_EVTDIR_INFO_ST));
    snprintf(pstHttpReqEvtDirInfo->acDir, sizeof(pstHttpReqEvtDirInfo->acDir), "%s", pcDir);
    httpReq_EvtDir_Hash(pstHttpReqEvtDirInfo->acDir, 
                        sizeof(pstHttpReqEvtDirInfo->acDir), 
                        pstHttpReqEvtDirInfo->aucDirHash);
    DCL_AddTail(&g_stHttpReq_EvtDir_ListHead, &(pstHttpReqEvtDirInfo->stNode));

    return pstHttpReqEvtDirInfo;
}

UINT HASH_UINT_024CMP(IN VOID *p1, IN VOID *p2)
{
    UINT *pi1 = 0; 
    UINT *pi2 = 0;

    pi1 = (UINT *)(p1);
    pi2 = (UINT *)(p1);

    return ((pi1[0] == pi2[0]) && (pi1[2] == pi2[2]) && (pi1[4] == pi2[4]));
}

/* ================================================================================ */
/**
* @Synopsis HTTPREQ_EvtDir_findByHash find event dir int list by hash of dir string
*
* @Param pcDir
* @Param iLen
*
* @Returns 
* @author langyj, lyj051031448@163.com
* @date 2016-09-19
*/
/* ================================================================================ */
HTTPREQ_EVTDIR_INFO_ST *HTTPREQ_EvtDir_findByHash(IN CHAR *pcDir, IN INT iLen)
{
    UCHAR ucHash[SHA_DIGEST_LENGTH];
    HTTPREQ_EVTDIR_INFO_ST *pstHttpReqEvtDirInfo = NULL;

    memset(ucHash, 0, sizeof(ucHash));
    httpReq_EvtDir_Hash(pcDir, iLen, ucHash);

    DCL_FOREACH_ENTRY(&(g_stHttpReq_EvtDir_ListHead), pstHttpReqEvtDirInfo, stNode) {
        if (HASH_UINT_024CMP(pstHttpReqEvtDirInfo->aucDirHash, ucHash)) {
            return pstHttpReqEvtDirInfo;
        }
    }

    return NULL;
}

/* ================================================================================ */
/**
* @Synopsis HttpReq_EvtDir_Init Initial http request event url-dir list
* @Param p
* @Returns 
* @author langyj, lyj051031448@163.com
* @date 2016-09-19
*/
/* ================================================================================ */
ULONG HTTPREQ_EvtDir_Init(VOID)
{

    DCL_Init(&g_stHttpReq_EvtDir_ListHead);
    
    return ERROR_SUCCESS;
}
