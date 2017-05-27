/***************************************************************************
 * file  : rsa_sign.c
 * author: lang.yanjun@tianyuantechnology.com
 * data  : 2017-4-20 20:34
 * description: rsa api to make sign and verify sign
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#define LOG_ERR     (4)
#define LOG_WARING  (5)
#define LOG_DEBUG   (6)
#define LOG_INFO    (7)

#include "fw.h"

/* build private key and public key:
 *  openssl genrsa -out pri.key 2048 //生成私钥长度2048bit
 *  openssl rsa -in pri.key -pubout > pub.key //生成公钥
 */

char pbuf[] = 
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzHT83KupygKknOreFvvi\n"
"3Dp10mSdcIBzeHLoYIZy30/nKoNeDAqPLYMKxnlSy742xtwUWa6TFJgPCDoNwQ4l\n"
"xRzTudkpBkloOqt6NtbVdsT715GNoJQZiC1w3wHDyPv0C/qRkkB4d/Ujbxii3FHN\n"
"KeKHfFe0io0jxnu0L5LeaP15m2ZAPPza9VkMr91uamG4dJQxu2u28fcw0wUc4aZ+\n"
"0OPMTpoGgzTWBjb2Gyoe5RrAUKt1VvDcobxIWYIj/3hWkeodA9p7X6sx7C84hY/B\n"
"JKFSDVuxC14kn+VJT66RCYc0o7I+k1LdGhkF8sBuAmrgGusnqNQWxQsIlYTaN2Hc\n"
"tQIDAQAB\n"
"-----END PUBLIC KEY-----\n";

RSA * pub_key_buf_to_rsa(const char *pub_buf, int len)
{
    int ret = -1;
    BIO *b = NULL;
    RSA *rsa = NULL;

    b = BIO_new_mem_buf(pub_buf, len);
    if (b == NULL) {
        printf("BIO_new_mem_buf failed!\n");
        return NULL;
    }

    PEM_read_bio_RSA_PUBKEY(b, &rsa, NULL, NULL);
    ret = BIO_set_close(b, BIO_CLOSE);
    BIO_free(b);
    if (NULL == rsa){
        printf("Build RSA Private Key failed! %d\n", ret);
        return NULL;
    }

    return rsa;
}

RSA * pub_key_file_to_rsa(const char *pub_file)
{
    int ret = -1;
    BIO *b2   = NULL;
    RSA *rsa2 = NULL;

    b2 = BIO_new_file("pub.key", "r");
    if (b2 == NULL) {
        printf("BIO_new_file failed!\n");
        return NULL;
    }

    PEM_read_bio_RSA_PUBKEY(b2, &rsa2, NULL, NULL);
    ret = BIO_set_close(b2, BIO_CLOSE);
    BIO_free(b2);
    (void)ret;

    if (NULL == rsa2){
        FW_LOG("Build RSA Public Key failed!\n");
        return NULL;
    }

    return rsa2;
}

int rsa_sign_verify(unsigned char *md, int md_len, 
                    unsigned char *sigret, int siglen)
{
    int res = -1;
    RSA *rsa2 = NULL;

#ifdef FW_RSA_USE_BUF 
    rsa2 = pub_key_buf_to_rsa(pbuf, strlen(pbuf));
#else
    rsa2 = pub_key_file_to_rsa("pub.key");
#endif
    if (NULL == rsa2){
        printf("Build RSA Public Key failed!\n");
        goto fail;
    }

    dump_data(md, md_len, __func__, __LINE__); \
    dump_data(sigret, siglen, __func__, __LINE__); \
    res = RSA_verify(NID_sha256, md, md_len, sigret, siglen, rsa2);
    if (!res){
        printf("Invalid Signature\n");
        RSA_free(rsa2);
        goto fail;
    } else {
        RSA_free(rsa2);
        printf("Signature is OK.\n");
    }

    return 0;
fail:
    return -1;
}

#if 0
int test_main(void)
{
    unsigned char *sigret = NULL;
    int siglen = 0;
    unsigned char md[32] = {0xAA};

    rsa_sign_generate(md, sizeof(md), &sigret, &siglen);
    __DUMP_DATA(md, sizeof(md));
    __DUMP_DATA(sigret, siglen);
    printf("siglen = %d\n", siglen);
    rsa_sign_verify(md, sizeof(md), sigret, siglen);

    free(sigret);
    sigret = NULL;

    return 0;
}
#endif
