
#include <asm/io.h>
#define TF_MSG_PRINTF(fmt, ...) \
    printk("[%s][%d]", __func__, __LINE__); \
    printk(fmt, ##__VA_ARGS__); \
    printk("\n")


#define TF_SKB_MARK_MASK             0x0000FFFF
#define TF_SKB_MARK_NEED_FAKE_FIN    0x00000001
