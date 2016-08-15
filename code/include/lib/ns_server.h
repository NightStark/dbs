#ifndef __SERVER_H__
#define __SERVER_H__

#include <ns_msg_server_link.h>

VOID SERVER_Init(VOID);
VOID SERVER_Fint(VOID);


INT  MSG_server_link_Init(VOID);
VOID MSG_server_link_Fini(VOID);
MSG_SRV_LINK_ST * MSG_server_link_Create(IN INT iSockFd,
                                         IN INT iThrdID);
VOID MSG_server_link_Destroy (MSG_SRV_LINK_ST *stSrvLink);
INT  MSG_server_link_Add2List(MSG_SRV_LINK_ST *stSrvLInk);
INT  MSG_server_link_DelFList(MSG_SRV_LINK_ST *stSrvLInk);
MSG_SRV_LINK_ST * MSG_server_link_FindBySockFd(IN INT iSockFd);
//INT MSG_server_GetSockFdBySm(IN SRV_SM_ST *pstSrvSm);
MSG_SRV_LINK_ST * MSG_server_link_Up(IN INT iSockFd,
                                     IN INT iThrdID);
INT MSG_server_link_Down(IN INT iSockFd);

  

ULONG Server_CreateClientTable(VOID);
ULONG Server_DestroyClientTable(VOID);

#endif //__SERVER_H__
