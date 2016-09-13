#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>

int data_encrypt(char *data, int data_len, char *buf, int buf_len) 
{
    //char *gen_data = "hello wrold!";
    char *key = "123456";
    DES_cblock cblock;
    DES_key_schedule ks;
    char un_enc_buf[16];
    char *enc_buf = NULL;
    int  do_len = 0;

    enc_buf = buf;
    des_string_to_key(key, &cblock);
    DES_set_key_unchecked(&cblock, &ks);

    
    do {
        /* padding as Zero */
        memset(un_enc_buf, 0, sizeof(un_enc_buf));
        if ((data_len - do_len) < 8) {
            memcpy(un_enc_buf, (data + do_len), (data_len - do_len));
            do_len = data_len;
        } else {
            memcpy(un_enc_buf, (data + do_len), 8);
            do_len += 8;
        }
        DES_ecb_encrypt((const_DES_cblock *)un_enc_buf, 
                (DES_cblock *)enc_buf, 
                &ks, 
                DES_ENCRYPT);
        enc_buf += 8;
    } while (do_len < data_len);

    return 0;
}

int data_decrypt(char *data, int data_len, char *buf, int buf_len) 
{
    //char *gen_data = "hello wrold!";
    char *key = "123456";
    DES_cblock cblock;
    DES_key_schedule ks;
    char *un_enc_buf = NULL;
    char enc_buf[16];
    int  do_len = 0;

    un_enc_buf = buf;
    des_string_to_key(key, &cblock);
    DES_set_key_unchecked(&cblock, &ks);
    
    do {
        memset(enc_buf, 0, sizeof(enc_buf));
        if ((data_len - do_len) < 8) {
            memcpy(enc_buf, (data + do_len), (data_len - do_len));
            do_len = data_len;
        } else {
            memcpy(enc_buf, (data + do_len), 8);
            do_len += 8;
        }
        DES_ecb_encrypt((const_DES_cblock *)enc_buf, 
                (DES_cblock *)un_enc_buf, 
                &ks, 
                DES_DECRYPT);
        un_enc_buf += 8;
    } while (do_len < data_len);

    return 0;
}

int main(void)
{
    char *gen_data = "hello wrold!, hahahaha";

    char data2[128];
    char data3[128];

    data_encrypt(gen_data, strlen(gen_data), data2, sizeof(data2));
    data_decrypt(data2, strlen(data2), data3, sizeof(data3));


    printf("des:%s\n", data3);

    return 0;
}

