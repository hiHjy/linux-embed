#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm-generic/uaccess.h>
#define DTSLED_CNT 1
#define DTSLED_NAME "dts_leds"
//static int majoy = -1; //设备号
struct led dtsled;
/*物理地址映射后的虚拟指针*/
static void *IMX6U_CCM_CCGR1;
static void *SW_MUX_GPIO1_IO03;
static void *SW_PAD_GPIO1_IO03;
static void *GPIO1_DR;
static void *GPIO1_GDIR;

/*led 设备结构体*/
static struct led {
	dev_t devid; //设备号
	int major;	 //主设备号
	int minor;   //次设备号
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd; /*设备节点*/

};


static int dtsled_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dtsled;



	return 0;
}
//ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);

static ssize_t dtsled_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	struct dtsled *device = (struct dtsled *)filp->private_data;
	int retval;
	int val;
	unsigned char databuf;
	retval = copy_from_user(&databuf, buf, count);
	printk("----------------\n");
	printk("%c\n",databuf);
	if (databuf == '0') {
		val = readl(GPIO1_DR);

		val |= (1 << 3); //关闭led灯
		writel(val, GPIO1_DR);
	} else {
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);
		writel(val, GPIO1_DR);
	}
	return 0;
}

static int dtsled_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct dtsled *)filp->private_data;

	return 0;
}



static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = dtsled_write,
	.open = dtsled_open,
	//.release = dts_led_exit,
};

static int __init dts_led_init(void)
{	



	


	/*   1， 申请设备号        */
	
	int ret = 0;
	dtsled.major = 0;
	if (dtsled.major) {
		//表示主设备号不为0，有人设置了主设备号
		dtsled.devid = MKDEV(dtsled.major, 0);	//次设备号为0
		ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
		if (ret < 0) {

		}
	} else {

		//主设备号为0，表示没人设置主设备号，要申请
		ret = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
		dtsled.major = MAJOR(dtsled.devid);
		dtsled.minor = MINOR(dtsled.devid);
	}

	if (ret < 0) {

		printk("设备号申请失败\n");
		goto faile_devid;

	}

	/*  2，添加字符设备       */
	dtsled.cdev.owner = THIS_MODULE;
	cdev_init(&dtsled.cdev, &fops);
	ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
	if (ret < 0) {
		goto faile_cdev;
	}

	/*3, 自动创建设备节点*/
	dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
	if (IS_ERR(dtsled.class)) {
		ret = PTR_ERR(dtsled.class);

		goto faile_class;
	}

	dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
	if (IS_ERR(dtsled.device)) {
		ret = PTR_ERR(dtsled.class);
		goto faile_device;
	}

	/*获取设备树的属性内容*/
	
	dtsled.nd = of_find_node_by_path("/leds");
	if (dtsled.nd == NULL) {
		printk("of_find_node_by_path error");
		goto faile_findnd;
	}
	const char *out_str;
	of_property_read_string(dtsled.nd, "compatible", &out_str);
	printk("compatible:%s\n", out_str);
	/**/
	of_property_read_string(dtsled.nd, "status", &out_str);
	printk("status:%s\n", out_str);


		//获取reg
	u32 arr[10];
	ret = of_property_read_u32_array(dtsled.nd, "reg", arr, 10);

	if (ret < 0) {
		printk("of_property_read_u32_array error\n");
		goto rs;

	} else {
		
		int i;
		for (i = 0; i < 10; ++i) {
			printk("%X  ", arr[i]);
		}
		printk("\n");

	}

	/*led*/

	//static void *SW_MUX_GPIO1_IO03;
	//static void *SW_PAD_GPIO1_IO03;
	//static void *GPIO1_DR;
	//static void *GPIO1_GDIR;
	IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
	
	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
	GPIO1_DR = of_iomap(dtsled.nd, 3);
	GPIO1_GDIR = of_iomap(dtsled.nd, 4);

	int val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);
	
	val |= 3 << 26;
	writel(val, IMX6U_CCM_CCGR1);

	writel(0x5, SW_MUX_GPIO1_IO03);
	val = readl(GPIO1_GDIR);
	val |= 1 << 3;
	writel(val, GPIO1_GDIR);

	val = readl(GPIO1_DR);

	val |= (1 << 3); //关闭led灯
	writel(val, GPIO1_DR);
	
	
	return 0;
rs:
faile_findnd:
	device_destroy(dtsled.class, dtsled.devid);
faile_device:
	class_destroy(dtsled.class); 	//删除/sys/devices/dts_leds

faile_class:
	cdev_del(&dtsled.cdev);//		//删除设备
	
faile_devid:
faile_cdev:

	unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
	return ret;
}

static void __exit dts_led_exit(void)
{
	int val = readl(GPIO1_DR);

	val |= (1 << 3); //关闭led灯
	writel(val, GPIO1_DR);
		
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);
	cdev_del(&dtsled.cdev);//删除设备
	
	
	device_destroy(dtsled.class, dtsled.devid);
	class_destroy(dtsled.class);
	unregister_chrdev_region(dtsled.devid, DTSLED_CNT);//删除设备号
}





/*注册驱动和卸载驱动入口函数*/
module_init(dts_led_init);
module_exit(dts_led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hjy");











