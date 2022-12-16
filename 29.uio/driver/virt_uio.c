#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/gpio.h>

struct virt_uio_priv {
    struct uio_info *uio_info;
    struct gpio_desc *gpio;
    unsigned long flag;
};

static irqreturn_t virt_uio_handler(int irq, struct uio_info *dev_info)
{
    printk("virt_uio_handler\n");
    return IRQ_HANDLED;
}

static int virt_uio_irqcontrol(struct uio_info *uio_info, s32 irq_on)
{
    struct virt_uio_priv *priv = uio_info->priv;
    printk("virt_uio:%s\n", irq_on ? "irq_on" : "irq_off");
    if (irq_on) {
        if (!test_and_set_bit(0, &priv->flag))
            enable_irq(uio_info->irq);
    } else {
        if (test_and_clear_bit(0, &priv->flag))
            disable_irq_nosync(uio_info->irq);
    }

    return 0;
}

static int virt_uio_probe(struct platform_device *pdev)
{
    struct virt_uio_priv *priv;
    struct uio_info *uio_info;
    struct uio_mem *uio_mem;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&pdev->dev, "unable to kmalloc priv\n");
        return -ENOMEM;
    }

    uio_info = devm_kzalloc(&pdev->dev, sizeof(*uio_info), GFP_KERNEL);
    if (!uio_info) {
        dev_err(&pdev->dev, "unable to kmalloc uio_info\n");
        return -ENOMEM;
    }

    uio_info->name = "virt-uio";
    uio_info->version = "virt uio test";

    priv->gpio = devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_IN);
    if (IS_ERR(priv->gpio)) {
        dev_err(&pdev->dev, "unable to get gpio\n");
        return PTR_ERR(priv->gpio);
    }
    uio_info->irq = gpiod_to_irq(priv->gpio);
    uio_info->irq_flags = IRQF_TRIGGER_RISING;
    uio_info->irqcontrol = virt_uio_irqcontrol;
    uio_info->handler = virt_uio_handler;

    uio_mem = &uio_info->mem[0];
    uio_mem->addr = (unsigned long)devm_kzalloc(&pdev->dev, 1024, GFP_KERNEL);
    if (!priv) {
        dev_err(&pdev->dev, "unable to kmalloc uio_mem\n");
        return -ENOMEM;
    }
    uio_mem->size = 1024;
    uio_mem->memtype = UIO_MEM_LOGICAL;

    set_bit(0, &priv->flag);
    priv->uio_info = uio_info;
    uio_info->priv = priv;
    platform_set_drvdata(pdev, priv);

    ret = uio_register_device(&pdev->dev, uio_info);
    if (ret) {
        dev_err(&pdev->dev, "unable to register uio device\n");
        return ret;
    }

    return 0;
}

static int virt_uio_remove(struct platform_device *pdev)
{
    struct virt_uio_priv *priv = platform_get_drvdata(pdev);

    uio_unregister_device(priv->uio_info);

    return 0;
}

static const struct of_device_id virt_uio_of_match[] = {
    {.compatible = "virt-uio"},
    {/* Sentinel */},
};

static struct platform_driver virt_uio_driver = {
    .probe = virt_uio_probe,
    .remove = virt_uio_remove,
    .driver =
        {
            .name = "virt-uio",
            .of_match_table = virt_uio_of_match,
        },
};

module_platform_driver(virt_uio_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virtual uio driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");