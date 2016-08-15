#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ns_base.h>
#include <ns_sm.h>
#include <ns_lilist.h>
#include <ns_msg_client_link.h>

/* 我认为，定义，只有一个SERVER(master)便于管理和维护 */
STATIC MSG_CLT_LINK_ST *g_pstMsgCltLink = NULL;

/*  crate link info for client sock and fsm */
MSG_CLT_LINK_ST *MSG_client_link_Create(IN INT iSockFd,
                                        IN INT iThrdID)
{
    MSG_CLT_LINK_ST *pstMsgCltLink = NULL;

    pstMsgCltLink = malloc(sizeof(MSG_CLT_LINK_ST));
    if (NULL == pstMsgCltLink) {
        ERR_PRINTF("malloc failed");
        return NULL;
    }
    memset(pstMsgCltLink, 0, sizeof(MSG_CLT_LINK_ST));
    pstMsgCltLink->iSockFd   = iSockFd;
    pstMsgCltLink->iThreadID = iThrdID;

    g_pstMsgCltLink = pstMsgCltLink;

    return pstMsgCltLink;
}

MSG_CLT_LINK_ST *MSG_client_link_Get(VOID)
{
    return g_pstMsgCltLink;
}

