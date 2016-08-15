#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEV_BASE_MINOR 	(0)
#define DEV_COUNT 		(0)
#define DEV_NAME		"ns"
#define DEV_CLASS_NAME  "ntsk"
#define DEV_NAME_BUF_LEN (16)

typedef struct tag_ns_dev_info
{
	dev_t dev_id;
	u32   dev_major;
	u32   dev_minor;
	char  dev_name[DEV_NAME_BUF_LEN];
	struct device *dev;		//device_create 产生
	struct cdev cdev;		//用于cdev_add
}NS_DEV_INFO_S;

typedef struct tag_ns_vm_disk
{
	void *pDisk;	/* 指向开始 */
	char *pNow;		/* 当前位置 字节记 */
	
}NS_VM_DISK;

NS_DEV_INFO_S stNsDevInfo;
static struct class *dev_class;
NS_VM_DISK stNsVMDisk;

/*
	设备类struct class是一个设备的高级视图，它抽象出低级的实现细节。
	例如，驱动可以见到一个SCSI磁盘或者一个ATA磁盘，在类的级别，他们都是磁盘，
	类允许用户空间基于它们作什么来使用设备，而不是它们如何被连接或者它们如何工作。
*/

static int ntsk_open(struct inode *inode, struct file *file)
{
	printk("OPEN SUCCESS!\n");

	return 0;
}


/* 设备操作挂接 */
static struct file_operations ntsk_fops = {
	.owner			= THIS_MODULE,
	.open 			= ntsk_open,
	//.release 		= button_close,
	//.read			= button_read,
	//.write			= led_write,
	//.unlocked_ioctl = button_ioctl,
};

static int ntsk_dev_setup(NS_DEV_INFO_S *pstNsDevInfo)
{
	int iRetVal = 0;

	/* 创建字符设备，注册到内核 */
	cdev_init(&(pstNsDevInfo->cdev), &ntsk_fops);
	iRetVal = cdev_add(&(pstNsDevInfo->cdev), pstNsDevInfo->dev_id, 1);
	if (iRetVal < 0) {
		printk("Cdev add failed.\n");
		goto err_cdev_add;
	}

	/* 创建节点 */
	pstNsDevInfo->dev = device_create(dev_class, 
	 							 	  NULL, 
	 							 	  pstNsDevInfo->dev_id, 
	 							 	  NULL, 
	 							 	  "ntsk");
	if (IS_ERR(pstNsDevInfo->dev))	 
	{
		printk("Create led device failed.\n");
		goto err_device_create;
	}

	return 0;

err_device_create:
	cdev_del(&(pstNsDevInfo->cdev));
err_cdev_add:

	return -1;
}

static void ntsk_dev_unsetup(NS_DEV_INFO_S *pstNsDevInfo)
{
	device_destroy(dev_class, pstNsDevInfo->dev_id);

	cdev_del(&(pstNsDevInfo->cdev));	
};

static int create_dev(NS_DEV_INFO_S *pstNsDevInfo)
{
	int iRet = 0;

	/* 1，申请设备号 */
	iRet = alloc_chrdev_region(&(pstNsDevInfo->dev_id), 
								DEV_BASE_MINOR, DEV_COUNT, 
								pstNsDevInfo->dev_name);
	if (iRet < 0)
	{
		printk("Can not register char device!\n");
		goto err_reg_chrdev;
	}

	pstNsDevInfo->dev_major = MAJOR(pstNsDevInfo->dev_id);
	pstNsDevInfo->dev_minor = MINOR(pstNsDevInfo->dev_id);

	printk("Alloc device Success!\n"
		   "    major = %d\n"
		   "    minor = %d\n", 
		   pstNsDevInfo->dev_major, pstNsDevInfo->dev_minor);

	/* 2，创建设备类 */
	dev_class = class_create(THIS_MODULE, DEV_CLASS_NAME);
	if (IS_ERR(dev_class))
	{
		printk("Cannot create class!\n");
		goto err_class_create;
	}
	
	/* 3，安装设备和驱动 */
	ntsk_dev_setup(pstNsDevInfo);

	return 0;

err_class_create:
	unregister_chrdev_region(pstNsDevInfo->dev_id, DEV_COUNT);
err_reg_chrdev:	

	return -1;
}

#define SIZE_1M      (1024 * 1024)
#define VM_DISK_SIZE (1 * SIZE_1M)
/* 使用内存作为一个模拟的Disk */
static void * vm_disk_create(void)
{
	void *pDisk = NULL;
	uint uiPDiskPhyAddr;
	uint uiVMF;
	uint uiPHYF;

	/* kmalloc 申请的大小有限制的，?? */
	pDisk = kmalloc(VM_DISK_SIZE, GFP_KERNEL);
	if (IS_ERR(pDisk))
	{
		printk("Create VM Disk Failed!\n");
		return NULL;
	}

	uiPDiskPhyAddr = (uint)phys_to_virt(pDisk);
	printk("VM Disk Start ADDR:\n"
		   "                   VM :%#x\n"
		   "                   PHY:%#x\n", 
		   (uint)pDisk,
		   uiPDiskPhyAddr);

	uiVMF  = (uint)phys_to_virt;
	uiPHYF = (uint)phys_to_virt((void *)phys_to_virt);
	printk("VMF ADDR:\n"
		   "     VM :%#x\n"
		   "     PHY:%#x\n", 
		   (uint)uiVMF,
		   uiPHYF);

	return pDisk;
}

static void vm_disk_destroy(void *pDisk)
{
	kfree(pDisk);

	return;
}

static int destroy_dev(NS_DEV_INFO_S *pstNsDevInfo)
{
	ntsk_dev_unsetup(&stNsDevInfo);
	
	class_destroy(dev_class);

	unregister_chrdev_region(pstNsDevInfo->dev_id, DEV_COUNT);

	return 0;
}

/* 模块初始化 */
static int __init helloworld_init(void)
{
	int iRet = 0;

	memset(&stNsDevInfo, 0, sizeof(stNsDevInfo));
	sprintf(stNsDevInfo.dev_name, "ns");
	iRet = create_dev(&stNsDevInfo);
	if (iRet < 0)
	{
		printk("Create Dev Failed!");
		goto err_create_dev_failed;
	}

	memset(&stNsVMDisk, 0, sizeof(stNsVMDisk));
	stNsVMDisk.pDisk = vm_disk_create();
	if (NULL == stNsVMDisk.pDisk)
	{
		printk("Create VM Disk Failed!");
		goto err_vm_disk_create;
	}

	printk("Device VM Disk Create SUCCESS !\n");
	
	return 0;

err_vm_disk_create:
	destroy_dev(&stNsDevInfo);
err_create_dev_failed:

	return -1;
}

/* 模块去初始化 */
static void __exit helloworld_exit(void)
{
	vm_disk_destroy(stNsVMDisk.pDisk);

	destroy_dev(&stNsDevInfo);

	printk("Device VM Disk is Destroy !\n");

	return;
}

module_init(helloworld_init);
module_exit(helloworld_exit);

MODULE_LICENSE("GPL"); /* 没有这个会出现 "unknown symbol class_create" */
MODULE_AUTHOR("ljy051031448@163.com");
MODULE_DESCRIPTION("ntsk");
MODULE_VERSION("0.0.1");

