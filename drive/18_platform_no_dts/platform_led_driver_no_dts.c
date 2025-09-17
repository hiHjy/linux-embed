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
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#define DTSLED_CNT 1
#define DTSLED_NAME "platform_led_no_dts"

static void *IMX6U_CCM_CCGR1;
static void *SW_MUX_GPIO1_IO03;
static void *SW_PAD_GPIO1_IO03;
static void *GPIO1_DR;
static void *GPIO1_GDIR;  
static struct led {
	dev_t devid; //设备号
	int major;	 //主设备号
	int minor;   //次设备号
	struct cdev cdev;
	struct class *class;
	struct device *device; 
	struct device_node *nd; /*设备节点*/

};
struct led dtsled;

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
	
};

int led_remove (struct platform_device * dev)
{

	printk("led_remove--\n");
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

static int led_probe(struct platform_device * dev) 
{
	int i; 
	printk("led_probe--\n");
	struct resource *res[dev->num_resources];
	for (i = 0; i <  dev->num_resources; ++i) {
		res[i] =  platform_get_resource(dev, IORESOURCE_MEM, i); 
		if (res[i] == NULL) {
			 printk("res[i] =  platform_get_resource\n")  ;
			return -EINVAL; 
		}
		  
	}

//	int reg_legenth = res[0].end - res[0].start + 1; 

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
	IMX6U_CCM_CCGR1 = ioremap(res[0]->start, resource_size(res[0]));
	SW_MUX_GPIO1_IO03 = ioremap(res[1]->start, resource_size(res[0]));
  	SW_PAD_GPIO1_IO03 = ioremap(res[2]->start, resource_size(res[0]));
	GPIO1_DR = ioremap(res[3]->start, resource_size(res[0]));
	GPIO1_GDIR = ioremap(res[4]->start, resource_size(res[0]));
	 
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
	
	printk("probe end\n");
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
	  

	return 0;  
}



static struct platform_driver led_driver = {
	.driver = {
		.name = "imx6ull-led",  
			
	},
	.probe = led_probe,
	.remove = led_remove,

}; 


/*驱动加载*/
static int __init leddriver_init(void)
{

	platform_driver_register(&led_driver);
	return 0;

}

/*驱动卸载*/

static void __exit leddriver_exit(void)
{
	platform_driver_unregister(&led_driver); /*会调用remove函数    */

}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");
 
