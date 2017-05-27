#ifndef __FW_H__
#define __FW_H__

void dump_data(unsigned char*buff, int count, const char *func, int line);

#ifdef FW_DEBUG
#define FW_LOG(fmt, ...) \
    do { \
        printf("[%s][%d]"fmt"\n" , __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define __DUMP_DATA(d, n) \
    do { \
        dump_data(d, n, __func__, __LINE__); \
    } while(0)
#else 
#define FW_LOG(fmt, ...) \
    do { \
        printf(fmt"\n", ##__VA_ARGS__); \
    } while (0)

#define __DUMP_DATA(d, n)
#endif

int rsa_sign_generate(unsigned char *md, int md_len,
                      unsigned char **sig, int *len);
int rsa_sign_verify(unsigned char *md, int md_len, 
                    unsigned char *sigret, int siglen);

#endif //__FW_H__
