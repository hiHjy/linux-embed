
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
#define KEY_NUM 1
struct irq_key_desc {
	int gpio;			/*io编号 */
	int irq_num;		/*中断号 */
	unsigned char value;/*键值 */
	char name[10];		/*按键名字*/
						/*中断处理函数*/
};
struct gpiokey {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int key_gpio;
	struct irq_key_desc irq_key[KEY_NUM];
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
	// struct gpiokey *dev = (struct gpiokey *)filp->private_data;
	// if (gpio_get_value(dev->key_gpio) == 0) {
	// 	while (!gpio_get_value(dev->key_gpio));
	// 	atomic_set(&dev->value, KEY0VALUE);
	// } else {
	// 	atomic_set(&dev->value, INVAKEY);
	// }
	
	
	// // int value = atomic_read(&dev->value);
	// copy_to_user(buf, &value, sizeof(value));
	// return sizeof(value);
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

int irq_key_init(struct gpiokey *dev)
{
	int ret =0;
	/*按键初始化*/
	dev->nd = of_find_node_by_path("/key");
	if (dev->nd == NULL) {
		printk("of_find_node_by_path error\n");
		ret = -EINVAL;
		goto failed_find_nd;
	}
	printk("dt node:%p\n", dev->nd);

	//of_get_named_gpio(struct device_node * np, const char * propname, int index)

	int i;
	for (i = 0; i < KEY_NUM; ++i) {
		dev->irq_key[i].gpio = of_get_named_gpio(dev->nd, "key-gpios", i);
		if (dev->irq_key[i].gpio < 0) {
			ret = -EINVAL;
			goto failed_find_nd;
		}
		printk("gpio num:%d\n", dev->irq_key[i].gpio);
		
		
		memset(dev->irq_key[i].name, '\0', sizeof(dev->irq_key[i].name));
		char str[10] = "irq_key";
		snprintf(str + strlen(str), sizeof(str) - strlen(str), "%d", i);
		memcpy(dev->irq_key[i].name, str, sizeof(dev->irq_key[i].name) - 1);

		ret = gpio_request(dev->irq_key[i].gpio,dev->irq_key[i].name); 
		if (ret < 0) {
			ret = -EBUSY;
			goto failed_find_nd;
		}
		printk("key name:%s\n",dev->irq_key[i].name);
		ret = gpio_direction_input(dev->irq_key[i].gpio);
		if (ret < 0) {
			goto failed_direction;
		}
	}

	/*中断*/

	return 0;
failed_direction:
	for (i = 0; i < KEY_NUM; ++i) 
		gpio_free(gpiokey.irq_key[i].gpio);
failed_find_nd:
	return ret;
}

/*驱动入口出口函数*/
static int __init gpiokey_init(void) 
{

	
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
	// ret = irq_key_init(&gpiokey);
	// if (ret < 0) {
	// 	goto failed_device;

	// }
	return 0;
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
	int  i;
	

	for (i = 0; i < KEY_NUM; ++i) {
		if (gpiokey.irq_key[i].gpio) {
			pr_info("gpiokey_exit: before gpio_free(gpiokey.irq_key[i].gpio);\n");
			gpio_free(gpiokey.irq_key[i].gpio);
			pr_info("gpiokey_exit: after gpio_free(gpiokey.irq_key[i].gpio);\n");
		}
	}
		
	pr_info("gpiokey_exit: before device_destroy(gpiokey.class, gpiokey.devid);\n");
	device_destroy(gpiokey.class, gpiokey.devid);
	pr_info("gpiokey_exit: after device_destroy(gpiokey.class, gpiokey.devid);\n");

	pr_info("gpiokey_exit: before class_destroy(gpiokey.class);\n");
	class_destroy(gpiokey.class);
	pr_info("gpiokey_exit: afterclass_destroy(gpiokey.class);\n");
	cdev_del(&gpiokey.cdev);
	unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);

}

module_init(gpiokey_init);
module_exit(gpiokey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






