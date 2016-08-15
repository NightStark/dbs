#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <ns_base.h>

STATIC SSL_CTX *ctx = NULL;

ULONG SSL_ShowCerts(IN const SSL * ssl, INOUT CHAR *pcBuf, IN INT iBufLen)
{
	X509 *cert;
	char *line;
	INT  iLen = 0;

	DBGASSERT(NULL != ssl);
	DBGASSERT(NULL != pcBuf);
	DBGASSERT(0    >= iBufLen);

	cert = SSL_get_peer_certificate(ssl);
	if (cert != NULL) {
		iLen += snprintf(pcBuf + iLen, iBufLen - iLen, "数字证书信息:\n");

		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		iLen += snprintf(pcBuf + iLen, iBufLen - iLen, "\r证书: %s\n", line);
		free(line);
		line = NULL;
		
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		iLen += snprintf(pcBuf + iLen, iBufLen - iLen, "\r颁发者: %s\n", line);
		free(line);
		line = NULL;

		X509_free(cert);
	} else {
		iLen += snprintf(pcBuf + iLen, iBufLen - iLen, "\r无证书信息！\n");
	}

	return ERROR_SUCCESS;
}

VOID CLIENT_SSL_link_destroy(IN SSL *pstSSL)
{
	DBGASSERT(NULL != pstSSL);

	/* 关闭连接 */
	SSL_shutdown(pstSSL);
	SSL_free(pstSSL);
	pstSSL = NULL;

	return;
}

SSL * CLIENT_SSL_link_create(IN INT iSock)
{
	SSL *pstSSL = NULL;

	DBGASSERT(NULL != ctx);
	DBGASSERT(NULL != ctx);

	pstSSL = SSL_new(ctx);
	if (NULL == pstSSL) {
		ERR_PRINTF("create new ssl failed");
		return NULL;
	}
	if (0 == SSL_set_fd(pstSSL, iSock)) {
		ERR_print_errors_fp(stderr);
		ERR_PRINTF("ssl set fd failed");
		goto err;
	}

	return pstSSL;

err:
	SSL_free(pstSSL);
	pstSSL = NULL;
	return NULL;
}

INT CLIENT_SSL_link_connect(IN SSL *pstSSL)
{
	INT iRet    = -1;
	INT iRetErr = -1;
	CHAR acBuf[256];

	DBGASSERT(NULL != pstSSL);

	/* 建立 SSL 连接 */
	iRet = SSL_connect(pstSSL);
	if (iRet == -1) {
		iRetErr = SSL_get_error(pstSSL, iRet);
		if (iRetErr == SSL_ERROR_WANT_READ) {
			MSG_PRINTF("ssl connect not compeleed.reed reconned.[%d:%d], ", iRetErr, SSL_ERROR_WANT_READ);
			return ERROR_LINK_SSL_WANT_READ;
		}
		ERR_PRINTF("ssl connect failed[%d:%d]", iRetErr, SSL_ERROR_WANT_READ);
		ERR_print_errors_fp(stderr);
		return -1;
	} else {
		MSG_PRINTF("Connected with %s encryption\n", SSL_get_cipher(pstSSL));
		SSL_ShowCerts(pstSSL, acBuf, sizeof(acBuf));
		MSG_PRINTF("%s", acBuf);
	}

	return 0;
}

ULONG CLIENT_SSL_link_init(VOID)
{
	/* SSL 库初始化，参看 ssl-server.c 代码 */
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) {
		ERR_print_errors_fp(stdout);
		return ERROR_FAILE;
	}

	MSG_PRINTF("server SSL init success.");

	return ERROR_SUCCESS;
}

VOID CLIENT_SSL_link_fini(VOID)
{
	if (NULL != ctx) {
		SSL_CTX_free(ctx);
		ctx = NULL;
	}
	return;
}
