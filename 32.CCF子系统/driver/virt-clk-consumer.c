#include <linux/module.h>
#include <linux/clkdev.h>
#include <asm/uaccess.h>
#include <linux/clk.h>
#include <linux/platform_device.h>

//#define USE_DTS

static int virt_clk_consumer_probe(struct platform_device *pdev)
{
    struct clk *clk = clk_get(&pdev->dev, "virt_gate");
    printk("%s\n", __func__);
    if (IS_ERR(clk)) {
        printk("could not get virt_gate clock\n");
        return PTR_ERR(clk);
    }
    clk_prepare_enable(clk);
    platform_set_drvdata(pdev, clk);
    return 0;
}

static int virt_clk_consumer_remove(struct platform_device *pdev)
{
    struct clk *clk = platform_get_drvdata(pdev);
    printk("%s\n", __func__);
    clk_disable_unprepare(clk);
    clk_put(clk);
    return 0;
}

static const struct of_device_id virt_clk_consumer_of_match[] = {
    {.compatible = "xm,virt-clk-consumer"},
    {},
};

static struct platform_driver virt_clk_consumer_driver = {
    .probe = virt_clk_consumer_probe,
    .remove = virt_clk_consumer_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "virt-clk-consumer",
            .of_match_table = virt_clk_consumer_of_match,
        },
};
#ifdef USE_DTS
module_platform_driver(virt_clk_consumer_driver);
#else
static void virt_clk_consumer_release(struct device *dev)
{
}

static struct platform_device virt_clk_consumer_device = {
    .name = "virt-clk-consumer",
    .id = -1,
    .dev =
        {
            .release = virt_clk_consumer_release,
        },
};

static int virt_clk_consumer_init(void)
{
    printk("%s\n", __func__);
    platform_device_register(&virt_clk_consumer_device);
    platform_driver_register(&virt_clk_consumer_driver);

    return 0;
}

static void virt_clk_consumer_exit(void)
{
    printk("%s\n", __func__);
    platform_driver_unregister(&virt_clk_consumer_driver);
    platform_device_unregister(&virt_clk_consumer_device);
}
module_init(virt_clk_consumer_init);
module_exit(virt_clk_consumer_exit);
#endif
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt clk consumer driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");