#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

static int val = 123;
static int major;  //主设备号
ssize_t hello_read (struct file *, char __user *, size_t, loff_t *);
ssize_t hello_write (struct file *, const char __user *, size_t, loff_t *);

ssize_t hello_read (struct file *f, char __user *buf, size_t size, loff_t *arg) 
{
	printk("--%s-- %s-- line %d 驱动read被调用\n", __FILE__, __FUNCTION__, __LINE__);
	copy_to_user(buf, &val, 4);

	return 1;
}
ssize_t hello_write (struct file *f, const char __user *buf, size_t size, loff_t *arg)
{
	
	printk("--%s-- %s-- line %d 驱动write被调用\n", __FILE__, __FUNCTION__, __LINE__);
	copy_from_user(&val, buf, 4);
	return 0;
}

static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.read = hello_read,
	.write = hello_write
};


int __init hello_init(void)
{
	printk("入口函数\n");
	
	/*确定主设备号*/
	
	major = register_chrdev(0, "hello_drv", &hello_fops); //注册了一个字符驱动
	return 0;
}

void __exit hello_exit(void)
{
	unregister_chrdev(major, "hello_drv");
	
}

MODULE_LICENSE("GPL");

module_exit(hello_exit);
module_init(hello_init);



