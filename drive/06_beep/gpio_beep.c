
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

#define GPIOBEEP_CNT 1
#define GPIOBEEP_NAME "gpiobeep"

struct gpiobeep {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int beep_gpio;
};
struct gpiobeep gpiobeep;

static ssize_t gpiobeep_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	struct gpiobeep *dev = (struct gpiobeep *)filp->private_data; 
	unsigned char databuf;
	copy_from_user(&databuf, buf, 1);

	if (databuf == '0') {
		gpio_set_value(dev->beep_gpio, 1);
		
	} else {
		
		gpio_set_value(dev->beep_gpio, 0);
	}
		
	return 0;
}


static int gpiobeep_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpiobeep;



	return 0;
}

static int gpiobeep_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct gpiobeep *)filp->private_data;

	return 0;
}

static const struct file_operations gpiobeep_fops = {
		.owner = THIS_MODULE,
		.write = gpiobeep_write,
		.open = gpiobeep_open,
		.release = gpiobeep_release,
};


/*驱动入口出口函数*/
static int __init beep_init(void) 
{
	/*注册字符设备驱动*/
	//gpiobeep.major = 0;

	//自动申请设备号

	int ret = alloc_chrdev_region(&gpiobeep.devid, 0, GPIOBEEP_CNT, GPIOBEEP_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&gpiobeep.devid, 0, GPIOBEEP_CNT, GPIOBEEP_NAME)\n");
		goto failed_devid;
	}
		
	gpiobeep.major = MAJOR(gpiobeep.devid);
	gpiobeep.minor = MINOR(gpiobeep.devid);

	printk("gpiobeep major:%d 	minor:%d\n", gpiobeep.major, gpiobeep.minor);

	/*初始化cdev*/
	gpiobeep.cdev.owner = THIS_MODULE;
	cdev_init(&gpiobeep.cdev, &gpiobeep_fops);

	/*添加cdev*/
	ret = cdev_add(&gpiobeep.cdev, gpiobeep.devid, GPIOBEEP_CNT);
	if (ret < 0) {
		printk("cdev_add(&gpiobeep.cdev, gpiobeep.devid, GPIOBEEP_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	gpiobeep.class = class_create(THIS_MODULE, GPIOBEEP_NAME);
	if (IS_ERR(gpiobeep.class)) {
		printk("gpiobeep.class = class_create(THIS_MODULE, GPIOBEEP_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	gpiobeep.device = device_create(gpiobeep.class, NULL, gpiobeep.devid, NULL, GPIOBEEP_NAME);
	if (IS_ERR(gpiobeep.device)) {
		printk("gpiobeep.device = device_create(gpiobeep.class, NULL, gpiobeep.devid, NULL, GPIOBEEP_NAME)\n");
		goto failed_device;
	}


	/*初始化beep*/
	gpiobeep.nd = of_find_node_by_path("/beep");
	if (gpiobeep.nd == NULL) {
		printk(" of_find_node_by_path error\n");
		ret = -EINVAL;
		goto failed_nd;
	}
	printk(" beep node: %p\n", gpiobeep.nd);
	
	gpiobeep.beep_gpio = of_get_named_gpio(gpiobeep.nd, "beep-gpios", 0);
	if (gpiobeep.beep_gpio < 0) {
		printk("  of_get_named_gpio error\n");
		ret = -EINVAL;
		goto failed_get_gpio;
		
	}
	printk("gpio num:%d\n", gpiobeep.beep_gpio);

	ret = gpio_request(gpiobeep.beep_gpio, "gpio");
	if (ret < 0) {
		printk("  of_get_named_gpio error\n");
		
		goto failed_request_gpio;
	}

	ret = gpio_direction_output(gpiobeep.beep_gpio, 1);//默认不响
	if (ret < 0) {
		printk("  gpio_direction_output error\n");
		
		goto failed_get_gpio;
	}

    return 0;
failed_request_gpio:
	gpio_free(gpiobeep.beep_gpio);
	
failed_get_gpio:
failed_nd:
	device_destroy(gpiobeep.class, gpiobeep.devid);
failed_device:
	class_destroy(gpiobeep.class);
failed_class:
	cdev_del(&gpiobeep.cdev);
failed_cdev_add:
	unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);
failed_devid:
	return ret;
}

static void __exit  beep_exit(void)
{
	gpio_free(gpiobeep.beep_gpio);
	
	device_destroy(gpiobeep.class, gpiobeep.devid);
	class_destroy(gpiobeep.class);
	cdev_del(&gpiobeep.cdev);
	unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);

}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






