#ifndef __NS_SSL_H__
#define __NS_SSL_H__

#include <openssl/ssl.h>
#include <openssl/err.h>

/*******************SERVER API***********************/
SSL * SERVER_SSL_link_create(INT iNewFd);
VOID  SERVER_SSL_link_destroy(SSL *pstSSL);
ULONG SERVER_SSL_link_init(VOID);
VOID  SERVER_SSL_link_fini(VOID);

/*******************CLIENT API***********************/
SSL * CLIENT_SSL_link_create(IN INT iSock);
VOID  CLIENT_SSL_link_destroy(IN SSL *pstSSL);
INT   CLIENT_SSL_link_connect(IN SSL *pstSSL);
ULONG CLIENT_SSL_link_init(VOID);
VOID  CLIENT_SSL_link_fini(VOID);

#endif //__NS_SSL_H__
