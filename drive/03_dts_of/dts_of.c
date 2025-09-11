#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
static u32 *arr;

static int __init dtsof_init(void)
{
	int ret = 0;
	/*/home/hjy/linux/imx6ull/kernel/nxp/linux-imx-rel_imx_4.1.15_2.1.0_ga/arch/arm/boot/dts/imx6ull-14x14-hjy-emmc.dts*/
	struct device_node *bl_nd = NULL;//设备树文件
	
	/*1，查找根节点下的backlight节点*/
	bl_nd = of_find_node_by_path("/backlight");
	if (!bl_nd) {
		ret = -EINVAL;
		goto faile_findnd;
	}
	
	/*
		backlight {
			compatible = "pwm-backlight";
			pwms = <&pwm1 0 5000000>;
			brightness-levels = <0 4 8 16 32 64 128 255>;
			default-brightness-level = <6>;
			status = "okay";
		};

	*/
	/*2，获取属性*/
		//of_property_read_string(struct device_node * np, const char * propname, const char * * out_string)

		//compatible	字符串
	struct property *pp = NULL; 
	pp = of_find_property(bl_nd, "compatible", NULL);
		//status
	
	
	if (!pp) {
		ret = -EINVAL;
		goto faile_findpro;
	} else {
		printk("compatible:%s\n", (char *)pp->value);
	
	}

		//status 	字符串
	const char *out_string;
	if ((ret = of_property_read_string(bl_nd, "status", &out_string)) < 0) {
		
		goto faile_findpro;
	} else {
		printk("status:%s\n", out_string);
	
	}

		//default-brightness-level 数字

	u32 out_value;
	ret = of_property_read_u32(bl_nd, "default-brightness-level", &out_value);
	
	if (ret < 0) 
		goto faile_findpro;
	else 
		printk("default-brightness-level:%d\r\n", out_value);

		//brightness-levels 获取："数组" 类型的属性
	u32 size = -1;
	if ((size = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32))) < 0) {
		goto faile_findpro;
	} else {

		//申请内存
		printk("property:brightness-levels elem size is %d\n", size);
		arr = kmalloc(size * sizeof(u32), GFP_KERNEL);
		if (arr < 0) {
			printk(" kmalloc error\n");
			goto faile_findpro;
		}
		
		ret = of_property_read_u32_array(bl_nd, "brightness-levels", arr, size);
		if (ret < 0) {
			printk("retof_property_read_u32_array error\n");
			goto faile_findpro;
		}
		int i;
		for (i = 0; i < size; ++i) {
			printk("%d ", arr[i]);
	
		}
		printk("\n");
		
	}
	return 0;
	
faile_findnd:
faile_findpro:
faile_findpro_arr:
	kfree(arr);
	return ret;
}


static void __exit dtsof_exit(void)
{

	



}
/*注册模块入口函数*/

module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");




