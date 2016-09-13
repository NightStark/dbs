#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/des.h>
#include "base64.h"


static void
__dump_data(unsigned char *ptr, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n");
}

static unsigned char cbc_key[8] = {'x', '#', '2', 's', 'x', '9', '(', 'x'};
static unsigned char cbc_iv[8]  = {'s', 'a', 'l', 't', '#', '&', '@', '!'};

static int _des_encrypt(const unsigned char *data, int data_len, unsigned char *buf) 
{

    DES_key_schedule ks;
    DES_set_key_unchecked((DES_cblock *)cbc_key, &ks);

    DES_ncbc_encrypt(data, buf, data_len, &ks, (DES_cblock *)cbc_iv, DES_ENCRYPT);
    
    return 0;
}

static int _des_decrypt(const unsigned char *data, int data_len, unsigned char *buf) 
{
    DES_key_schedule ks;

    DES_set_key_unchecked((DES_cblock *)cbc_key, &ks);

    DES_ncbc_encrypt(data, buf, data_len, &ks, (DES_cblock *)cbc_iv, DES_DECRYPT);
    

    return 0;
}

int des_encrypt(unsigned char *data, int data_len,
                unsigned char *enc_buf, int enc_buf_len)
{
    int pad_len = 0;
    unsigned char *data2 = NULL;

    /* PKCS7 padding */
    pad_len = 8 - (data_len % 8);
    memset(data + data_len, pad_len, pad_len);
    data_len += pad_len;

    __dump_data(data, data_len);
    data2 = malloc(data_len);
    if (data2 == NULL) {
        return -1;
    }

    _des_encrypt((const unsigned char *)data, data_len, data2);
    __dump_data(data2, data_len);
    base64_encode(enc_buf, data2, data_len);
    free(data2);
    data2 = NULL;

    return 0;
}

#define DD_DATA_SIZE (512)

int main(void)
{
    int data_len = 0;
    unsigned char *gen_data = "12345678";
    unsigned char  data1[DD_DATA_SIZE + 8] = {0}; //+8 for padding
    unsigned char  b64[DD_DATA_SIZE * 2] = {0}; //x2 for base64 

    data_len = snprintf((char *)data1, sizeof(data1), "%s", gen_data);

    des_encrypt(data1, data_len, b64, sizeof(b64));

    printf("des-b64:%s\n", b64);

    return 0;
}
