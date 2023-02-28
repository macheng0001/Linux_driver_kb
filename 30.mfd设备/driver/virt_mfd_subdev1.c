#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>

static int virt_mfd_subdev1_probe(struct platform_device *pdev)
{
    char *drvdata = dev_get_drvdata(pdev->dev.parent);
    char *pdata = dev_get_platdata(&pdev->dev);
    const struct mfd_cell *cell = mfd_get_cell(pdev);

    printk("%s:\n\tdrvdata:%s\n\tpdata:%s\n\tcell->name:%s\n\tcell->platform_"
           "data:%s\n",
           __func__, drvdata, pdata, cell->name, (char *)cell->platform_data);
    return 0;
}

static int virt_mfd_subdev1_remove(struct platform_device *pdev)
{
    printk("%s\n", __func__);
    return 0;
}

static const struct of_device_id virt_mfd_subdev_of_id_table[] = {
    {.compatible = "xm,virt-mfd-dev1"},
    {},
};
MODULE_DEVICE_TABLE(of, virt_mfd_subdev_of_id_table);

static struct platform_driver virt_mfd_subdev_driver = {
    .driver =
        {
            .name = "virt-mfd-dev1",
            .of_match_table = virt_mfd_subdev_of_id_table,
        },
    .probe = virt_mfd_subdev1_probe,
    .remove = virt_mfd_subdev1_remove,
};

module_platform_driver(virt_mfd_subdev_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt mfd sub device driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");