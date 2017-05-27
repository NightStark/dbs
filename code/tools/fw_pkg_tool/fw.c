#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <openssl/evp.h>

#include "list.h"
#include "fw.h"

#define FWTOOL_MAGIC    0x48445A4E

#define FWTOOL_PTNAME_SZ    32
#define FWTOOL_DEVNAME_SZ   32

struct fw_pkg_info
{
    char in_file[256];    /* -i */
    char out_file[256];   /* -o */
    char part_name[256];  /* -p */
    char part_file[256];  /* -f */
    char board[256];      /* -b */
    unsigned int hw_ver;  /* -V */
    unsigned int sw_ver;  /* -v */
    unsigned int cc_date; /* -d */
    unsigned int buildno; /* -N */
    char ext_pname[256];  /* -e */
    char show;            /* -s */
    char do_check;        /* -c */
};

static struct fw_pkg_info g_fw_pkg_info;

void dump_data(unsigned char*buff, int count, const char *func, int line)
{       
    int i = 0;
    if (NULL != func) {
        printf("\n================================================\n");
        printf("[%s][%d]\n", func, line);
    }
    for(i = 0; i < count; i++){
        printf("%02X ", buff[i]);
        if ((i + 1) != 1 && (i + 1) % 8 == 0) {
            printf(" ");
            if ((i + 1) != 1 && (i + 1) % 16 == 0) {
                printf("\n");
            }
        }
    }   
    if (NULL != func) {
        printf("\n");
        printf("================================================\n");
    }
    return;
}   

struct pt_hdr {
    unsigned int magic;
    unsigned int size; /* exclude hdr */
    char name[FWTOOL_PTNAME_SZ];
    unsigned int data_addr;
    unsigned int next_offset;
    unsigned char data[0];
}__attribute__ ((packed));

struct fw_hdr {
    unsigned int magic;
    char board[FWTOOL_PTNAME_SZ];
    unsigned int sw_ver;
    unsigned short hw_ver;
    unsigned char flags;
    unsigned int crc;
    unsigned int size;
    unsigned int cc_date;   /* the date of compiling firmware */
    unsigned int buildno;   /* the build number of compiling firmware */
    unsigned int partitions;
    unsigned char hash256[32];
    unsigned char sign[256];
}__attribute__ ((packed));

struct fw_check_info {
    struct fw_hdr fw_hdr;
    struct pt_hdr pt_hdr[0];
};

struct pt_list
{
    struct list_head node;
    struct pt_hdr *phdr;
    int f_len; /* include header */
};

static struct list_head g_pt_list;
static struct fw_hdr g_fw_hdr;

#define FWTOOL_PTHDR_SZ     80
#define FWTOOL_FWHDR_SZ     128

#define FWTOOL_ALIGN_SZ     8

#define FWTOOL_PT_MAX       16


/* @md_value: return hash256 values
 * */
int do_hash256(unsigned char *md_value)
{
    int i;
    EVP_MD_CTX mdctx;
    const EVP_MD *md;
    unsigned int md_len;
    const char *digest = "sha256";
    struct pt_list *ptl = NULL;

    OpenSSL_add_all_digests();


    md = EVP_get_digestbyname(digest);

    if(!md) {
        FW_LOG("Unknown message digest %s\n", digest);
        return -1;
    }

    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);

    EVP_DigestUpdate(&mdctx, &g_fw_hdr, sizeof(g_fw_hdr));
    __DUMP_DATA((unsigned char *)&g_fw_hdr, sizeof(g_fw_hdr));

    list_for_each_entry(ptl, &g_pt_list, node) {
        EVP_DigestUpdate(&mdctx, ptl->phdr, ptl->f_len);
        __DUMP_DATA((unsigned char *)ptl->phdr, ptl->f_len);
    }

    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    printf("Digest(sha256) is[%d]: ", md_len);
    for(i = 0; i < md_len; i++) printf("%02x", md_value[i]);
    printf("\n");

    return md_len;
}

struct pt_list * partition_header_create(void)
{
    struct pt_list *pth = NULL;

    pth = malloc(sizeof(struct pt_list));
    if (NULL == pth) {
        FW_LOG("oom.");
        return NULL;
    }
    memset(pth, 0, sizeof(struct pt_list));

    return pth;
}

static FILE *g_fw_fp = NULL;
static FILE *g_fw_new_fp = NULL;

static int parse_fw_partition(void)
{
    int ret = 0;
    int cnt = 0;
    struct pt_hdr pth;
    struct pt_list *ptl = NULL;

    while (cnt < g_fw_hdr.partitions) {
        /*skip fireware header */
        //fseek(g_fw_fp, sizeof(g_fw_fp), SEEK_SET);
        ret = fread(&pth, 1, sizeof(pth), g_fw_fp);
        if (ret != sizeof(pth)) {
            FW_LOG("read pt header failed.");
            return -1;
        }

        if (pth.magic != FWTOOL_MAGIC) {
            FW_LOG("invalide partition magic 0x%08X != 0x%08X.", pth.magic, FWTOOL_MAGIC);
            return -1;
        }

        FW_LOG("partition infomation:\n\t"
                "name:%s\n\t"
                "size:%d\n\t"
                "nextoffset:%d\n\t",
                pth.name, pth.size, pth.next_offset);

        ptl = partition_header_create();
        if (NULL == ptl) {
            return -1;
        }

        //ptl->f_len = sizeof(struct pt_hdr) + pth.size;
        ptl->f_len = pth.size + sizeof(*ptl->phdr);
        ptl->phdr = malloc(ptl->f_len);
        if (ptl->phdr == NULL) {
            FW_LOG("oom."); 
            return -1;
        }
        memcpy(ptl->phdr, &pth, sizeof(pth));
        ret = fread((char *)(ptl->phdr) + sizeof(pth), 1, pth.size, g_fw_fp);
        if (ret != pth.size) {
            FW_LOG("read pt data failed.");
            return -1;
        }

        list_add_tail(&(ptl->node), &g_pt_list);
        g_fw_hdr.size += ptl->f_len;

        if (ptl->phdr->next_offset == 0) {
            FW_LOG("list to end.");
            break;
        }

        cnt ++;
    }
    
    return 0;
error:
    //TODO destroy ptl
    return -1;
}

#ifndef EXT_PKG
int rsa_sign_verify(unsigned char *md, int md_len, 
                    unsigned char *sigret, int siglen)
{ 
    return -1;
}
#endif
#ifndef BUILD_PKG 
int rsa_sign_generate(unsigned char *md, int md_len,
                      unsigned char **sig, int *len)
{ 
    return -1;
}
#endif

static int parse_fw_target(void)
{
    int ret = 0;
    unsigned char md_value_old[EVP_MAX_MD_SIZE] = {0};
    unsigned char md_value[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len;
    int len_t = 0;
    unsigned char sigret_old[256] = {0};
    int sigret_len_old = 0;


    fseek(g_fw_fp, 0, SEEK_SET);
    FW_LOG("seek:%ld", ftell(g_fw_fp));
    ret = fread(&g_fw_hdr, 1, sizeof(g_fw_hdr), g_fw_fp);
    if (ret == sizeof(g_fw_fp)) {
        FW_LOG("read fw header failed.");
        return -1;
    }

    if (g_fw_hdr.magic != FWTOOL_MAGIC) { //TODO:bigend and littlend
        FW_LOG("fw magic check failed 0x%08X != 0x%08X.", g_fw_hdr.magic, FWTOOL_MAGIC);
        return -1;
    }

    FW_LOG("fireware infomation:\n\t"
           "magic:0x%08X\n\t"
           "board name:%s\n\t"
           "sw ver:0x%08X\n\t"
           "hw ver:0x%08X\n\t"
           "crc:0x%08X\n\t"
           "size:0x%08X\n\t"
           "build No.:0x%08X\n\t"
           "build data:0x%08X\n\t"
           "partitions:%d\n\t",
           g_fw_hdr.magic,
           g_fw_hdr.board,
           g_fw_hdr.sw_ver,
           g_fw_hdr.hw_ver,
           g_fw_hdr.crc,
           g_fw_hdr.size,
           g_fw_hdr.buildno,
           g_fw_hdr.cc_date,
           g_fw_hdr.partitions); 

#define VER_FMT "%d.%d.%d"
#define PRINT_VER(v) (((v) >> 24) & 0xFF), (((v) >> 16) & 0xFF), (((v) >> 0) & 0xFFFF)
    if (g_fw_pkg_info.show) {
        printf("fireware infomation:\n\t"
                "magic:0x%08X\n\t"
                "board name:%s\n\t"
                "sw ver:"VER_FMT"\n\t"
                "hw ver:"VER_FMT"\n\t"
                "crc:0x%08X\n\t"
                "size:%d\n\t"
                "build No.:%d\n\t"
                "build data:0x%08X\n\t"
                "partitions:%d\n",
                g_fw_hdr.magic,
                g_fw_hdr.board,
                PRINT_VER(g_fw_hdr.sw_ver),
                PRINT_VER(g_fw_hdr.hw_ver),
                g_fw_hdr.crc,
                g_fw_hdr.size,
                g_fw_hdr.buildno,
                g_fw_hdr.cc_date,
                g_fw_hdr.partitions); 
    }

    len_t = g_fw_hdr.size;
    g_fw_hdr.size = 0;

    g_fw_hdr.size += sizeof(g_fw_hdr);
    parse_fw_partition();

    if (len_t != g_fw_hdr.size) {
        printf("fireware size check failed.\n");
    }

    memcpy(md_value_old, g_fw_hdr.hash256, sizeof(g_fw_hdr.hash256));
    memset(g_fw_hdr.hash256, 0, sizeof(g_fw_hdr.hash256));
    sigret_len_old = sizeof(g_fw_hdr.sign);
    memcpy(sigret_old, g_fw_hdr.sign, sizeof(sigret_old));
    memset(g_fw_hdr.sign, 0, sizeof(g_fw_hdr.sign));

    md_len = do_hash256(md_value);
    if (memcmp(md_value_old, md_value, sizeof(md_value_old)) != 0) {
        printf("hash check failed.\n");
        return -1;
    }
    if (rsa_sign_verify(md_value_old, md_len, sigret_old, sigret_len_old)) {
        printf("ras sign verify failed, invalid fireware.\n");
        return -1;
    }
    
    return 0;
}

int open_fw_target_new(const char *tmp_name)
{
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "rm -rfv %s", tmp_name);
    system(cmd);

    g_fw_new_fp = fopen(tmp_name, "a+");
    if (NULL == g_fw_new_fp) {
        FW_LOG("open %s failed.", tmp_name);
        return -1;
    }

    return 0;
}

int open_fw_target(const char *fwname)
{
    struct stat f_stat;

    if (access(fwname, F_OK | R_OK | W_OK) != 0) {
        FW_LOG("access %s failed. make sure it is exist.", fwname);
        return -1;
    }

    if(stat(fwname, &f_stat)<0) {
        FW_LOG("Open file %s failed!\n", fwname);
        goto error;
    }

    if (f_stat.st_size == 0) {
        FW_LOG("need create a new fw.");
        return 1;
    }

    if (f_stat.st_size < sizeof(struct fw_hdr)) {
        FW_LOG("invalid fireware image.");
        goto error;
    }

    /* open with "a+"
     * Open  for  reading  and  appending  (writing at end of file).  
     * The file is created if it does not exist.  
     * The initial file position for reading is at the beginning of the
     * file, but output is always appended to the end of the file.
     * */
    g_fw_fp = fopen(fwname, "a+");
    if (NULL == g_fw_fp) {
        FW_LOG("open %s failed.", fwname);
        return -1;
    }

    if (parse_fw_target() < 0) {
        FW_LOG("parse fw failed.");
        goto error;
    }

    return 0;
error:
    if (g_fw_fp) {
        fclose(g_fw_fp);
        g_fw_fp = NULL;
    }
    return -1;
}

int create_fireware_header(void)
{
    g_fw_hdr.magic = FWTOOL_MAGIC;
    snprintf(g_fw_hdr.board, sizeof(g_fw_hdr.board), "%s", g_fw_pkg_info.board);
    g_fw_hdr.sw_ver  = g_fw_pkg_info.sw_ver;
    g_fw_hdr.hw_ver  = g_fw_pkg_info.hw_ver;
    g_fw_hdr.buildno = g_fw_pkg_info.buildno;

    g_fw_hdr.size += sizeof(g_fw_hdr);

    return 0;
}

static int partition_name_dup_check(const char *ptname)
{
    struct pt_list *ptl = NULL;

    list_for_each_entry(ptl, &g_pt_list, node) {
        if (strncmp(ptl->phdr->name, ptname, (strlen(ptl->phdr->name) + 1)) == 0) {
            return 1;
        }
    }

    return 0;
}

static int add_partition(const char *pfname, const char *pname) 
{
    int ret = 0;
    FILE *p_fp = NULL;
    struct pt_list *pt_l_p = NULL;
    struct pt_list *pt_l = NULL;
    unsigned int p_offset = 0;
    struct stat f_stat;

    if (partition_name_dup_check(pname)) {
        FW_LOG("partition name %s is exist!\n", pname);
        return -1;
    }

    if(stat(pfname, &f_stat)<0) {
        FW_LOG("stat file %s failed!\n", pfname);
        return -1;
    }

    if (list_empty(&g_pt_list)) {
        p_offset = sizeof(g_fw_hdr);
    } else {
        pt_l_p = list_last_entry(&g_pt_list, struct pt_list, node);
        p_offset = pt_l_p->phdr->next_offset;
    }

    pt_l = partition_header_create();
    if (NULL == pt_l) {
        goto error;
    }

    pt_l->f_len = f_stat.st_size + sizeof(*pt_l->phdr);
    pt_l->phdr = malloc(pt_l->f_len);
    if (NULL == pt_l->phdr) {
        FW_LOG("oom.");
        goto error;
    }
    memset(pt_l->phdr, 0, pt_l->f_len);

    snprintf(pt_l->phdr->name, sizeof(pt_l->phdr->name), "%s", pname);
    pt_l->phdr->magic = FWTOOL_MAGIC;
    pt_l->phdr->size = f_stat.st_size;
    pt_l->phdr->next_offset = p_offset + pt_l->f_len;

    FW_LOG("hdr size = %d", (unsigned int)sizeof(*pt_l->phdr));
    FW_LOG("p size = %d", pt_l->phdr->size);

    p_fp = fopen(pfname, "r");
    if (NULL == p_fp) {
        FW_LOG("open %s failed.", pfname);
        return -1;
    }

    ret = fread(pt_l->phdr->data, 1, pt_l->phdr->size, p_fp);
    if (ret != pt_l->phdr->size) {
        FW_LOG("read failed.ret = %d", ret);
        goto error;
    }

    /*
    fseek(g_fw_fp, p_offset, SEEK_SET);
    ret = fwrite(pt_l->phdr, 1, pt_l->f_len, g_fw_fp);
    if (ret != pt_l->f_len) {
        FW_LOG("write header failed.");
        goto error;
    }
    */

    list_add_tail(&pt_l->node, &g_pt_list);
    g_fw_hdr.size += pt_l->f_len;

    g_fw_hdr.partitions++;

    fclose(p_fp);
    p_fp = NULL;

    return 0;
    
error:
    if (pt_l) {
        //destroy
    }
    if (p_fp) {
        fclose(p_fp);
        p_fp = NULL;
    }
    return -1;
}

static int add_sign_partition(void)
{
     return 0;
}

static int build_new_fw(void)
{
    int ret = 0;
    struct pt_list *ptl = NULL;
    unsigned char md_value[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len;
    int siglen = 0;
    unsigned char *sigret = NULL;

    #if 0
    ptl = list_last_entry(&g_pt_list, struct pt_list, node);
    ptl->phdr->next_offset = 0;
    #endif

    md_len = do_hash256(md_value);
    if (md_len < 0) {
        FW_LOG("do hash 256 failed.");
        return -1;
    }
    memcpy(g_fw_hdr.hash256, md_value, sizeof(g_fw_hdr.hash256));
    if (rsa_sign_generate(md_value, md_len, &sigret, &siglen) , 0) {
        FW_LOG("do rsa sign failed.");
        return -1;
    }
    memcpy(g_fw_hdr.sign, sigret, sizeof(g_fw_hdr.sign));

    //TODO add a sign partiton? and drop this partiton when extract

    ret = fwrite(&g_fw_hdr, 1, sizeof(g_fw_hdr), g_fw_new_fp);
    if (ret != sizeof(g_fw_hdr)) {
        FW_LOG("write fw header failed.");
        return -1;
    }

    list_for_each_entry(ptl, &g_pt_list, node) {
        ret = fwrite(ptl->phdr, 1, ptl->f_len, g_fw_new_fp);
        if (ret != ptl->f_len) {
            FW_LOG("write pt failed.");
            return -1;
        }
    }

    return 0;
}

static void fwtool_usage()
{
    printf( "\nusage:\n"
            " -o <file> : output file name.\n"
            " -i <file> : input file name.\n"
            " -p <name> : partition name.\n"
            " -f <file> : partition file name. \n"
            " -b <name> : board name.\n"
            " -N <No>   : build number.\n"
            " -d <date> : compile date(Format:YYYYMMDDHH, eg.2014042110).\n"
            " -V <ver>  : hardware verion.\n"
            " -v <ver>  : software verion.\n"
            " -e <pname>: extract part, all extract all.\n"
            " -s        : show fireware infomation.\n"
            " -c        : check fireware valid.\n"
            " -h        : show this helper.\n"
            "create a new firmware:\n\t"
            "fw_builder -o <fw.bin> -p <test7> -f <t.bin> -b <GW3200>\n\t"
            "eg:\n\t\tfw_builder -o fw.bin -p test3 -f t.bin -b GW3200\n"
            "appended a file to firmware:\n\t"
            "fw_builder -o <fw.bin> -p <test7> -f <t.bin> -b <GW3200> -i <fw.bin>\n\t"
            "eg:\n\t\tfw_builder -o fw.bin -p test3 -f t.bin -b GW3200 -i fw.bin\n"
            "extract a partition:\n\t"
            "fw_builder -i <fw.bin> -e <pname>\n"
            "extract all partition:\n\t"
            "fw_builder -i <fw.bin> -e all\n\n");
    return;
}

int get_opt(int argc, char **argv)
{
    int ret = 0;
    char opt = 0;
    char *arg = NULL;
    int i = 0, j = 0, m = 0;
    int ext = 0;
    int show = 0;
    int do_check = 0;

    while ((opt = getopt(argc, argv, "i:o:p:f:b:V:v:d:N:e:hsT")) != -1) {
        switch (opt) {
            case 'i':
                arg = optarg;
                snprintf(g_fw_pkg_info.in_file, sizeof(g_fw_pkg_info.in_file), "%s", arg);
                break;
            case 'o':
                arg = optarg;
                snprintf(g_fw_pkg_info.out_file, sizeof(g_fw_pkg_info.out_file), "%s", arg);
                break;
            case 'p':
                arg = optarg;
                snprintf(g_fw_pkg_info.part_name, sizeof(g_fw_pkg_info.part_name), "%s", arg);
                break;
            case 'f':
                arg = optarg;
                snprintf(g_fw_pkg_info.part_file, sizeof(g_fw_pkg_info.part_file), "%s", arg);
                break;
            case 'b':
                arg = optarg;
                snprintf(g_fw_pkg_info.board, sizeof(g_fw_pkg_info.board), "%s", arg);
                break;
            case 'V':
                if (strchr(optarg, '.')) {
                    if (sscanf(optarg, "%d.%d.%d", &i, &j, &m) < 3)
                        return -1;
                    g_fw_pkg_info.hw_ver = ((i&0xff)<<24)|((j&0xff)<<16)|(m&0xffff);
                } else {
                    g_fw_pkg_info.hw_ver = atol(optarg);
                }
                break;
            case 'v':
                if (strchr(optarg, '.')) {
                    if (sscanf(optarg, "%d.%d.%d", &i, &j, &m) < 3)
                        return -1;
                    g_fw_pkg_info.sw_ver = ((i&0xff)<<24)|((j&0xff)<<16)|(m&0xffff);
                } else {
                    g_fw_pkg_info.sw_ver = atol(optarg);
                }
                break;
            case 'd':
                g_fw_pkg_info.cc_date = atol(optarg);
                break;
            case 'N':
                g_fw_pkg_info.buildno = atol(optarg);
                break;
            case 'e':
                arg = optarg;
                snprintf(g_fw_pkg_info.ext_pname, sizeof(g_fw_pkg_info.ext_pname), "%s", arg);
                ext = 1;
                break;
            case 's':
                g_fw_pkg_info.show = 1;
                show = 1;
                break;
            case 'c':
                g_fw_pkg_info.do_check = 1;
                do_check = 1;
                break;
            case 'h':
                fwtool_usage();
                exit(0);
            default:
                fwtool_usage();
                return -1;
        }
    }

    if (!show && !ext && g_fw_pkg_info.out_file[0] == '\0') {
        printf("error -o is need.\n");
        ret = -1;
    }

    if (ext && g_fw_pkg_info.in_file[0] == '\0') {
        printf("error -i is need.\n");
        ret = -1;
    }

    if (!show && !ext && g_fw_pkg_info.part_name[0] == '\0') {
        printf("error -p is need.\n");
        ret = -1;
    }

    if (!show && !ext && g_fw_pkg_info.part_name[0] == '\0') {
        printf("error -f is need.\n");
        ret = -1;
    }

    if (!show && !ext && g_fw_pkg_info.board[0] == '\0') {
        printf("error -b is need.\n");
        ret = -1;
    }
    
    return ret;
}

static int extract_fw_partition(struct pt_list *ptl)
{
    int ret = 0;
    char pfname[256];
    FILE *p_fp = NULL;

    snprintf(pfname, sizeof(pfname), "_%s/%s.bin", 
            g_fw_pkg_info.in_file,
            ptl->phdr->name);

    p_fp = fopen(pfname, "a+");
    if (NULL == p_fp) {
        printf("open %s failed.", pfname);
        return -1;
    }

    ret = fwrite(ptl->phdr->data, 1, ptl->phdr->size, p_fp);
    if (ret != ptl->phdr->size) {
        printf("extract partition %s failed.", ptl->phdr->name);
        goto exit;
    }

    fclose(p_fp);
    p_fp = NULL;

    printf("extract partition %s ==> %s\n", ptl->phdr->name, pfname);

    return 0;
exit:
    if (p_fp) {
        fclose(p_fp);
        p_fp = NULL;
    }

    return -1;
}

static int extract_fw_pkg(void)
{
    int ret = 0;
    struct pt_list *ptl = NULL;
    int all = 0;
    char cmd[256];

    ret = open_fw_target(g_fw_pkg_info.in_file);
    if (ret < 0) {
        FW_LOG("open %s failed.", g_fw_pkg_info.in_file);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "rm -rfv _%s; mkdir _%s", 
            g_fw_pkg_info.in_file,
            g_fw_pkg_info.in_file);

    system(cmd);

    if (strncmp(g_fw_pkg_info.ext_pname, "all", strlen("all") + 1) == 0) {
        all = 1;
    }

    list_for_each_entry(ptl, &g_pt_list, node) {
        if (all || (strncmp(ptl->phdr->name, g_fw_pkg_info.ext_pname, strlen(ptl->phdr->name) + 1) == 0)) {
            if (extract_fw_partition(ptl) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

int build_fw_pkg(void)
{
    int ret = 0;
    char tmp_name[128];

    snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", g_fw_pkg_info.out_file);

    if (open_fw_target_new(tmp_name) < 0) {
        return -1;
    }

    if (g_fw_pkg_info.in_file[0] == '\0') {
        create_fireware_header();
    } else {
        ret = open_fw_target(g_fw_pkg_info.in_file);
        if (ret < 0) {
            FW_LOG("open fw.bin failed.");
            return -1;
        }
        FW_LOG("if add partition, new partition will append to %s", g_fw_pkg_info.in_file);
    }

    if (g_fw_pkg_info.part_file[0] == '\0') {
        FW_LOG("not part file to add exit.");
        goto exit;
    }

    ret = add_partition(g_fw_pkg_info.part_file, g_fw_pkg_info.part_name);
    if (ret < 0) {
        goto exit;
    }

    ret = build_new_fw();
    if (ret < 0) {
        goto exit;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rfv %s; mv %s %s -fv", 
            g_fw_pkg_info.out_file, 
            tmp_name,
            g_fw_pkg_info.out_file);

    system(cmd);

exit:
    if (g_fw_new_fp) {
        fclose(g_fw_new_fp);
        g_fw_new_fp = NULL;
    }
    if (g_fw_fp) {
        fclose(g_fw_fp);
        g_fw_fp = NULL;
    }
    
    return 0;
}

int main(int argc, char **argv)
{
    memset(&g_fw_hdr, 0, sizeof(g_fw_hdr));
    memset(&g_fw_pkg_info, 0, sizeof(g_fw_pkg_info));
    INIT_LIST_HEAD(&g_pt_list);

    //test_main();
    if (get_opt(argc, argv) < 0) {
        return -1;
    }

    if (g_fw_pkg_info.ext_pname[0] == '\0') {
        #ifdef BUILD_PKG
        build_fw_pkg();
        #else
        printf("not support build firmware.\n");
        #endif
    } else {
        #ifdef EXT_PKG
        extract_fw_pkg();        
        #else
        printf("not support extract fireware.\n");
        #endif
    }
    
    return 0;
}

