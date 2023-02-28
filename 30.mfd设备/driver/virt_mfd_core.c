#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static const struct mfd_cell virt_mfd_devs[] = {
    {
        .name = "virt-mfd-dev1",
        .of_compatible = "xm,virt-mfd-dev1",
        .platform_data = "virt mfd dev1 platform data",
        .pdata_size = strlen("virt mfd dev2 platform data") + 1,
    },
    {
        .name = "virt-mfd-dev2",
        .of_compatible = "xm,virt-mfd-dev2",
        .platform_data = "virt mfd dev2 platform data",
        .pdata_size = strlen("virt mfd dev2 platform data") + 1,
    },
};

static int virt_mfd_core_probe(struct platform_device *pdev)
{
    printk("%s\n", __func__);
    dev_set_drvdata(&pdev->dev, "virt mfd core driver data");
    return devm_mfd_add_devices(&pdev->dev, PLATFORM_DEVID_NONE, virt_mfd_devs,
                                ARRAY_SIZE(virt_mfd_devs), NULL, 0, NULL);
}

static int virt_mfd_core_remove(struct platform_device *pdev)
{
    printk("%s\n", __func__);
    return 0;
}

static void virt_mfd_core_release(struct device *dev)
{
}

static struct platform_device virt_mfd_core_device = {
    .name = "virt-mfd-core-dev",
    .id = -1,
    .dev =
        {
            .release = virt_mfd_core_release,
        },
};

static struct platform_driver virt_mfd_core_driver = {
    .probe = virt_mfd_core_probe,
    .remove = virt_mfd_core_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "virt-mfd-core-dev",
        },
};

static int virt_mfd_core_init(void)
{
    printk("virt_mfd_core_init\n");
    platform_device_register(&virt_mfd_core_device);
    platform_driver_register(&virt_mfd_core_driver);

    return 0;
}

static void virt_mfd_core_exit(void)
{
    printk("virt_mfd_core_exit\n");

    platform_driver_unregister(&virt_mfd_core_driver);
    platform_device_unregister(&virt_mfd_core_device);
}

module_init(virt_mfd_core_init);
module_exit(virt_mfd_core_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt mfd core driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");