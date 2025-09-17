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
/*platform平台总线无设备树*/

#define CCM_CCGR1_BASE				(0X020C406C)	
#define SW_MUX_GPIO1_IO03_BASE		(0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0X020E02F4)
#define GPIO1_DR_BASE				(0X0209C000)
#define GPIO1_GDIR_BASE				(0X0209C004)
#define REGISTER_LENGTH				4

void leddevice_release(struct device *dev)
{
	printk("leddevice_release 执行\r\n");
 
}
//struct resource {
//	resource_size_t start;
//	resource_size_t end;
//	const char *name;
//	unsigned long flags;
//	struct resource *parent, *sibling, *child;
//};

 /*寄存器资源*/ 
static struct resource led_resources[] = {
	{
		/*资源起始地址*/
		.start = CCM_CCGR1_BASE,
		.end = CCM_CCGR1_BASE + REGISTER_LENGTH -1,
		.flags = IORESOURCE_MEM,
	},
	
	{
		.start = SW_MUX_GPIO1_IO03_BASE,
		.end = SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH -1,
		.flags = IORESOURCE_MEM,
	},

	{
		.start = SW_PAD_GPIO1_IO03_BASE,
		.end = SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH -1,
		.flags = IORESOURCE_MEM,
	},

	{
		.start = GPIO1_DR_BASE,
		.end = GPIO1_DR_BASE + REGISTER_LENGTH -1,
		.flags = IORESOURCE_MEM,
	},

	{
		.start = GPIO1_GDIR_BASE,
		.end = GPIO1_GDIR_BASE + REGISTER_LENGTH -1,
		.flags = IORESOURCE_MEM,
	},
	
};

static struct platform_device leddevice = {
	.name 	= "imx6ull-led",
	.id		= -1,/*表示此设备无id*/
	.dev = {
		.release =  leddevice_release,
	},
	
	/*重点:资源的数量*/
	.num_resources = ARRAY_SIZE(led_resources),
	.resource = led_resources,


};
//platform_device_register(struct platform_device * pdev)



/*设备加载*/
static int __init leddevice_init(void)
{
	/*注册platform设备*/
	return platform_device_register(&leddevice);

}

/*设备卸载*/

static void __exit leddevice_exit(void)
{
	/*卸载platform设备*/
	platform_device_unregister(&leddevice);

}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");
