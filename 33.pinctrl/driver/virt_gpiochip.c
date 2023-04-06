#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

struct virt_gpiochip {
    struct gpio_chip gc;
};

static int virt_gpiochip_get_value(struct gpio_chip *chip, unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static void virt_gpiochip_set_value(struct gpio_chip *chip, unsigned offset,
                                    int value)
{
    printk("%s\n", __FUNCTION__);
}

static int virt_gpiochip_get_direction(struct gpio_chip *chip, unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_gpiochip_direction_input(struct gpio_chip *chip,
                                         unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_gpiochip_direction_output(struct gpio_chip *chip,
                                          unsigned offset, int value)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_gpiochip_probe(struct platform_device *pdev)
{
    struct virt_gpiochip *virt_gpiochip;
    int ret;

    printk("%s\n", __FUNCTION__);
    virt_gpiochip =
        devm_kzalloc(&pdev->dev, sizeof(struct virt_gpiochip), GFP_KERNEL);
    if (!virt_gpiochip) {
        dev_err(&pdev->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    virt_gpiochip->gc.label = pdev->name;
    virt_gpiochip->gc.base = -1;
    virt_gpiochip->gc.owner = THIS_MODULE;
    virt_gpiochip->gc.ngpio = 4;
    virt_gpiochip->gc.can_sleep = 0;
    virt_gpiochip->gc.request = gpiochip_generic_request;
    virt_gpiochip->gc.free = gpiochip_generic_free;
    virt_gpiochip->gc.get = virt_gpiochip_get_value;
    virt_gpiochip->gc.set = virt_gpiochip_set_value;
    virt_gpiochip->gc.get_direction = virt_gpiochip_get_direction;
    virt_gpiochip->gc.direction_input = virt_gpiochip_direction_input;
    virt_gpiochip->gc.direction_output = virt_gpiochip_direction_output;
    virt_gpiochip->gc.of_node = pdev->dev.of_node;

    ret = gpiochip_add(&virt_gpiochip->gc);
    if (ret) {
        dev_err(&pdev->dev, "Could not register gpio chip %d\n", ret);
        return ret;
    }

    platform_set_drvdata(pdev, virt_gpiochip);

    return 0;
}

static int virt_gpiochip_remove(struct platform_device *pdev)
{
    struct virt_gpiochip *virt_gpiochip;

    printk("%s\n", __FUNCTION__);
    virt_gpiochip = platform_get_drvdata(pdev);
    gpiochip_remove(&virt_gpiochip->gc);

    return 0;
}

static const struct of_device_id virt_gpiochip_of_match[] = {
    {.compatible = "xm,virt-gpiochip"},
    {},
};

static struct platform_driver virt_gpiochip_driver = {
    .probe = virt_gpiochip_probe,
    .remove = virt_gpiochip_remove,
    .driver =
        {
            .name = "virt-gpiochip",
            .of_match_table = virt_gpiochip_of_match,
        },
};

module_platform_driver(virt_gpiochip_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt gpio chip driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
