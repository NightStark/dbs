#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "string.h"
#include "up.h"

static const char * g_http_header_field_list[] = 
{
    //"Accept",
    //"Accept-Charset",
    //"Accept-Encoding",
    "Accept-Language",
    "Accept-Ranges",
    "Age",
    "Allow",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Content-Encoding",
    "Content-Language",
    //"Content-Length",
    "Content-Location",
    "Content-MD5",
    "Content-Range",
    //"Content-Type",
    "Date",
    "ETag",
    "Expect",
    "Expires",
    "From",
    //"Host",
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Last-Modified",
    "Location",
    "Max-Forwards",
    "Pragma",
    "Proxy-Authenticate",
    "Proxy-Authorization",
    "Range",
    "Referer",
    "Retry-After",
    "Server",
    "TE",
    "Trailer",
};

static int __http_field_add_sort_by_addr (HTTP_FIELD_NODE_ST *pstHttpF, 
                                          struct list_head *head)
{
    HTTP_FIELD_NODE_ST *pstN = NULL;

    list_for_each_entry(pstN, head, stNode) {
        if (pstN->stA2B.posA < pstHttpF->stA2B.posA) {
            list_add(&(pstHttpF->stNode), (pstN->stNode.prev));
            return 0;
        }
    }
    list_add_tail(&(pstHttpF->stNode), head);
    return 0;
}

static int __http_response_inject_check_room(char *http_hdr, int http_hdr_len, 
        int need_len, struct list_head *http_field_list_head)
{
    int i                  = 0;
    int dig_len            = 0;
    HTTP_FIELD_NODE_ST *pstHttpField = NULL, *n = NULL;
    STR_A2B_INFO_ST stA2B;

    memset(&stA2B, 0, sizeof(stA2B));

    UP_MSG_PRINTF("--------------");
    for (i = 0; i < ARRAY_SIZE(g_http_header_field_list); i++) {
        if (0 == _get_http_header_filed(http_hdr, http_hdr_len, g_http_header_field_list[i], &stA2B)) {
            dig_len += stA2B.A2BLen;
            /*pstHttpField = vmalloc(sizeof(HTTP_FIELD_NODE_ST));
             *use vmalloc will cause:
             *  211.998180]  [<c1096810>] ? __vmalloc_node_range+0x50/0x1c0
             *  [  211.999119]  [<d0d7bb4a>] ? up_ct_http_response_inject+0x21a/0x4c0 [mod_up]
             *  [  212.000273]  [<c13e0576>] ? printk+0x37/0x3b
             *  [  212.000995]  [<c10969c3>] ? __vmalloc_node+0x43/0x50
             *  [  212.001827]  [<d0d7bb4a>] ? up_ct_http_response_inject+0x21a/0x4c0 [mod_up]
             *
             *  kmalloc 适用于物理上连续的小内存。
             *  vmalloc 用于虚拟的大块内存。
             */
            pstHttpField = kmalloc(sizeof(HTTP_FIELD_NODE_ST), GFP_ATOMIC);
            if (pstHttpField == NULL) {
                UP_MSG_PRINTF("oom ...!");
                goto err_oom;
            }
            memcpy(&(pstHttpField->stA2B), &stA2B, sizeof(pstHttpField->stA2B));
            //list_add_tail(&(pstHttpField->stNode), http_field_list_head);
            /* 一定要添加到头部，这样可以倒叙遍历去进行move,而不需要刷新该表了！！！ */
            //list_add(&(pstHttpField->stNode), http_field_list_head);
            UP_MSG_PRINTF("--------------");
            __http_field_add_sort_by_addr(pstHttpField, http_field_list_head);
        }
    }

    if (dig_len >= need_len) {
        return 1;
    }


    return 0;
err_oom:
    list_for_each_entry_safe(pstHttpField, n, http_field_list_head, stNode) {
        list_del(&(pstHttpField->stNode));
        kfree(pstHttpField);
    }
    pstHttpField = NULL;
    n = NULL;
    return -1;
}

/*
 * |**************|****[fieldA][fieldB][fieldC]***********|**************************************|
 * |<-TCP-Header->|<-----------HTTP-Header--------------->|<html><head>......</head>......</html>|
 *                ^                                       ^            ^
 *                |                    ^                  |            |
 *                http_hdr             |                  http_data    inject_start(inject_end)
 *                                     |                               |
 * after inject                        |                               |
 *                                     |-------------------------------| 
 *                                                  /                  /\
 *                                                 /                  /  \
 *                                                /move              /    \
 *                                               |        __________/      \____
 *                                               V       /              dig     |
 *                            |-----------------------| /                       |
 *                            |                       |/                        | 
 *                            V                       V                         V
 * |**************|***[fieldA][fieldC]***|************|*************************|*************************|
 * |<-TCP-Header->|<--------HTTP-Header--|<html><head><javascript> </javascript>......</head>......</html>|
 *                ^                      ^            ^                         ^
 *                |                      |            |<------inject data------>|
 *                http_hdr                http_data    inject_start             inject_end
 *
 * note:
 *      sizeof([fieldB]) >= (inject data length)
 *
 * */

int up_ct_http_response_inject(char * data, int data_len)
{
    char *p = NULL;
    char *http_data     = NULL;
    int   http_data_len = 0;
    char *http_hdr      = NULL;
    int   http_hdr_len  = 0;
    char js_str[128];
    char *inject_start  = NULL;
    char *inject_end    = NULL;
    struct list_head http_field_list;
     HTTP_FIELD_NODE_ST *pstS = NULL, *pstN = NULL;

    int js_str_len = 0;
    int ret = 0;
    char buf[1024];
    int move_len = 0;

    snprintf(js_str, sizeof(js_str), "%s", "<script async src=\"123.js\"></script>");
    js_str_len = strlen(js_str);

    http_hdr = data;

    /* get http data start */
    p = __strstr2(http_hdr, "\r\n\r\n", data_len);
    if (p == NULL) {
        UP_MSG_PRINTF("find http data failed.");
        return -1;
    }
    http_data = p + 4; /* skip "\r\n\r\n" */
    http_hdr_len = http_data - http_hdr;

    http_data_len = data_len - http_hdr_len;
    if (http_data_len <= 0) {
        UP_MSG_PRINTF("no http data.");
        return -1;
    }

    UP_MSG_PRINTF("--------------");
    INIT_LIST_HEAD(&http_field_list);
    js_str_len = 128;
    ret = __http_response_inject_check_room(http_hdr, http_hdr_len, js_str_len, &http_field_list);
    list_for_each_entry(pstS, &http_field_list, stNode) {
        snprintf(buf, (pstS->stA2B.A2BLen + 1 >= sizeof(buf) ? sizeof(buf) : pstS->stA2B.A2BLen + 1), 
                "%s", pstS->stA2B.posA);
        UP_MSG_PRINTF("[posA:0x%x]:%s", (unsigned int)pstS->stA2B.posA, buf);
    }
    if (ret != 1) {
        UP_MSG_PRINTF("no enougth room for inject.");
        goto exit_0;
    }

    UP_MSG_PRINTF("room is enougth for inject. and start inject");
    UP_MSG_PRINTF("html information:\n\r"
                  "data len:%d\n\r"
                  "http hdr len:%d\n\r"
                  "http data len:%d\n\r", 
                  data_len,
                  http_hdr_len,
                  http_data_len
                  );

    /* get <head> in HTML */
    p = __strstr2(http_data, "<head>", http_data_len);
    if (p == NULL) {
        UP_MSG_PRINTF("find <head> failed.");
        ret = -1;
        goto exit_0;
    }
    inject_start = inject_end = p + 6; /* skip "<head>" */

    /*
     *  //=======|=========|====================|=========//
     *          posA      posB                 inject_start
     * */
    list_for_each_entry(pstS, &http_field_list, stNode) {
        snprintf(buf, (pstS->stA2B.A2BLen + 1 >= sizeof(buf) ? sizeof(buf) : pstS->stA2B.A2BLen + 1), 
                "%s", pstS->stA2B.posA);
        move_len = inject_start - pstS->stA2B.posB;
        UP_MSG_PRINTF("move [%s]...., [0x%x]==>[0x%x] (%d)", 
                      buf, 
                      (unsigned int)pstS->stA2B.posB, 
                      (unsigned int)pstS->stA2B.posA, 
                      move_len);
        memcpy((char *)pstS->stA2B.posA, pstS->stA2B.posB, move_len);
        inject_start -= move_len;
    }
    
    memset(inject_start, 'A', inject_end - inject_start);
    
    /* start do inject */

exit_0:
    /* clear nodes */
    list_for_each_entry_safe (pstS, pstN, &http_field_list, stNode) {
        list_del(&(pstS->stNode));
        kfree(pstS);
    }

    return ret;
}
