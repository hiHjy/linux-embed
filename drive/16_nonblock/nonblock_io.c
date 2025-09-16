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
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>  
#define IRQ_GPIO_KEY_CNT 1
#define IRQ_GPIO_KEY_NAME "irq_gpio_key"
#define KEY_NUM 1

#define KEY0VALUE 0X0C
#define INVAKEY 0X0
struct irq_key_desc {
	int gpio;			/*io编号 */
	int irq_num;		/*中断号 */
	unsigned char value;/*键值 */
	char name[10];		/*按键名字*/
	irqreturn_t (*handler) (int, void*);					/*中断处理函数*/
	struct tasklet_struct tasklet;
	
};
struct irq_gpio_key {
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	struct timer_list timer; //定时器
	struct irq_key_desc irq_key[KEY_NUM];
	atomic_t key_value;
	atomic_t isrelease;
	struct tasklet_struct tasklet;
	struct work_struct work;
	wait_queue_head_t r_wait;

	 
	
};
struct irq_gpio_key irq_gpio_key;





static int irq_gpio_key_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &irq_gpio_key;
	


	return 0;
}

static int irq_gpio_key_release(struct inode *inode, struct file *filp) {

	
	return 0;
}

unsigned int irq_gpio_key_poll (struct file * filp, struct poll_table_struct * wait) 
{

	int mask = 0;
	struct irq_gpio_key *dev = filp->private_data;

	poll_wait(filp, &dev->r_wait, wait);

	//按键按下可读
	if (atomic_read(&dev->isrelease)) {
		mask = POLLIN |POLLRDNORM;
		return mask;
	}
	return 0;
	

   
	

}

ssize_t irq_gpio_key_read (struct file *filp, char __user *buf, size_t count, loff_t *offt)
{
	
	struct irq_gpio_key *dev = filp->private_data;
	int ret = 0;
	printk("filp->f_flags & O_NONBLOCK:%d\n", filp->f_flags & O_NONBLOCK);
	if (filp->f_flags & O_NONBLOCK) {
		/*非阻塞的方式*/
		printk("非阻塞读\n");
		if (atomic_read(&dev->isrelease) == 0) {

			return -EAGAIN;
		}
	} else {
		/*阻塞的方式 */
		//wait_event_interruptible(dev->r_wait, atomic_read(&dev->isrelease));
		printk("阻塞的读分支\n");
	}
	
#if 0 
	/*实现阻塞io的第一种方式：可中断的等待：wait_event_interruptible() wake_up() 这一对函数*/

	/*可被信号打断*/

	wait_event_interruptible(dev->r_wait, atomic_read(&dev->isrelease));//等待按键被按下，isrelease为0
#endif

#if 0
	/*实现阻塞io的第二种方式:等待队列*/
	DECLARE_WAITQUEUE(wait, current); //声明一个等待队列项名字为wait，与当前进程绑定起来
	
	if (atomic_read(&dev->isrelease) == 0) {
		add_wait_queue(&dev->r_wait, &wait);/*将队列项添加到等待队列头(入队)*/
		__set_current_state(TASK_INTERRUPTIBLE); /*设置可以被信号打断*/
		schedule(); /*进入休眠状态*/

		/*检测被唤醒的原因*/
		
		if (signal_pending(current)) {
			/*被信号唤醒*/
			ret = -ERESTARTSYS;
			goto data_error;
		}
		
		__set_current_state(TASK_RUNNING); /*设置当前进程为运行状态*/
		remove_wait_queue(&dev->r_wait, &wait);
		
	}
#endif
	unsigned char keyvalue = atomic_read(&dev->key_value);
	unsigned char is_release = atomic_read(&dev->isrelease);
	

	if (is_release) {
			
				copy_to_user(buf, &keyvalue, sizeof(keyvalue));
				atomic_set(&dev->isrelease, 0);	
				
				return 1;
					
	}
	
     	return 0;
#if 0	
data_error:
	__set_current_state(TASK_RUNNING); /*设置当前进程为运行状态*/
	remove_wait_queue(&dev->r_wait, &wait);

	return ret;
#endif

}

static const struct file_operations irq_gpio_key_fops = {
		.owner		= THIS_MODULE,
		.read 		= irq_gpio_key_read,
		.open 		= irq_gpio_key_open,
		.release 	= irq_gpio_key_release,
		.poll  		= irq_gpio_key_poll,
};
/*定时器处理函数*/
void timer_func(unsigned long data)
{
	int value = 0;
	struct irq_gpio_key *dev = (struct irq_gpio_key *)data;
	
	value = gpio_get_value(dev->irq_key[0].gpio);
	
	if (value == 0) {
		
		atomic_set(&dev->isrelease, 0);
		
		//如果按键被按下，那么唤醒阻塞在read函数中wait_event(dev->r_wait, !atomic_read(&dev->isrelease))
		
	
	}  else {

		
		wake_up(&dev->r_wait);

		atomic_set(&dev->isrelease, 1);
		
	}
}
/*中断处理函数*/
static irqreturn_t handler_fun(int irq, void *arg) 
{
//	int value = 0;
	struct irq_gpio_key *dev = (struct irq_gpio_key *)arg;
//	
//	value = gpio_get_value(dev->irq_key[0].gpio);
//	if (value == 0) {
//		printk("按下\n");
//	}  else {
//		printk("释放\n");
//	}

//	dev->timer.data = (volatile unsigned long)arg;
//	mod_timer(&irq_gpio_key.timer, jiffies + msecs_to_jiffies(10));
	//tasklet_schedule(&dev->irq_key[0].tasklet);
	schedule_work(&dev->work);
	return IRQ_HANDLED; 
}

///*下半部 tasklet 任务函数*/
//void tasklet_handler(unsigned long data)
//{
//
//	int value = 0;
//	
//	struct irq_gpio_key *dev = (struct irq_gpio_key *)data;
//	dev->timer.data = (volatile unsigned long)data;
//	mod_timer(&irq_gpio_key.timer, jiffies + msecs_to_jiffies(10));
//	
//
//}
static void work_handler(struct work_struct *work)
{
	
	
	
	struct irq_gpio_key *dev = container_of(work, struct irq_gpio_key, work);
	dev->timer.data = (volatile unsigned long)dev;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
	
}

int irq_gpio_key_init(struct irq_gpio_key *dev)
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
		
		
		/*中断初始化*/
		
#if 1
		dev->irq_key[i].irq_num = gpio_to_irq(dev->irq_key[i].gpio);
#endif

#if 0
	
		dev->irq_key[i].irq_num = irq_of_parse_and_map(dev->nd, i);
#endif		
		dev->irq_key[i].handler = handler_fun;//所有按键公用一个中断处理函数
		ret = request_irq(dev->irq_key[i].irq_num, dev->irq_key[i].handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
											dev->irq_key[i].name, &irq_gpio_key);

		if (ret) {
			printk("request_irq error\n");
			goto failed_request_irq;
		}
		
		/*下半部初始化*/
		
		//tasklet_init(&dev->irq_key[i].tasklet,tasklet_handler, dev);
		INIT_WORK(&dev->work, work_handler);
		
	}


	
	return 0;
failed_request_irq:
		for (i = 0; i < KEY_NUM; ++i) {
			if (dev->irq_key[i].irq_num >= 0)
			free_irq(dev->irq_key[i].irq_num,dev);
		}
failed_direction:
		for (i = 0; i < KEY_NUM; ++i) {
			if (dev->irq_key[i].gpio >= 0)
			gpio_free(dev->irq_key[i].gpio);
		}
			
failed_find_nd:
	
	return ret;

}

/*驱动入口出口函数*/
static int __init led_init(void) 
{
	/*注册字符设备驱动*/
	//irq_gpio_key.major = 0;
	//自动申请设备号

	int ret = alloc_chrdev_region(&irq_gpio_key.devid, 0, IRQ_GPIO_KEY_CNT, IRQ_GPIO_KEY_NAME);
	if (ret < 0) {
		printk("alloc_chrdev_region(&irq_gpio_key.devid, 0, IRQ_GPIO_KEY_CNT, IRQ_GPIO_KEY_NAME)\n");
		goto failed_devid;
	}
		
	irq_gpio_key.major = MAJOR(irq_gpio_key.devid);
	irq_gpio_key.minor = MINOR(irq_gpio_key.devid);

	printk("irq_gpio_key major:%d 	minor:%d\n", irq_gpio_key.major, irq_gpio_key.minor);

	/*初始化cdev*/
	irq_gpio_key.cdev.owner = THIS_MODULE;
	cdev_init(&irq_gpio_key.cdev, &irq_gpio_key_fops);

	/*添加cdev*/
	ret = cdev_add(&irq_gpio_key.cdev, irq_gpio_key.devid, IRQ_GPIO_KEY_CNT);
	if (ret < 0) {
		printk("cdev_add(&irq_gpio_key.cdev, irq_gpio_key.devid, IRQ_GPIO_KEY_CNT)\n");
		goto failed_cdev_add;
	}

	/*创建类*/

	irq_gpio_key.class = class_create(THIS_MODULE, IRQ_GPIO_KEY_NAME);
	if (IS_ERR(irq_gpio_key.class)) {
		printk("irq_gpio_key.class = class_create(THIS_MODULE, IRQ_GPIO_KEY_NAME)\n");
		goto failed_class;
	}

	/*创建设备*/

	irq_gpio_key.device = device_create(irq_gpio_key.class, NULL, irq_gpio_key.devid, NULL, IRQ_GPIO_KEY_NAME);
	if (IS_ERR(irq_gpio_key.device)) {
		printk("irq_gpio_key.device = device_create(irq_gpio_key.class, NULL, irq_gpio_key.devid, NULL, IRQ_GPIO_KEY_NAME)\n");
		goto failed_device;
	}

	/*初始化led io*/
	ret = irq_gpio_key_init(&irq_gpio_key);
	if (ret < 0) {
	 	goto failed_device;
	}

	/*初始化定时器*/
	init_timer(&irq_gpio_key.timer);
	irq_gpio_key.timer.function = timer_func;

	atomic_set(&irq_gpio_key.isrelease, 0);
	atomic_set(&irq_gpio_key.key_value, KEY0VALUE);

	/*初始化等待队列头*/

	init_waitqueue_head(&irq_gpio_key.r_wait);
    return 0;

failed_device:
	
	class_destroy(irq_gpio_key.class);
failed_class:
	cdev_del(&irq_gpio_key.cdev);
failed_cdev_add:
	unregister_chrdev_region(irq_gpio_key.devid, IRQ_GPIO_KEY_CNT);
failed_devid:
	del_timer_sync(&irq_gpio_key.timer);
	return ret;
}

static void __exit led_exit(void)
{
	
	int i;
	/*删除定时器*/
	del_timer_sync(&irq_gpio_key.timer);
	for (i = 0; i < KEY_NUM; ++i) {
			if (irq_gpio_key.irq_key[i].irq_num >= 0)
			free_irq(irq_gpio_key.irq_key[i].irq_num, &irq_gpio_key);
		}
	
	for (i = 0; i < KEY_NUM; ++i) {
		if (irq_gpio_key.irq_key[i].gpio >= 0) {
		
			gpio_free(irq_gpio_key.irq_key[i].gpio);
		}
	}

	device_destroy(irq_gpio_key.class, irq_gpio_key.devid);
	class_destroy(irq_gpio_key.class);
	cdev_del(&irq_gpio_key.cdev);
	unregister_chrdev_region(irq_gpio_key.devid, IRQ_GPIO_KEY_CNT);

}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("黄纪元");






