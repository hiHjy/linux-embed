
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
#include <linux/timer.h>
#define TIMER_LED_CNT 1
#define TIMER_LED_NAME "timer_led"

struct timer_led {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int led_gpio;
	struct timer_list timer; //定时器
	
	
};
struct timer_led timer_led;

static ssize_t timer_led_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	unsigned char databuf;
	copy_from_user(&databuf, buf, 1);

	
		
	return 0;
}


static int timer_led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &timer_led;



	return 0;
}

static int timer_led_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct timer_led *)filp->private_data;

	return 0;
}

static const struct file_operations timer_led_fops = {
		.owner = THIS_MODULE,
		.write = timer_led_write,
		.open = timer_led_open,
		.release = timer_led_release,
};
void timer_function (unsigned long data) {
	struct timer_led *dev = (struct timer_led *)data;
	static int status = 1;
	status = !status;
	gpio_set_value(dev->led_gpio, status);

	mod_timer(&timer_led.timer, jiffies + msecs_to_jiffies(500));
	
}
int timer_led_init(void)
{
	int ret = 0;
	timer_led.nd = of_find_node_by_path("/gpioled");
	if (timer_led.nd == NULL) {
		printk("of_find_node_by_path error\n");
		ret = -EINVAL;
		goto failed_find_nd;
	}
	printk("get dt nd:%p\n", timer_led.nd);
	
	timer_led.led_gpio = of_get_named_gpio(timer_led.nd, "led-gpios", 0);
	
	if (timer_led.led_gpio < 0) {
		printk("of_get_named_gpio error\n");
		ret = -EINVAL;
		goto failed_find_nd;
	}
	printk("get gpio num:%d\n", timer_led.led_gpio);

	ret = gpio_request(timer_led.led_gpio, "timer_led");
	if (ret < 0) {
		printk("gpio_request error\n");
		goto failed_find_nd;
	}

	ret = gpio_direction_output(timer_led.led_gpio, 1);//默认关灯
	if (ret < 0) {
		printk("gpio_direction_output error\n");
		goto failed_set_direction;
	}

	return 0;
failed_set_direction:
	gpio_free(timer_led.led_gpio);
failed_find_nd:
	device_destroy(timer_led.class, timer_led.devid);
	
	return ret;


}

/*驱动入口出口函数*/
static int __init led_init(void) 
{
	/*注册字符设备驱动*/
	//timer_led.major = 0;
	//自动申请设备号

	int ret = alloc_chrdev_region(&timer_led.devid, 0, TIMER_LED_CNT, TIMER_LED_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&timer_led.devid, 0, TIMER_LED_CNT, TIMER_LED_NAME)\n");
		goto failed_devid;
	}
		
	timer_led.major = MAJOR(timer_led.devid);
	timer_led.minor = MINOR(timer_led.devid);

	printk("timer_led major:%d 	minor:%d\n", timer_led.major, timer_led.minor);

	/*初始化cdev*/
	timer_led.cdev.owner = THIS_MODULE;
	cdev_init(&timer_led.cdev, &timer_led_fops);

	/*添加cdev*/
	ret = cdev_add(&timer_led.cdev, timer_led.devid, TIMER_LED_CNT);
	if (ret < 0) {
		printk("cdev_add(&timer_led.cdev, timer_led.devid, TIMER_LED_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	timer_led.class = class_create(THIS_MODULE, TIMER_LED_NAME);
	if (IS_ERR(timer_led.class)) {
		printk("timer_led.class = class_create(THIS_MODULE, TIMER_LED_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	timer_led.device = device_create(timer_led.class, NULL, timer_led.devid, NULL, TIMER_LED_NAME);
	if (IS_ERR(timer_led.device)) {
		printk("timer_led.device = device_create(timer_led.class, NULL, timer_led.devid, NULL, TIMER_LED_NAME)\n");
		goto failed_device;
	}

	/*初始化led io*/
	ret = timer_led_init();
	if (ret < 0) {
		goto failed_device;
	}
	/*初始化定时器*/

	init_timer(&timer_led.timer);

	timer_led.timer.expires = jiffies + msecs_to_jiffies(2000);
	timer_led.timer.function = timer_function;
	timer_led.timer.data = (unsigned long)&timer_led;
	

	/*定时器添加到系统*/
	
	add_timer(&timer_led.timer);
	
	
	




	
	
	
    return 0;

failed_device:
	class_destroy(timer_led.class);
failed_class:
	cdev_del(&timer_led.cdev);
failed_cdev_add:
	unregister_chrdev_region(timer_led.devid, TIMER_LED_CNT);
failed_devid:
	return ret;
}

static void __exit led_exit(void)
{
	
	gpio_set_value(timer_led.led_gpio, 1);
	gpio_free(timer_led.led_gpio);
	del_timer(&timer_led.timer);
	device_destroy(timer_led.class, timer_led.devid);
	class_destroy(timer_led.class);
	cdev_del(&timer_led.cdev);
	unregister_chrdev_region(timer_led.devid, TIMER_LED_CNT);

}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






