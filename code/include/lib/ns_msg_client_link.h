#ifndef __NS_MSG_CLT_LINK_H__
#define __NS_MSG_CLT_LINK_H__

#include <openssl/ssl.h>

typedef struct tag_MsgClientLink
{
    DCL_NODE_S stNode;

    INT        iSockFd;
    UINT       uiLinkID;
    INT        iThreadID; /* this link is work at this thread  */
    VOID      *pstCltSm;
	SSL       *pstSSL;    
    ULONG      ulSessionID;
}MSG_CLT_LINK_ST;

MSG_CLT_LINK_ST *MSG_client_link_Create(IN INT iSockFd,
                                        IN INT iThrdID);
MSG_CLT_LINK_ST *MSG_client_link_Get(VOID);

ULONG MSG_clinet_ctl_recv_attach (MSG_CLT_LINK_ST *pstCltLink, VOID *pstMsg);
ULONG MSG_clinet_ctl_recv_Upgrade(MSG_CLT_LINK_ST *pstCltLink, VOID *pMsg);

#endif //__NS_MSG_CLT_LINK_H__
