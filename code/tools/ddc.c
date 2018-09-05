#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#define IEEE80211_MSG_MAX 63
#define IEEE80211_MSG_SMARTANT 7            /* Bit 7 (0x80)for Smart Antenna debug */

#define    N(a)    (sizeof (a) / sizeof (a[0]))

enum {
    /* IEEE80211_PARAM_DBG_LVL */
    /* DBG LVL 0 (zero) is free */
    IEEE80211_MSG_ACS = 1,                  /* auto channel selection */
    IEEE80211_MSG_SCAN_SM,                  /* scan state machine */
    IEEE80211_MSG_SCANENTRY,                /* scan entry */
    IEEE80211_MSG_WDS,                      /* WDS handling */
    IEEE80211_MSG_ACTION,                   /* action management frames */
    IEEE80211_MSG_ROAM,                     /* sta-mode roaming */
    IEEE80211_MSG_INACT,                    /* inactivity handling */
    IEEE80211_MSG_DOTH      = 8,            /* 11.h */
    IEEE80211_MSG_IQUE,                     /* IQUE features */
    IEEE80211_MSG_WME,                      /* WME protocol */
    IEEE80211_MSG_ACL,                      /* ACL handling */
    IEEE80211_MSG_WPA,                      /* WPA/RSN protocol */
    IEEE80211_MSG_RADKEYS,                  /* dump 802.1x keys */
    IEEE80211_MSG_RADDUMP,                  /* dump 802.1x radius packets */
    IEEE80211_MSG_RADIUS,                   /* 802.1x radius client */
    IEEE80211_MSG_DOT1XSM   = 16,           /* 802.1x state machine */
    IEEE80211_MSG_DOT1X,                    /* 802.1x authenticator */
    IEEE80211_MSG_POWER,                    /* power save handling */
    IEEE80211_MSG_STATE,                    /* state machine */
    IEEE80211_MSG_OUTPUT,                   /* output handling */
    IEEE80211_MSG_SCAN,                     /* scanning */
    IEEE80211_MSG_AUTH,                     /* authentication handling */
    IEEE80211_MSG_ASSOC,                    /* association handling */
    IEEE80211_MSG_NODE      = 24,           /* node handling */
    IEEE80211_MSG_ELEMID,                   /* element id parsing */
    IEEE80211_MSG_XRATE,                    /* rate set handling */
    IEEE80211_MSG_INPUT,                    /* input handling */
    IEEE80211_MSG_CRYPTO,                   /* crypto work */
    IEEE80211_MSG_DUMPPKTS,                 /* IFF_LINK2 equivalant */
    IEEE80211_MSG_DEBUG,                    /* IFF_DEBUG equivalent */
    IEEE80211_MSG_MLME,                     /* MLME */
    /* IEEE80211_PARAM_DBG_LVL_HIGH */
    IEEE80211_MSG_RRM       = 32,           /* Radio resource measurement */
    IEEE80211_MSG_WNM,                      /* Wireless Network Management */
    IEEE80211_MSG_P2P_PROT,                 /* P2P Protocol driver */
    IEEE80211_MSG_PROXYARP,                 /* 11v Proxy ARP */
    IEEE80211_MSG_L2TIF,                    /* Hotspot 2.0 L2 TIF */
    IEEE80211_MSG_WIFIPOS,                  /* WifiPositioning Feature */
    IEEE80211_MSG_WRAP,                     /* WRAP or Wireless ProxySTA */
    IEEE80211_MSG_DFS,                      /* DFS debug mesg */
    IEEE80211_MSG_ATF       = 40,           /* ATF debug mesg */
    IEEE80211_MSG_SPLITMAC,                 /* splitmac debug */
    IEEE80211_MSG_IOCTL,                    /* ioctl */
    IEEE80211_MSG_NAC,                      /* nac debug */
    IEEE80211_MSG_MESH,                     /* mesh debug */
    IEEE80211_MSG_MBO,                      /* MBO debug mesg */
    IEEE80211_MSG_EXTIOCTL_CHANSWITCH  = 46,/* extended IOCTL chan switch debug */
    IEEE80211_MSG_EXTIOCTL_CHANSSCAN  = 47, /* extended IOCTL chan scan debug */

    IEEE80211_MSG_NUM_CATEGORIES,           /* total ieee80211 messages */
    IEEE80211_MSG_UNMASKABLE = IEEE80211_MSG_MAX,  /* anything */
    IEEE80211_MSG_ANY = IEEE80211_MSG_MAX,  /* anything */
};

typedef struct dbgLVL_tag
{
    int category_bit;
    char *category_name;
    char *desc; 
}dbgLVL_INFO_ST;

static dbgLVL_INFO_ST dbgLVL_info_list[] = 
{
    {IEEE80211_MSG_ACS,                     "ACS",                      "auto channel selection"},
    {IEEE80211_MSG_SCAN_SM,                 "SCAN_SM",                  "scan state machine"},
    {IEEE80211_MSG_SCANENTRY,               "SCANENTRY",                "scan entry"},
    {IEEE80211_MSG_WDS,                     "WDS",                      "WDS handling"},
    {IEEE80211_MSG_ACTION,                  "ACTION",                   "action management frames"},
    {IEEE80211_MSG_ROAM,                    "ROAM",                     "sta-mode roaming"},
    {IEEE80211_MSG_INACT,                   "INACT",                    "inactivity handling"},
    {IEEE80211_MSG_DOTH,                    "DOTH",                     "11.h"},
    {IEEE80211_MSG_IQUE,                    "IQUE",                     "IQUE features"},
    {IEEE80211_MSG_WME,                     "WME",                      "WME protocol"},
    {IEEE80211_MSG_ACL,                     "ACL",                      "ACL handling"},
    {IEEE80211_MSG_WPA,                     "WPA",                      "WPA/RSN protocol"},
    {IEEE80211_MSG_RADKEYS,                 "RADKEYS",                  "dump 802.1x keys"},
    {IEEE80211_MSG_RADDUMP,                 "RADDUMP",                  "dump 802.1x radius packets"},
    {IEEE80211_MSG_RADIUS,                  "RADIUS",                   "802.1x radius client"},
    {IEEE80211_MSG_DOT1XSM,                 "DOT1XSM",                  "802.1x state machine"},
    {IEEE80211_MSG_DOT1X,                   "DOT1X",                    "802.1x authenticator"},
    {IEEE80211_MSG_POWER,                   "POWER",                    "power save handling"},
    {IEEE80211_MSG_STATE,                   "STATE",                    "state machine"},
    {IEEE80211_MSG_OUTPUT,                  "OUTPUT",                   "output handling"},
    {IEEE80211_MSG_SCAN,                    "SCAN",                     "scanning"},
    {IEEE80211_MSG_AUTH,                    "AUTH",                     "authentication handling"},
    {IEEE80211_MSG_ASSOC,                   "ASSOC",                    "association handling"},
    {IEEE80211_MSG_NODE,                    "NODE",                     "node handling"},
    {IEEE80211_MSG_ELEMID,                  "ELEMID",                   "element id parsing"},
    {IEEE80211_MSG_XRATE,                   "XRATE",                    "rate set handling"},
    {IEEE80211_MSG_INPUT,                   "INPUT",                    "input handling"},
    {IEEE80211_MSG_CRYPTO,                  "CRYPTO",                   "crypto work"},
    {IEEE80211_MSG_DUMPPKTS,                "DUMPPKTS",                 "IFF_LINK2 equivalant"},
    {IEEE80211_MSG_DEBUG,                   "DEBUG",                    "IFF_DEBUG equivalent"},
    {IEEE80211_MSG_MLME,                    "MLME",                     "MLME"},
    {IEEE80211_MSG_RRM,                     "RRM",                      "Radio resource measurement"},
    {IEEE80211_MSG_WNM,                     "WNM",                      "Wireless Network Management"},
    {IEEE80211_MSG_P2P_PROT,                "P2P_PROT",                 "P2P Protocol driver"},
    {IEEE80211_MSG_PROXYARP,                "PROXYARP",                 "11v Proxy ARP"},
    {IEEE80211_MSG_L2TIF,                   "L2TIF",                    "Hotspot 2.0 L2 TIF"},
    {IEEE80211_MSG_WIFIPOS,                 "WIFIPOS",                  "WifiPositioning Feature"},
    {IEEE80211_MSG_WRAP,                    "WRAP",                     "WRAP or Wireless ProxySTA"},
    {IEEE80211_MSG_DFS,                     "DFS",                      "DFS debug mesg"},
    {IEEE80211_MSG_ATF,                     "ATF",                      "ATF debug mesg"},
    {IEEE80211_MSG_SPLITMAC,                "SPLITMAC",                 "splitmac debug"},
    {IEEE80211_MSG_IOCTL,                   "IOCTL",                    "ioctl"},
    {IEEE80211_MSG_NAC,                     "NAC",                      "nac debug"},
    {IEEE80211_MSG_MESH,                    "MESH",                     "mesh debug"},
    {IEEE80211_MSG_MBO,                     "MBO",                      "MBO debug mesg"},
    {IEEE80211_MSG_EXTIOCTL_CHANSWITCH,     "EXTIOCTL_CHANSWITCH",      "extended IOCTL chan switch debug"},
    {IEEE80211_MSG_EXTIOCTL_CHANSSCAN ,     "EXTIOCTL_CHANSSCAN",      "extended IOCTL chan scan debug"},
                                            
    {IEEE80211_MSG_NUM_CATEGORIES,          "NUM_CATEGORIES",              "total IEEE80211 messages"},
    {IEEE80211_MSG_UNMASKABLE,              "IEEE80211_MSG_UNMASKABLE",              "anything"},
    {IEEE80211_MSG_ANY,                     "IEEE80211_MSG_ANY",                     "anything"},
};

int dbgLVL_get_category_bit(const char *category_name)
{
    int i = 0;

    for (i = 0; i < N(dbgLVL_info_list); i++) {
        if (strncmp(category_name, dbgLVL_info_list[i].category_name, strlen(category_name) + 1) == 0) {
            printf("open [%s]:%s\n", dbgLVL_info_list[i].category_name, dbgLVL_info_list[i].desc);
            return dbgLVL_info_list[i].category_bit;
        }
    }

    return -1;
}

int main(int argc, char **argv)
{
    char opt = 0;
    char dbgLVL_strs[1024] = {0};

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd':
                snprintf(dbgLVL_strs, sizeof(dbgLVL_strs), "%s", optarg);
                break;
            default:
                printf("invalid parameter.\n");
                return -1;
        }
    }

    printf("dbgLVL_strs:%s\n", dbgLVL_strs);

    char *outer_ptr=NULL;
    char *p1 = NULL;
    char *p = NULL;
    int bit = 0;
    unsigned int dbgLVL_hex = 0x0;

    if (dbgLVL_strs[0] != '\0') {
        if((p1 = strstr(dbgLVL_strs, "dbgLVL:"))) {
            p1 += strlen("dbgLVL:");
            printf("dbgLVL_strs:%s\n", p1);
            while((p = strtok_r(p1, ",", &outer_ptr)) != NULL) {
                //printf("::%s\n",p);
                bit = dbgLVL_get_category_bit(p);
                if (bit > 0) {
                   dbgLVL_hex |= (1 << bit); 
                } else {
                    printf("bad category[%s]!\n", p);
                }

                p1 = NULL;
            }
            printf("dbgLVL hex : 0x%08X\n", dbgLVL_hex);
        }
    }

    return 0;
}
