#ifndef __MSG_CLIENT_H__
#define __MSG_CLIENT_H__


INT   MSG_client_CreatSocket(VOID);
ULONG MSG_client_CreateLink(IN INT iClientfd, INT *piConnFd);
ULONG DataBase_client_init(VOID);
VOID  DataBase_client_Fini(VOID);

#endif //__MSG_CLIENT_H__
