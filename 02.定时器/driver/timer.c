#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>

static struct timer_list timer;

static void my_function(unsigned long data)
{
    printk("timer callback:%lx\n", data);

    mod_timer(&timer, jiffies + 3 * HZ);

    return;
}

static int timer_drv_init(void)
{
    printk("timer drv init\n");

    timer.data = 0xaa;
    timer.function = my_function;
    timer.expires = jiffies + 3 * HZ;
    init_timer(&timer);
    add_timer(&timer);

    return 0;
}

static void timer_drv_exit(void)
{
    printk("timer drv exit\n");

    del_timer_sync(&timer);
}

module_init(timer_drv_init);
module_exit(timer_drv_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("timer driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
