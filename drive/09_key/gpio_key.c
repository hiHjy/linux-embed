
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

#define GPIOKEY_CNT 1
#define GPIOKEY_NAME "gpiokey"
#define KEY0VALUE 0xf0
#define INVAKEY 0x0
struct gpiokey {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int key_gpio;
	atomic_t value;
};
struct gpiokey gpiokey;

static ssize_t gpiokey_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	
		
	return 0;
}


static int gpiokey_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpiokey;



	return 0;
}
ssize_t gpiokey_read (struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct gpiokey *dev = (struct gpiokey *)filp->private_data;
	if (gpio_get_value(dev->key_gpio) == 0) {
		while (!gpio_get_value(dev->key_gpio));
		atomic_set(&dev->value, KEY0VALUE);
	} else {
		atomic_set(&dev->value, INVAKEY);
	}
	
	
	int value = atomic_read(&dev->value);
	copy_to_user(buf, &value, sizeof(value));
	return sizeof(value);
}

static int gpiokey_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct gpiokey *)filp->private_data;

	return 0;
}

static const struct file_operations gpiokey_fops = {
		.owner 		= THIS_MODULE,
		.write 		= gpiokey_write,
		.read  		= gpiokey_read,
		.open  		= gpiokey_open,
		.release = gpiokey_release,
};


/*驱动入口出口函数*/
static int __init gpiokey_init(void) 
{

	atomic_set(&gpiokey.value, INVAKEY);//初始未按下
	/*注册字符设备驱动*/
	//gpiokey.major = 0;
	//自动申请设备号

	int ret = alloc_chrdev_region(&gpiokey.devid, 0, GPIOKEY_CNT, GPIOKEY_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&gpiokey.devid, 0, GPIOKEY_CNT, GPIOKEY_NAME)\n");
		goto failed_devid;
	}
		
	gpiokey.major = MAJOR(gpiokey.devid);
	gpiokey.minor = MINOR(gpiokey.devid);

	printk("gpiokey major:%d 	minor:%d\n", gpiokey.major, gpiokey.minor);

	/*初始化cdev*/
	gpiokey.cdev.owner = THIS_MODULE;
	cdev_init(&gpiokey.cdev, &gpiokey_fops);

	/*添加cdev*/
	ret = cdev_add(&gpiokey.cdev, gpiokey.devid, GPIOKEY_CNT);
	if (ret < 0) {
		printk("cdev_add(&gpiokey.cdev, gpiokey.devid, GPIOKEY_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	gpiokey.class = class_create(THIS_MODULE, GPIOKEY_NAME);
	if (IS_ERR(gpiokey.class)) {
		printk("gpiokey.class = class_create(THIS_MODULE, GPIOKEY_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	gpiokey.device = device_create(gpiokey.class, NULL, gpiokey.devid, NULL, GPIOKEY_NAME);
	if (IS_ERR(gpiokey.device)) {
		printk("gpiokey.device = device_create(gpiokey.class, NULL, gpiokey.devid, NULL, GPIOKEY_NAME)\n");
		goto failed_device;
	}

	/*找设备节点*/

	gpiokey.nd = of_find_node_by_path("/key");
	if (gpiokey.nd == NULL) {
		printk("of_find_node_by_path error");
		ret = -EINVAL;
		goto failed_nd;
	}
	printk("dt nd:%p\n", gpiokey.nd);
	
	/*获取gpio*/
	gpiokey.key_gpio = of_get_named_gpio(gpiokey.nd, "key-gpios", 0);
	if (gpiokey.key_gpio < 0) {
		printk("of_get_named_gpio error\n");
		ret = -EINVAL;
		goto failed_nd;
	}
	printk("gpio num:%d\n", gpiokey.key_gpio);
	
	/*请求gpio*/
	ret = gpio_request(gpiokey.key_gpio, "key0");
	if (ret < 0) {
		printk("gpio_request error\n");
		
		goto failed_request_gpio;
	}

	/*设置输入还是输出*/
	ret = gpio_direction_input(gpiokey.key_gpio);
	if (ret < 0) {
		printk("gpio_direction_input error\n");
		
		goto failed_set_direction;
	}
	
    return 0;
failed_set_direction:
	gpio_free(gpiokey.key_gpio);
failed_request_gpio: 
failed_nd:
	device_destroy(gpiokey.class, gpiokey.devid);
failed_device:
	class_destroy(gpiokey.class);
failed_class:
	cdev_del(&gpiokey.cdev);
failed_cdev_add:
	unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);
failed_devid:
	return ret;
}

static void __exit gpiokey_exit(void)
{
	
	gpio_free(gpiokey.key_gpio);
	device_destroy(gpiokey.class, gpiokey.devid);
	class_destroy(gpiokey.class);
	cdev_del(&gpiokey.cdev);
	unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);

}

module_init(gpiokey_init);
module_exit(gpiokey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






