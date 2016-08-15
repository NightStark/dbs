#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <ns_base.h>
#include <ns_ssl.h>

#define CACERT_PEM_FILE_PATH  "cacert.pem"
#define PRIVKEY_PEM_FILE_PATH "privkey.pem"

//TODO:need a lock ??
static SSL_CTX *ctx = NULL;
static int g_iSSLIsInit = 0;

static inline int __SSL_IS_INITED__(VOID) 
{
	return g_iSSLIsInit == 1;
}

SSL * SERVER_SSL_link_create(INT iNewFd)
{
	SSL *pstSSL = NULL;

	DBGASSERT(__SSL_IS_INITED__());

	/* 基于 ctx 产生一个新的 SSL */
	pstSSL = SSL_new(ctx);
	if (NULL == pstSSL) {
		ERR_PRINTF("create a new ssl failed.");
		return NULL;
	}
	/* 将连接用户的 socket 加入到 SSL */
	if (!SSL_set_fd(pstSSL, iNewFd)) {
		ERR_PRINTF("SSL set FD failed.");
		goto err_free;
	}
	/* 建立 SSL 连接 */
	if (SSL_accept(pstSSL) == -1) {
		perror("accept");
		ERR_PRINTF("SSL accept failed.");
		goto err_free;
	}

	return pstSSL;

err_free:
	SSL_free(pstSSL);
	pstSSL = NULL;

	return NULL;
}

VOID SERVER_SSL_link_destroy(SSL *pstSSL)
{
	DBGASSERT(NULL != pstSSL);

	SSL_shutdown(pstSSL);    
	SSL_free(pstSSL);
	pstSSL = NULL;

	return;
}

ULONG SERVER_SSL_link_init(VOID)
{

	/* SSL 库初始化 */
	SSL_library_init();
	/* 载入所有 SSL 算法 */
	OpenSSL_add_all_algorithms();
	/* 载入所有 SSL 错误消息 */
	SSL_load_error_strings();
	/* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
	ctx = SSL_CTX_new(SSLv23_server_method());
	/* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
	if (ctx == NULL) {
		ERR_print_errors_fp(stdout);
		ERR_PRINTF("Create SSL CTX Failed!");
		return ERROR_FAILE;
	}
	MSG_PRINTF("Create SSL CTX success!");

	/* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
	if (SSL_CTX_use_certificate_file(ctx, CACERT_PEM_FILE_PATH, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stdout);
		ERR_PRINTF("load user certificat file failed!");
		return ERROR_FAILE;
	}
	MSG_PRINTF("load user certificat file success!");

	/* 载入用户私钥 */
	if (SSL_CTX_use_PrivateKey_file(ctx, PRIVKEY_PEM_FILE_PATH, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stdout);
		ERR_PRINTF("load user private file failed!");
		return ERROR_FAILE;
	}
	MSG_PRINTF("load user private file success!");

	/* 检查用户私钥是否正确 */
	if (!SSL_CTX_check_private_key(ctx)) {
		ERR_print_errors_fp(stdout);
		ERR_PRINTF("check private file failed!");
		return ERROR_FAILE;
	}
	MSG_PRINTF("check private file success!");

	g_iSSLIsInit = 1;

	return ERROR_SUCCESS;
}

VOID SERVER_SSL_link_fini(VOID)
{
	if (NULL != ctx) {
		SSL_CTX_free(ctx);
		ctx = NULL;
	}
	return;
}
