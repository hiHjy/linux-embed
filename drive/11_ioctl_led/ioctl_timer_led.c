
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

#define IOCTL_TIMER_LED_CNT 1
#define IOCTL_TIMER_LED_NAME "ioctl_timer_led"
#define PERIOD_500 500
#define PERIOD_1000 1000
#define PERIOD_2000 2000

#define CLOSE_CMD			_IO(0XEF, 1)
#define OPEN_CMD			_IO(0XEF, 2)
#define SETPERIOD_CMD		_IOW(0XEF, 3, int)
enum timer_period {
		TIMER_PERIOD_500,
		TIMER_PERIOD_1000,
		TIMER_PERIOD_2000,
	
};
enum timer_period timer_period;
struct ioctl_timer_led {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int led_gpio;
	struct timer_list timer; //定时器
	int period;
	
	
};

struct ioctl_timer_led ioctl_timer_led;




static int ioctl_timer_led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ioctl_timer_led;



	return 0;
}

static int ioctl_timer_led_release(struct inode *inode, struct file *filp) {

	struct dtsled *device = (struct ioctl_ioctl_timer_led *)filp->private_data;

	return 0;
}

/*ioctl 函数*/
long led_timer_ioctl (struct file *filp, unsigned int cmd, unsigned long buf)
{
	int period_buf;
	struct ioctl_timer_led *dev = (struct ioctl_timer_led *)filp->private_data;

	switch (cmd) {
		case CLOSE_CMD:
			del_timer_sync(&dev->timer);	
		break;

		case OPEN_CMD:
			mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(500));
		break;

		case SETPERIOD_CMD:
			
			if (copy_from_user(&period_buf, (int*)buf, sizeof(int))) {
				return -EFAULT;
			}
	
			switch (period_buf) {
				case PERIOD_500:
					ioctl_timer_led.period = TIMER_PERIOD_500;
					mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(PERIOD_500));
				break;
					
				case PERIOD_1000:
					ioctl_timer_led.period = TIMER_PERIOD_1000;
					mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(PERIOD_1000));
				break;

				case PERIOD_2000:
					ioctl_timer_led.period = TIMER_PERIOD_2000;
					mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(PERIOD_2000));
				break;

				default:

					printk("参数错误\n");

			}
			
		break;
		


	}
	
	return 0;

}

static const struct file_operations ioctl_timer_led_fops = {
		.owner = THIS_MODULE,
		
		.open = ioctl_timer_led_open,
		.release = ioctl_timer_led_release,
		.unlocked_ioctl = led_timer_ioctl,
};
void timer_function (unsigned long data) {
	struct ioctl_timer_led *dev = (struct ioctl_timer_led *)data;
	static int status = 1;
	status = !status;
	gpio_set_value(dev->led_gpio, status);
	switch (dev->period) {
		case TIMER_PERIOD_500:
		mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(500));
		break;
		case TIMER_PERIOD_1000:
		mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(1000));
		break;

		case TIMER_PERIOD_2000:
		mod_timer(&ioctl_timer_led.timer, jiffies + msecs_to_jiffies(2000));
		break;
	}
		
}
int ioctl_timer_led_init(void)
{
	int ret = 0;
	ioctl_timer_led.nd = of_find_node_by_path("/gpioled");
	if (ioctl_timer_led.nd == NULL) {
		printk("of_find_node_by_path error\n");
		ret = -EINVAL;
		goto failed_find_nd;
	}
	printk("get dt nd:%p\n", ioctl_timer_led.nd);
	
	ioctl_timer_led.led_gpio = of_get_named_gpio(ioctl_timer_led.nd, "led-gpios", 0);
	
	if (ioctl_timer_led.led_gpio < 0) {
		printk("of_get_named_gpio error\n");
		ret = -EINVAL;
		goto failed_find_nd;
	}
	printk("get gpio num:%d\n", ioctl_timer_led.led_gpio);

	ret = gpio_request(ioctl_timer_led.led_gpio, "ioctl_timer_led");
	if (ret < 0) {
		printk("gpio_request error\n");
		goto failed_find_nd;
	}

	ret = gpio_direction_output(ioctl_timer_led.led_gpio, 1);//默认关灯
	if (ret < 0) {
		printk("gpio_direction_output error\n");
		goto failed_set_direction;
	}

	return 0;
failed_set_direction:
	gpio_free(ioctl_timer_led.led_gpio);
failed_find_nd:
	device_destroy(ioctl_timer_led.class, ioctl_timer_led.devid);
	
	return ret;


}

/*驱动入口出口函数*/
static int __init led_init(void) 
{
	/*注册字符设备驱动*/
	//ioctl_timer_led.major = 0;
	//自动申请设备号

	int ret = alloc_chrdev_region(&ioctl_timer_led.devid, 0, IOCTL_TIMER_LED_CNT, IOCTL_TIMER_LED_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&ioctl_timer_led.devid, 0, IOCTL_TIMER_LED_CNT, IOCTL_TIMER_LED_NAME)\n");
		goto failed_devid;
	}
		
	ioctl_timer_led.major = MAJOR(ioctl_timer_led.devid);
	ioctl_timer_led.minor = MINOR(ioctl_timer_led.devid);

	printk("ioctl_timer_led major:%d 	minor:%d\n", ioctl_timer_led.major, ioctl_timer_led.minor);

	/*初始化cdev*/
	ioctl_timer_led.cdev.owner = THIS_MODULE;
	cdev_init(&ioctl_timer_led.cdev, &ioctl_timer_led_fops);

	/*添加cdev*/
	ret = cdev_add(&ioctl_timer_led.cdev, ioctl_timer_led.devid, IOCTL_TIMER_LED_CNT);
	if (ret < 0) {
		printk("cdev_add(&ioctl_timer_led.cdev, ioctl_timer_led.devid, IOCTL_TIMER_LED_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	ioctl_timer_led.class = class_create(THIS_MODULE, IOCTL_TIMER_LED_NAME);
	if (IS_ERR(ioctl_timer_led.class)) {
		printk("ioctl_timer_led.class = class_create(THIS_MODULE, IOCTL_TIMER_LED_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	ioctl_timer_led.device = device_create(ioctl_timer_led.class, NULL, ioctl_timer_led.devid, NULL, IOCTL_TIMER_LED_NAME);
	if (IS_ERR(ioctl_timer_led.device)) {
		printk("ioctl_timer_led.device = device_create(ioctl_timer_led.class, NULL, ioctl_timer_led.devid, NULL, IOCTL_TIMER_LED_NAME)\n");
		goto failed_device;
	}

	/*初始化led io*/
	ret = ioctl_timer_led_init();
	if (ret < 0) {
		goto failed_device;
	}
	/*初始化定时器*/

// 	enum timer_period {
// 		TIMER_PERIOD_500,
// 		TIMER_PERIOD_1000,
// 		TIMER_PERIOD_2000,
	
// };
// enum timer_period timer_period period;
// struct ioctl_timer_led {
// 	dev_t devid;
// 	int major;
// 	int minor;
// 	struct cdev cdev;
// 	struct class *class;
// 	struct device *device;
// 	struct device_node *nd;
// 	int led_gpio;
// 	struct timer_list timer; //定时器
// 	int period;
	
	
// };

	init_timer(&ioctl_timer_led.timer);

	ioctl_timer_led.timer.expires = jiffies + msecs_to_jiffies(1000);
	ioctl_timer_led.period = TIMER_PERIOD_1000;
	ioctl_timer_led.timer.function = timer_function;
	ioctl_timer_led.timer.data = (unsigned long)&ioctl_timer_led;
	

	/*定时器添加到系统*/
	
	add_timer(&ioctl_timer_led.timer);
	
	
	




	
	
	
    return 0;

failed_device:
	class_destroy(ioctl_timer_led.class);
failed_class:
	cdev_del(&ioctl_timer_led.cdev);
failed_cdev_add:
	unregister_chrdev_region(ioctl_timer_led.devid, IOCTL_TIMER_LED_CNT);
failed_devid:
	return ret;
}

static void __exit led_exit(void)
{
	
	gpio_set_value(ioctl_timer_led.led_gpio, 1);
	gpio_free(ioctl_timer_led.led_gpio);
	del_timer(&ioctl_timer_led.timer);
	device_destroy(ioctl_timer_led.class, ioctl_timer_led.devid);
	class_destroy(ioctl_timer_led.class);
	cdev_del(&ioctl_timer_led.cdev);
	unregister_chrdev_region(ioctl_timer_led.devid, IOCTL_TIMER_LED_CNT);

}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






