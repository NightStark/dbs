#ifndef __UP_STRING_H__
#define __UP_STRING_H__

typedef struct tag_strA2Binfo
{
    const char *strA;
    const char *strB;
    const char *posA;
    const char *posB;
    int A2BLen; /* exclude '\0' */
}STR_A2B_INFO_ST;

typedef struct tag_http_field_node
{
    struct list_head stNode;
    STR_A2B_INFO_ST stA2B;
}HTTP_FIELD_NODE_ST;

char *__strstr2(const char *src, const char *d, int max);
const char * __get_msg_of_A2B(const char *tt, int tt_len, 
                                        char *buf, int buf_len, 
                                        const char *a,
                                        const char *b);
const char * __get_msg_of_P2B(const char *tt, int tt_len, 
                                        char *buf, int buf_len, 
                                        const char *b);
const char * __get_msg_of_ltgt(const char *tt, int tt_len, char *buf, int buf_len);
int __get_str_pos_of_A2B(const char *str, int strlen, STR_A2B_INFO_ST *pstStrA2BInfo);
int _get_http_header_filed(const char *str, 
                                         int strlen, 
                                         const char *hdr_filed, 
                                         STR_A2B_INFO_ST *pstA2B);
char * __A2B_strstr(STR_A2B_INFO_ST *pstA2B, const char *str);

#include <asm/io.h>
#define UP_MSG_PRINTF(fmt, ...) \
    printk("[%s][%d]", __func__, __LINE__); \
    printk(fmt, ##__VA_ARGS__); \
    printk("\n")

#endif //__UP_STRING_H__
