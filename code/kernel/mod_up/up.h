

#define UP_MSG_PRINTF(fmt, ...) \
    printk("[%s][%d]", __func__, __LINE__); \
    printk(fmt, ##__VA_ARGS__); \
    printk("\n")

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(m) (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5]

enum {
    __UP_ATTR_INVALID,

    UP_ATTR_TYPE,
    UP_ATTR_MAC,
    UP_ATTR_SEQUENCE,

    __UP_ATTR_MAX,
};
/* Don't bigger than 32! */
#define UP_ATTR_MAX (__UP_ATTR_MAX - 1)

enum {
    __UP_OPS_INVALID,

    UP_OPS_REPORT_PID,

    __UP_OPS_MAX,
};
#define UP_OPS_MAX (__UP_OPS_MAX - 1)
