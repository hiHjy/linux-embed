
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
#include <linux/atomic.h>
#define GPIOLED_CNT 1
#define GPIOLED_NAME "gpioled"

struct gpioled {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int led_gpio;
	spinlock_t lock; //自旋锁
	int status; /*0 表示未被使用，1表示已被使用*/
};
struct gpioled gpioled;

static ssize_t gpioled_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	unsigned char databuf;
	copy_from_user(&databuf, buf, 1);

	if (databuf == '0') {
		gpio_set_value(gpioled.led_gpio, 1);
		
	} else {
		
		gpio_set_value(gpioled.led_gpio, 0);
	}
		
	return 0;
}


static int gpioled_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled;
	
	spin_lock(&gpioled.lock);
	//gpioled.status = 1;

	if (gpioled.status != 0) {
		spin_unlock(&gpioled.lock);
		return -EBUSY;
	} else {
		gpioled.status++;
	}
	spin_unlock(&gpioled.lock);
	return 0;
}

static int gpioled_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct gpioled *)filp->private_data;

	spin_lock(&gpioled.lock);
	if (gpioled.status)


 	return 0;
}

static const struct file_operations gpioled_fops = {
		.owner = THIS_MODULE,
		.write = gpioled_write,
		.open = gpioled_open,
		.release = gpioled_release,
};


/*驱动入口出口函数*/
static int __init led_init(void) 
{
	/*初始化自旋锁*/
	spin_lock_init(&gpioled.lock);
	gpioled.status = 0;
	/*注册字符设备驱动*/
	//gpioled.major = 0;
	//自动申请设备号

	int ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME)\n");
		goto failed_devid;
	}
		
	gpioled.major = MAJOR(gpioled.devid);
	gpioled.minor = MINOR(gpioled.devid);

	printk("gpioled major:%d 	minor:%d\n", gpioled.major, gpioled.minor);

	/*初始化cdev*/
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);

	/*添加cdev*/
	ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
	if (ret < 0) {
		printk("cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class)) {
		printk("gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device)) {
		printk("gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME)\n");
		goto failed_device;
	}

	/*获取设备节点*/

	gpioled.nd = of_find_node_by_path("/gpioled");
	
	if (gpioled.nd == NULL) {
		printk("of_find_node_by_path error\n");
		goto failed_nd;
	}

	printk("gpioled.nd = %p\n", gpioled.nd);

	/*获取led对应的gpio编号*/

	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
	if (gpioled.led_gpio < 0) {
		printk("获取led gpio失败\n");
		ret = -EINVAL;
		goto failed; 
	}

	printk("led gpio num:%d \n", gpioled.led_gpio);

	/*申请一下gpio，如果次gpio被占用，那么就会申请失败*/

	
	ret = gpio_request(gpioled.led_gpio, "gpio-leds");
	if (ret != 0) {
		printk("gpio_request error\n");
		goto failed;
	}

	/*使用gpio*/

	ret = gpio_direction_output(gpioled.led_gpio, 1); //默认高电平led熄灭
	if (ret != 0) {
		printk("gpio_direction_output error\n");
		ret = -EINVAL;
		goto failed_gpio_direction;
	}
	
	gpio_set_value(gpioled.led_gpio, 0);//led亮
	printk("value:%d\n",gpio_get_value(gpioled.led_gpio));
    return 0;
failed_gpio_direction:
	gpio_free(gpioled.led_gpio);
failed:
failed_nd:
	device_destroy(gpioled.class, gpioled.devid);
failed_device:
	class_destroy(gpioled.class);
failed_class:
	cdev_del(&gpioled.cdev);
failed_cdev_add:
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
failed_devid:
	return ret;
}

static void __exit led_exit(void)
{
	/*关灯*/
	printk("出口函数\n");
	gpio_set_value(gpioled.led_gpio, 1);
	printk("value:%d\n",gpio_get_value(gpioled.led_gpio));
	gpio_free(gpioled.led_gpio);
	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






