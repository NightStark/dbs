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


char pri_buf[] = 
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEAzHT83KupygKknOreFvvi3Dp10mSdcIBzeHLoYIZy30/nKoNe\n"
"DAqPLYMKxnlSy742xtwUWa6TFJgPCDoNwQ4lxRzTudkpBkloOqt6NtbVdsT715GN\n"
"oJQZiC1w3wHDyPv0C/qRkkB4d/Ujbxii3FHNKeKHfFe0io0jxnu0L5LeaP15m2ZA\n"
"PPza9VkMr91uamG4dJQxu2u28fcw0wUc4aZ+0OPMTpoGgzTWBjb2Gyoe5RrAUKt1\n"
"VvDcobxIWYIj/3hWkeodA9p7X6sx7C84hY/BJKFSDVuxC14kn+VJT66RCYc0o7I+\n"
"k1LdGhkF8sBuAmrgGusnqNQWxQsIlYTaN2HctQIDAQABAoIBAQC7CVZ8xKlJfMt8\n"
"JTvQSNNK5kv8kUCZt1hq2quMCdKySRVPBegWGd7LIrLLyBce/b58gr43IIW6dDRk\n"
"MT992WRiArIJtQhbkPHBojbmTffvc9pzZNrsCC+nLPKxf5rMlPi3sJxU75eFHYMx\n"
"0ZbC+C44X9vTuyQVD3Kn02iNkMzXu05o4nC781LpDpELt0E8+U2LTSQPua31QgmV\n"
"/wd//jkGvpd+d9UD3xeTjnAZ9zf3Pz+VcORyTKqIdgvgP+n7H1a1mgEWrvSVkKZU\n"
"rNGsPr0NBGlkhEmVBnToxCStBH+FtjT/U+l+i+VDq9PSHfmbbb1Es9wgxsYLzHQE\n"
"Kb6YxhvBAoGBAPS1nzSOX5hHfTcDZNfGa3WtVDXVeDSUbnqnurxATW+HgUY4nA72\n"
"fNfNiCjxDVtYTGWwj73MCP/Gsul1V44O3myJ6b0By7RQK3ESrkVJTzCpycNPjnkg\n"
"nyiVeNcrSvFEQwUpTDtKQ9sfUErZvnJNjRTclV+GTHRdJkO4N3CYS+8FAoGBANXj\n"
"7MtOQ/iJjH0moWoKCvWsEYHzmWnRPRGqsTbcZREgjn/l/dDDHStHs0yY1ReOZdrP\n"
"/6nZDPuOc5PFE8mBHTe+54QvVjSHUraz43djPoN1FYWxjWX+S0JLVPNL5zGT5/rm\n"
"MOhPTLovfQwMfkNZ71yutnOHyg84acoIZpBgy8XxAoGBAJm7kC0gVIYCQ3PQOgYk\n"
"rVXcck7vsPi3V0DPJs0lSICMjHZlEgWEbKHWiXfPGCkOq9Amv+boy66EUdnA0iR4\n"
"yts9OQ54llTNGL2MktcMWyejQxHkZQlLqTGB1cy3cflofQI9E5sb6M08OuELMk30\n"
"0nEMsvq0EEv6SRO6I3ok829RAoGBAMiyA2CI4bJ1rkNIWjEbl0kJ2ausHYly8fB/\n"
"z6UAjDzT5aIQ9mZKjjYsIwt+ZISj8uKsROLkbRAljEFdrMFoyzehg68+k47aYzxX\n"
"8tn1X5UN6dH21gyb3ZYpBV79G8QjWorML5vrvLz/DkCeAGPGIZk/vqcNBQXfEzSl\n"
"SUez68ChAoGATjGyYJOc/ybL3X6U+vM6nAYVnHNAMUv3ZEaBG02izp9jQ/2r5yW1\n"
"DQw8pxFv7Fn9Cor7QMw84bBh/Gl+qUGccRzmpd75VBV8TdZjqmGuiYwCHIayoNXK\n"
"N8Yj90A5nWwt7mrndc/2Sp53V58lGm4PDvhjCpq5ctzbLNTTRaJTs4g=\n"
"-----END RSA PRIVATE KEY-----\n";

RSA * pri_key_buf_to_rsa(const char *pbuf2, int len)
{
    BIO *b2 = NULL;
    RSA *rsa = NULL;


    b2 = BIO_new_mem_buf(pbuf2, len);
    if (b2 == NULL) {
        printf("BIO_new_mem_buf failed!\n");
        return NULL;
    }
    PEM_read_bio_RSAPrivateKey(b2, &rsa, NULL, NULL);
    BIO_set_close(b2, BIO_CLOSE);
    BIO_free(b2);
    if (NULL == rsa){
        printf("Build RSA Public Key failed!\n");
        return NULL;
    }

    return rsa;
}

RSA * pri_key_file_to_rsa(const char *pri_file)
{
    int ret = -1;
    BIO *b = NULL;
    RSA *rsa = NULL;

    b = BIO_new_file(pri_file, "r");
    if (b == NULL) {
        printf("BIO_new_filefailed!\n");
        return NULL;
    }
    PEM_read_bio_RSAPrivateKey(b, &rsa, NULL, NULL);
    ret = BIO_set_close(b, BIO_CLOSE);
    BIO_free(b);
    if (NULL == rsa){
        printf("Build RSA Private Key failed! %d\n", ret);
        return NULL;
    }

    return rsa;
}

int rsa_sign_generate(unsigned char *md, int md_len,
                      unsigned char **sig, int *len)
{
    unsigned char *sigret = NULL;
    unsigned int siglen = 0;
    RSA *rsa = NULL;

#ifdef FW_RSA_USE_BUF
    rsa = pri_key_buf_to_rsa(pri_buf, strlen(pri_buf));
#else
    rsa = pri_key_file_to_rsa("pri.key");
#endif
    if (NULL == rsa) {
        return -1;
    }

    sigret = malloc(RSA_size(rsa));
    if (NULL == sigret){
        printf("Malloc error\n");
        RSA_free(rsa);
        return -1;
    }
    memset(sigret, 0, RSA_size(rsa));
    if (!RSA_sign(NID_sha256, md, md_len, sigret, &siglen, rsa)){
        printf("RSA_sign error\n");
        RSA_free(rsa);
        free(sigret);
        return -1;
    }
    RSA_free(rsa);

    *sig = sigret;
    *len = siglen;

    dump_data(md, md_len, __func__, __LINE__); \
    dump_data(sigret, siglen, __func__, __LINE__); \

    return 0;
}
