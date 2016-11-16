#ifndef __NS_MSG_SRV_LINK_H__
#define __NS_MSG_SRV_LINK_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

typedef union usa {
    struct sockaddr sa;
    struct sockaddr_in sin;
#if defined(USE_IPV6)
    struct sockaddr_in6 sin6;
#endif
}SOCK_ADDR_UN;
typedef struct tag_ClientInfo
{
    VOID *pLink;
    SOCK_ADDR_UN unClientAddr; 
    UCHAR        aucClientAddrStr[16]; /* eg:192.168.111.128 */
    UCHAR        aucClientMac[6];
}LINK_CLIENT_INFO_ST;

typedef struct tag_MsgServerLink
{
    DCL_NODE_S stNode;

    INT        iSockFd;
    UINT       uiLinkID;
    INT        iThreadID; /* this link is work at this thread  */
	SSL       *pstSSL;
    VOID      *pstSrvSm;
    ULONG      ulSessionID;
    LINK_CLIENT_INFO_ST stClientInfo;
}MSG_SRV_LINK_ST;

ULONG MSG_server_ctl_send_attach(MSG_SRV_LINK_ST *pstSrvLink);
#endif //__NS_MSG_SRV_LINK_H__
