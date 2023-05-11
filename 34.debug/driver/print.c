#define pr_fmt(fmt) "xm: " fmt
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>

static int print_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    printk(KERN_EMERG "KERN_EMERG\n");
    printk(KERN_ALERT "KERN_ALERT\n");
    printk(KERN_CRIT "KERN_CRIT\n");
    printk(KERN_ERR "KERN_ERR\n");
    printk(KERN_WARNING "KERN_WARNING\n");
    printk(KERN_NOTICE "KERN_NOTICE\n");
    printk(KERN_INFO "KERN_INFO\n");
    printk(KERN_DEBUG "KERN_DEBUG\n");
    printk("the default kernel loglevel\n");

    pr_emerg("pr_emerg\n");
    pr_alert("pr_alert\n");
    pr_crit("pr_crit\n");
    pr_err("pr_err\n");
    pr_warn("pr_warn\n");
    pr_notice("pr_notice\n");
    pr_info("pr_info\n");
    pr_debug("pr_debug\n");

    dev_emerg(dev, "dev_emerg\n");
    dev_alert(dev, "dev_alert\n");
    dev_crit(dev, "dev_crit\n");
    dev_err(dev, "dev_err\n");
    dev_warn(dev, "dev_warn\n");
    dev_notice(dev, "dev_notice\n");
    dev_info(dev, "dev_info\n");
    dev_dbg(dev, "dev_debug\n");

    WARN_ON(1);
    BUG_ON(1);

    return 0;
}

static int print_remove(struct platform_device *pdev)
{
    return 0;
}

static void print_release(struct device *dev)
{
}

static struct platform_device print_device = {
    .name = "print-dev",
    .id = -1,
    .dev =
        {
            .release = print_release,
        },
};

static struct platform_driver print_driver = {
    .probe = print_probe,
    .remove = print_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "print-dev",
        },
};

static int print_init(void)
{
    platform_device_register(&print_device);
    platform_driver_register(&print_driver);
    return 0;
}

static void print_exit(void)
{
    platform_driver_unregister(&print_driver);
    platform_device_unregister(&print_device);
}

module_init(print_init);
module_exit(print_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("print test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");