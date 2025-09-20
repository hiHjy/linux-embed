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
#include <linux/miscdevice.h>

#define MISCBEEP_NAME "miscbeep"
#define MISCBEEP_MINOR 144 
/*
	主要结构体定义
		
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};



struct device_driver {
	const char		*name;
	struct bus_type		*bus;

	struct module		*owner;
	const char		*mod_name;	

	bool suppress_bind_attrs;	

	const struct of_device_id	*of_match_table;
	const struct acpi_device_id	*acpi_match_table;

	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
	const struct attribute_group **groups;

	const struct dev_pm_ops *pm;

	struct driver_private *p;
};

struct of_device_id {
	char	name[32];
	char	type[32];
	char	compatible[128];
	const void *data;
};


struct miscdevice  {
	int minor;
	const char *name;
	const struct file_operations *fops;
	struct list_head list;
	struct device *parent;
	struct device *this_device;
	const struct attribute_group **groups;
	const char *nodename;
	umode_t mode;
};

*/






/*描述设备的结构体按需添加*/

struct miscbeep_dev {
	int gpio;
	struct device_node *nd;

} miscbeep;

/*操作函数*/

static int miscbeep_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &miscbeep;



	return 0;
}

static ssize_t miscbeep_write (struct file * filp, const char __user * buf, size_t count, loff_t *ppos)
{	
	struct miscbeep_dev *dev = (struct miscbeep_dev *)filp->private_data; 
	unsigned char databuf;
	copy_from_user(&databuf, buf, 1);

	if (databuf == '0') {
		gpio_set_value(dev->gpio, 1);
		
	} else {
		
		gpio_set_value(dev->gpio, 0);
	}
		
	return 0;
}

static int miscbeep_release(struct inode *inode, struct file *filp) {

	struct miscbeep_dev *dev = (struct miscbeep_dev *)filp->private_data;

	return 0;
}



/*操作集*/

static struct file_operations miscbeep_fops = {
	.owner 		= THIS_MODULE,
	.open 		= miscbeep_open,
	.write 		= miscbeep_write,
	.release 	= miscbeep_release,
	//.read		=, 
};


struct miscdevice miscbeep_misc_dev = {
	.minor 	= MISCBEEP_MINOR,
	.name 	= MISCBEEP_NAME,
	.fops 	= &miscbeep_fops,

};







/*初始化硬件*/

static int miscbeep_probe(struct platform_device *pdev)
{
	/*****************  初始化io     *****************************/
	
	
	
	printk(" miscbeep_probe\n");
	
	int ret = 0;

	/*获取设备树节点*/
	
	miscbeep.nd = pdev->dev.of_node;

	
	/*获取gpio*/
	
	miscbeep.gpio = of_get_named_gpio(miscbeep.nd, "beep-gpios", 0); //根据需要修改
	if (miscbeep.gpio < 0) {
		ret = -EINVAL;	//无效参数
		printk("of_get_named_gpio error \n");
		goto failed_get_gpio;
	}

	/*请求gpio*/
	
	ret = gpio_request(miscbeep.gpio, "miscbeep");
	if (ret < 0) {
		printk("gpio_request error\n");
		goto failed_request_gpio;
	}
	
	/*设置gpio方向*/
		//gpio_direction_input(unsigned gpio) 设置为输入
	
	ret = gpio_direction_output(miscbeep.gpio, 1); //设置为输出，初始为高电平
	if (ret < 0) {
		printk("gpio_direction_output error\n");
		goto failed_set_gpio_direction;
	}

	/**************************  字符设备驱动注册           ******************************************/

	/*普通字符设备注册*/

	
	/*misc驱动注册*/
	
	ret = misc_register(&miscbeep_misc_dev);
	if (ret < 0) {
		goto failed_misc_register;
	}
	
	
	return 0;

	/*异常情况*/
failed_misc_register:
failed_set_gpio_direction:
	gpio_free(miscbeep.gpio);
	
failed_request_gpio:
	
failed_get_gpio:

	return ret;
}


/*卸载驱动*/

static int miscbeep_remove (struct platform_device *dev)
{	
	
	printk(" miscbeep_remove\n");
	gpio_set_value(miscbeep.gpio, 1);
	gpio_free(miscbeep.gpio);
	misc_deregister(&miscbeep_misc_dev);
	return 0;
}

/*设备树匹配表*/

struct of_device_id miscbeep_match_table[] = {
	{.compatible = "huang,hjybeep",},
	{},
};

/* platform_driver 结构体*/

struct platform_driver miscbeep_drive = {
	.driver = {
		.name = "hjy-beep",
		.of_match_table = miscbeep_match_table,

	},

	.probe = miscbeep_probe,
	.remove = miscbeep_remove,


};

/*注册platform驱动*/

static int __init miscbeep_init(void)
{
	
	
	
	
	return platform_driver_register(&miscbeep_drive);
}


/*卸载platform驱动*/

static void __exit miscbeep_exit(void)
{

	/*会调用struct platform_driver结构体中的xxx_remove函数去释放资源*/

	platform_driver_unregister(&miscbeep_drive);  /*--  此函数参数为struct platform_driver*         -- */
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");























