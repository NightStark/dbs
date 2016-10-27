#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
//#include <linux/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */
#include <sys/user.h>
#include <inttypes.h>

#include "list.h"
#include "pt.h"


//#define CONFIG_32BIT

/* define in kerner "arch/mips/include/asm/ptrace.h" */
/*
 * This struct defines the way the registers are stored on the stack during a
 * system call/exception. As usual the registers k0/k1 aren't being saved.
 */
struct pt_regs {
#ifdef CONFIG_32BIT
	/* Pad bytes for argument save space on the stack. */
	unsigned long long pad0[6];
#endif

	/* Saved main processor registers. */
    //in kernel arch/mips/kernel/ptrace.c 
    //always use the 64-bit format, even for 32-bit kernels
	unsigned long long regs[32]; 

	/* Saved special registers. */
	unsigned  long  long cp0_status;
	unsigned  long  long hi;
	unsigned  long  long lo;
#ifdef CONFIG_CPU_HAS_SMARTMIPS
	unsigned  long  long acx;
#endif
	unsigned  long  long cp0_badvaddr;
	unsigned  long  long cp0_cause;
	unsigned  long  long cp0_epc; //异常程序计数器 exception program counter
#ifdef CONFIG_MIPS_MT_SMTC
	unsigned long long cp0_tcstatus;
#endif /* CONFIG_MIPS_MT_SMTC */
#ifdef CONFIG_CPU_CAVIUM_OCTEON
	unsigned  long  long mpl[3];        /* MTM{0,1,2} */
	unsigned  long  long mtp[3];        /* MTP{0,1,2} */
#endif
	unsigned long long _safe_pad_[2];  /* must be all zero */
} __attribute__ ((aligned (8)));

static void
__dump_data(unsigned char *ptr, int len, const char *func, int line)
{
    int i;
    printf(" =============dump=============\n");
    printf("\n [%s][%d]", func, line);
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n ==============================\n");
}

#define __DUMP_MEM__(p, len) \
    __dump_data(p, len, __func__, __LINE__)

struct trace_line
{
    struct list_head node;
    unsigned long addr;
    unsigned int line;
    char func[128];
    char lib_path[128];
};

/*
 * 2b09f000-2b0a0000 rw-p 00059000 1f:03 816        /rom/lib/libuClibc-0.9.30.so
	seq_printf(m, "%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu %n",
			vma->vm_start,
			vma->vm_end,
			flags & VM_READ ? 'r' : '-',
			flags & VM_WRITE ? 'w' : '-',
			flags & VM_EXEC ? 'x' : '-',
			flags & VM_MAYSHARE ? 's' : 'p',
			pgoff,
			MAJOR(dev), MINOR(dev), ino, &len);
 * */
struct proc_map
{
    struct list_head node;
    unsigned long int vm_start;
    unsigned long int vm_end;
    unsigned int flags;
    unsigned long long int pgoff;
    unsigned int dev_major;
    unsigned int dev_minor;
    unsigned long int ino;
    char vm_name[128];
};

struct map_flag_desc
{
    unsigned int flag_v;
    unsigned int dis_index; 
    unsigned int dis_off; 
    char dis_char;
};

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

struct map_flag_desc m_flags[] =
{
    [0] = { VM_READ,     0, 'r' - 0, 'r' },
    [1] = { VM_WRITE,    1, 'w' - 1, 'w' },
    [2] = { VM_EXEC,     2, 'x' - 2, 'x' },
    [3] = { VM_MAYSHARE, 3, 's' - 3, 's' },
    [4] = { VM_MAYREAD,  4, 'p' - 4, 'p' },
    [5] = { VM_MAYWRITE, 5, 'p' - 5, 'p' },
    [6] = { VM_MAYEXEC,  6, 'p' - 6, 'p' },
};

int get_proc_maps (int pid, struct list_head *p_maps_head)
{
    int i = 0;
    //struct proc_map p_maps[128];
    FILE *maps_fp = NULL;
    char path[128];
    char line[256];
    unsigned long int vm_start;
    unsigned long int vm_end;
    unsigned long long int pgoff;
    unsigned int dev_major;
    unsigned int dev_minor;
    unsigned long int ino;
    char vm_name[128];
    char flag[4] = {0};
    struct proc_map *p_map = NULL;
    int ret = 0;

    snprintf(path, sizeof(path), "/proc/%d/maps", pid);

    maps_fp = fopen(path, "r");
    if (maps_fp == NULL) {
        return -1;
    }

    while(fgets(line, sizeof(line), maps_fp)) {
        line[strlen(line) -1 ] = '\0';
        //printf("%s\n", line);
	    ret = sscanf(line, "%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu %s",
                &(vm_start),
                &(vm_end),
                &flag[0], 
                &flag[1], 
                &flag[2], 
                &flag[3], 
                &(pgoff),
                &(dev_major),
                &(dev_minor),
                &(ino),
                vm_name
                );
        if (ret < 10) {
            printf("%s %d ret = %d.\n", __func__, __LINE__, ret);
            continue;
        }
        p_map = malloc(sizeof(struct proc_map));
        if (p_map == NULL) {
            goto error;
        }

        memset(p_map, 0, sizeof(struct proc_map));
        p_map->vm_start = vm_start;
        p_map->vm_end   = vm_end;
        p_map->flags |= ((flag[0] == 'r') ? VM_READ  : 0);
        p_map->flags |= ((flag[1] == 'w') ? VM_WRITE : 0);
        p_map->flags |= ((flag[2] == 'x') ? VM_EXEC  : 0);
        p_map->flags |= ((flag[3] == 's') ? VM_MAYSHARE : 0);
        //p_map->flags    = flag[0]; //TODO:
        p_map->pgoff    = pgoff;
        p_map->dev_major= dev_major;
        p_map->dev_minor= dev_minor;
        p_map->ino      = ino;
        if (ret == 11) {
            snprintf(p_map->vm_name, sizeof(p_map->vm_name), "%s", vm_name);
        }
        list_add_tail(&(p_map->node), p_maps_head);

        //printf("---------------------------------------------\n");
        /*
	    printf("%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu %n [%s]\n",
                (p_maps[i].vm_start),
                (p_maps[i].vm_end),
                flag[0], 
                flag[1], 
                flag[2], 
                flag[3],
                (p_maps[i].pgoff),
                (p_maps[i].dev_major),
                (p_maps[i].dev_minor),
                (p_maps[i].ino),
                (&p_maps[i].len),
                p_maps[i].vm_name
                );
        */
        i++;
    }

    fclose(maps_fp);
    maps_fp = NULL;

    return i;
error:
    if (maps_fp) {
        fclose(maps_fp);
        maps_fp = NULL;
    }
    return -1;
}

int ptrace_readdata(pid_t pid,  uint8_t *src, uint8_t *buf, size_t size)    
{    
    uint32_t i, j, remain;    
    uint8_t *laddr;    

    union u {    
        long val;    
        char chars[sizeof(long)];    
    } d;    

    j = size / 4;    
    remain = size % 4;    

    laddr = buf;    

    for (i = 0; i < j; i ++) {    
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);    
        memcpy(laddr, d.chars, 4);    
        src += 4;    
        laddr += 4;    
    }    

    if (remain > 0) {    
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);    
        memcpy(laddr, d.chars, remain);    
    }    

    return 0;    
}  

static int trace_line_insert(struct list_head *trace_list, unsigned long addr)
{
    struct trace_line *t_line = NULL;
    
    t_line = malloc(sizeof(struct trace_line));
    if (t_line == NULL) {
        return -1;
    }
    memset(t_line, 0, sizeof(struct trace_line));
    t_line->addr = addr;
    list_add_tail(&(t_line->node), trace_list);

    return 0;
}

static int trace_line_destroy_all(struct list_head *trace_list)
{
    struct trace_line *t_line = NULL, *n = NULL;

    list_for_each_entry_safe(t_line, n, trace_list, node){
        list_del(&(t_line->node));
        free(t_line);
        t_line = NULL;
    }

    return 0;
}

int __backtrace_proc(struct list_head *trace_list, 
        unsigned long int sp,
        unsigned long int spc,
        unsigned long int stack_start,
        int pid)  
{  
    int ii = 0;
    short slen = 0;
    int stack_len = 0;
    int high = 0;
    int stack_max = 4096;
    unsigned long int ra = 0;   //=eip
    unsigned long int _instruct = 0;
    unsigned long int pc = 0;

    if (trace_list == NULL) {
        printf("invalid parameters.\n");
        return -1;
    }

    pc = spc;

    trace_line_insert(trace_list, (unsigned long)__backtrace_proc);

    //pc = (unsigned long)__getpc();
    printf("%s pc:[%x] sp:%x\n", __func__,
            (unsigned int)pc, (unsigned int)sp);

    while (1) {
        /* find instruct like "400920:   27bdffb8    addiu   sp,sp,-72"
         * "27bdffb8"中后16位为sp偏移的值, 栈都是从高地址向低地址扩展，因此为负值。
         * */
        ii = 0;
        while(1) {
            //_instruct = (*(unsigned long *)pc);
            ptrace_readdata(pid, (uint8_t *)(pc), (uint8_t *)&_instruct, sizeof(_instruct));
            /*
               printf("ii = %x, pc = %x, instruct:%x\n", 
               (unsigned int)(ii * 4), (unsigned int)pc, (unsigned int)_instruct);
               */
            if ((( _instruct & 0xFFFF0000) == 0x27bd0000) || (ii++ > stack_max)) { //FIXME:1024 maybe a good value
                break;
            }
            pc -= 4;
        }

        if (ii >= stack_max) { //TODO: now I know stack_start and stark end from /proc/${pid}/maps or /proc/${pid}/stat
            //FIXME: all exit in here;?
            printf("invalid instruct or stack.\n");
            return high;
        }


        /* get stack len of this function */
        slen = (_instruct & 0xFFFF); /* a negative num, so use short can get sign */
        stack_len = (slen < 0) ? (-slen) : slen;
        printf("stack_len:[%d]\n", stack_len); 

        /* point to prev stack button */
        sp += stack_len;  
        if (sp > stack_start) {
            printf("stack overflow.\n");
            break;
        }
        /* the first instruct is : save ra to sp, and save ra to the top of this function stack
         * so: ra = *(sp + stack_len - 4)
         * */

        /* get (ra), return address, 
         * pc porint to the return address , return to the caller function */
        ra = sp - 4;
        //pc = (*(unsigned long *)(unsigned long)(ra)); 
        ptrace_readdata(pid, (uint8_t *)(ra), (uint8_t *)&pc, sizeof(ra));

        printf("sp1:[%x]\n", (unsigned int)((unsigned long)(ra))); 
        printf("pc :[%x]\n", (unsigned int)(pc)); 

        if (pc == 0 || stack_len == 0) {
            break;
        }

        trace_line_insert(trace_list, (unsigned long)pc);
        high++;
    }

    return high;  
}  

static int proc_maps_dump(struct list_head *p_map_head)
{
    struct proc_map *p_map = NULL;

    list_for_each_entry(p_map, p_map_head, node) {
        printf("%08lx-%08lx %c%c%c%c %08llx %02x:%02x %-10lu %s\n",
                (p_map->vm_start),
                (p_map->vm_end),
                p_map->flags & VM_READ ? 'r' : '-',
                p_map->flags & VM_WRITE ? 'w' : '-',
                p_map->flags & VM_EXEC ? 'x' : '-',
                p_map->flags & VM_MAYSHARE ? 's' : 'p',
                (p_map->pgoff),
                (p_map->dev_major),
                (p_map->dev_minor),
                (p_map->ino),
                p_map->vm_name
                );
    }
    
    return 0;
}

struct proc_map *proc_maps_find(struct list_head *p_map_head, unsigned long int addr)
{
    struct proc_map *p_map = NULL;

    list_for_each_entry(p_map, p_map_head, node) {
            if ((p_map->vm_start <= addr) &&
                    (p_map->vm_end >= addr)) {
                return p_map;
            }
    }
    
    return NULL;
}

static int proc_maps_destroy(struct list_head *p_map_head)
{
    struct proc_map *p_map = NULL, *n = NULL;

    list_for_each_entry_safe(p_map, n, p_map_head, node) {
        list_del(&(p_map->node));
        free(p_map);
        p_map = NULL;
    }
    
    return 0;
}

static int mips_regs_dump(struct pt_regs *regs)
{
    int i = 0;

    for (i = 0; i < 32; i ++) {
        printf("regs(%d)     0x%X\n", i, (unsigned int)(regs->regs[i]));
    }
#if 1
    printf("cp0_status   0x%08X\n", (unsigned int)(regs->cp0_status));
    printf("hi           0x%08X\n", (unsigned int)(regs->hi));
    printf("lo           0x%08X\n", (unsigned int)(regs->lo));
#ifdef CONFIG_CPU_HAS_SMARTMIPS
    printf("acx %08X\n", (unsigned int)(regs->acx));
#endif
    printf("cp0_badvaddr 0x%08X\n", (unsigned int)(regs->cp0_badvaddr));
    printf("cp0_cause    0x%08X\n", (unsigned int)(regs->cp0_cause));
    printf("cp0_epc      0x%08X\n", (unsigned int)(regs->cp0_epc));
#ifdef CONFIG_MIPS_MT_SMTC
    printf("cp0_tcstatus 0x%08X\n", (unsigned int)(regs->cp0_tcstatus));
#endif

#else
    printf("cp0_status   0x%16llX\n", (regs->cp0_status));
    printf("hi           0x%16llX\n", (regs->hi));
    printf("lo           0x%16llX\n", (regs->lo));
#ifdef CONFIG_CPU_HAS_SMARTMIPS
    printf("acx %16llX\n", (regs->acx));
#endif
    printf("cp0_badvaddr 0x%16llX\n", (regs->cp0_badvaddr));
    printf("cp0_cause    0x%16llX\n", (regs->cp0_cause));
    printf("cp0_epc      0x%16llX\n", (regs->cp0_epc));
#ifdef CONFIG_MIPS_MT_SMTC
    printf("cp0_tcstatus 0x%16llX\n", (regs->cp0_tcstatus));
#endif
#endif
    __DUMP_MEM__((unsigned char *)(regs->_safe_pad_), sizeof(regs->_safe_pad_));
    
    return 0;
}

static int get_mips_regs(int pid, struct pt_regs *regs)
{
    int ret = -1;
    //while (1) {
    //TODO:why first time will failed.
    sleep(2);
    ret = ptrace(PTRACE_GETREGS, pid, NULL, regs);
    if (ret < 0) {
        printf("%s %d PTRACE_GETREGS failed.\n", __func__, __LINE__);
        perror("PTRACE_GETREGS");
        //printf("%s %d sleep 1s and again.\n", __func__, __LINE__);
        //sleep(1);
        return -1;
    }
    //break;
    //}

    mips_regs_dump(regs);

    return 0;
}

static int trace_mod_attach(struct list_head *trace_list, struct list_head *proc_maps_list)
{
    struct proc_map *p_map = NULL;
    struct trace_line *t_line = NULL;

    list_for_each_entry(t_line, trace_list, node) {
        p_map = proc_maps_find(proc_maps_list, t_line->addr);
        if (p_map != NULL) {
            snprintf(t_line->lib_path, sizeof(t_line->lib_path), "%s", p_map->vm_name);
            if (strstr(t_line->lib_path, ".so")) {  //FIXME: bad check way
                t_line->addr = t_line->addr - p_map->vm_start;
            }
        }
    }

    return 0;
}

int get_proc_trace(int pid, struct list_head *trace_list, unsigned long stack_start)
{
    int h = 0;
    long ret = -1;
    unsigned long int sp = 0;
    unsigned long int ra = 0;
    struct pt_regs mips_regs;

    memset(&mips_regs, 0, sizeof(mips_regs));

    ret = ptrace(PTRACE_ATTACH,  pid, NULL, NULL);
    if (ret < 0) {
        printf("%s %d PTRACE_ATTACH failed.\n", __func__, __LINE__);
        perror("ptrace");
        return -1;
    }

    ret = get_mips_regs(pid, &mips_regs);
    if (ret < 0) {
        printf("%s %d get_mips_regs failed.\n", __func__, __LINE__);
        return -1;
    }

    sp = mips_regs.regs[29] & 0xFFFFFFFF; //ar934x,is bigend.
    ra = mips_regs.regs[31] & 0xFFFFFFFF;

    printf("sp 0x%X\n", (unsigned int)sp);
    printf("ra 0x%X\n", (unsigned int)ra);

    /* why lo as return address is always ok? */
    ra = (mips_regs.lo) & 0xFFFFFFFF; 

    h = __backtrace_proc(trace_list, sp, ra, stack_start, pid);

    ret = ptrace(PTRACE_DETACH,  pid, NULL, NULL);
    if (ret < 0) {
        printf("%s %d PTRACE_DETACH failed.\n", __func__, __LINE__);
        return -1;
    }

    /*
    ret = ptrace(PTRACE_CONT,  pid, NULL, NULL);
    if (ret < 0) {
        printf("%s %d PTRACE_CONT failed.\n", __func__, __LINE__);
        return -1;
    }
    */

    return h;
}

int main(int argc , char *argv[])
{   
	int c  = 0;
    pid_t pid = -1;

#ifdef CONFIG_32BIT
    printf("%s %d 32 bit.\n", __func__, __LINE__);
#endif


	while (1){
		c = getopt (argc, argv, "p:");
		if (c == -1)
			break;
        switch (c){
            case 'p':
                pid = atoi(optarg);
                break;
            default:
                printf("\n\tuse: xxxx \n");
                exit(0);
        }
	}
    
    int map_cnt = 0;
    struct list_head proc_maps_head;
    struct list_head trace_line_list_head;
    int h = 0;
    unsigned long stack_start = 0; /* stack top */
    struct proc_map *p_map = NULL;

    INIT_LIST_HEAD(&proc_maps_head);
    printf("%s %d pid = %d.\n", __func__, __LINE__, pid);
    map_cnt = get_proc_maps(pid, &proc_maps_head);
    if (map_cnt < 0) {
        return -1;
    }

    proc_maps_dump(&proc_maps_head);

    /* stack mm is always the last one, so do backwards is fast */
    list_for_each_entry_reverse(p_map, &(proc_maps_head), node) {
        if (strstr(p_map->vm_name, "[stack]")) {
            break;
        }
    }
    stack_start = p_map->vm_end;
    printf("%s %d vm_name:[%s] stack_start = %x.\n", __func__, __LINE__, 
            p_map->vm_name, (unsigned int)stack_start);

    printf("%s %d pid = %d.\n", __func__, __LINE__, pid);
    INIT_LIST_HEAD(&trace_line_list_head);
    h = get_proc_trace(pid, &trace_line_list_head, stack_start);

    trace_mod_attach(&trace_line_list_head, &proc_maps_head);


    char cmd_buf[128];
    struct trace_line *t_line = NULL;
    printf("%s %d ----------------------------.\n", __func__, __LINE__);
    list_for_each_entry(t_line, &trace_line_list_head, node) {
        /*
        printf("%s %d addr=%x mod_path:%s \n", __func__, __LINE__, 
                (unsigned int)t_line->addr, t_line->lib_path);
                */
        //./a2l 0x40206c -e /usr/sbin/dms -f
        //snprintf(cmd_buf, sizeof(cmd_buf), "./a2l 0x%x -e %s -f", t_line->addr, t_line->lib_path);
        //system(cmd_buf);
        addr2line(t_line->addr, t_line->lib_path);
    }
    printf("%s %d ----------------------------.\n", __func__, __LINE__);
    //printf("H:0x%x\n", h);
    //
    proc_maps_destroy(&proc_maps_head);
    trace_line_destroy_all(&trace_line_list_head);


    return 0;
}

