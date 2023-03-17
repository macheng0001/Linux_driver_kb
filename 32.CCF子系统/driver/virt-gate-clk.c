#include <linux/module.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>

//#define USE_DTS

struct virt_gate_clk_priv {
    atomic_t gate_en;
    struct clk_hw gate;
};
#define to_virt_gate_clk(_hw) container_of(_hw, struct virt_gate_clk_priv, gate)

static int virt_gate_clk_enable(struct clk_hw *hw)
{
    struct virt_gate_clk_priv *priv = to_virt_gate_clk(hw);
    printk("%s\n", __func__);
    atomic_set(&priv->gate_en, 1);
    return 0;
}

static void virt_gate_clk_disable(struct clk_hw *hw)
{
    struct virt_gate_clk_priv *priv = to_virt_gate_clk(hw);
    printk("%s\n", __func__);
    atomic_set(&priv->gate_en, 0);
}

static int virt_gate_clk_is_enabled(struct clk_hw *hw)
{
    struct virt_gate_clk_priv *priv = to_virt_gate_clk(hw);
    printk("%s\n", __func__);
    return atomic_read(&priv->gate_en);
}

static struct clk_ops virt_gate_clk_ops = {
    .enable = virt_gate_clk_enable,
    .disable = virt_gate_clk_disable,
    .is_enabled = virt_gate_clk_is_enabled,
};

static int virt_gate_clk_probe(struct platform_device *pdev)
{
    struct virt_gate_clk_priv *priv;
    struct clk *clk;
    struct clk_init_data init_data;
    const char *parent_name = "virt_osc";

    printk("%s\n", __func__);
#ifdef USE_DTS
    parent_name = of_clk_get_parent_name(pdev->dev.of_node, 0);
    if (!parent_name) {
        return -ENOENT;
    }
#endif
    priv =
        devm_kzalloc(&pdev->dev, sizeof(struct virt_gate_clk_priv), GFP_KERNEL);
    if (!priv) {
        printk("can not alloc memory\n");
        return -ENOMEM;
    }
    init_data.name = "virt_gate";
    init_data.ops = &virt_gate_clk_ops;
    init_data.parent_names = &parent_name;
    init_data.num_parents = 1;
    priv->gate.init = &init_data;
    clk = clk_register(NULL, &priv->gate);
    if (IS_ERR(clk)) {
        return PTR_ERR(clk);
    }
#ifdef USE_DTS
    of_clk_add_hw_provider(pdev->dev.of_node, of_clk_hw_simple_get,
                           &priv->gate);
#else
    clk_register_clkdev(clk, "virt_gate", NULL);
#endif

    platform_set_drvdata(pdev, priv);

    return 0;
}

static int virt_gate_clk_remove(struct platform_device *pdev)
{
    struct virt_gate_clk_priv *priv = platform_get_drvdata(pdev);
    printk("%s\n", __func__);
    clk_unregister(priv->gate.clk);
    return 0;
}

static const struct of_device_id virt_gate_clk_of_match[] = {
    {.compatible = "xm,virt-gate-clk"},
    {},
};

static struct platform_driver virt_gate_clk_driver = {
    .probe = virt_gate_clk_probe,
    .remove = virt_gate_clk_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "virt-gate-clk",
            .of_match_table = virt_gate_clk_of_match,
        },
};
#ifdef USE_DTS
module_platform_driver(virt_gate_clk_driver);
#else
static void virt_gate_clk_release(struct device *dev)
{
}

static struct platform_device virt_gate_clk_device = {
    .name = "virt-gate-clk",
    .id = -1,
    .dev =
        {
            .release = virt_gate_clk_release,
        },
};

static int virt_gate_clk_init(void)
{
    printk("%s\n", __func__);
    platform_device_register(&virt_gate_clk_device);
    platform_driver_register(&virt_gate_clk_driver);

    return 0;
}

static void virt_gate_clk_exit(void)
{
    printk("%s\n", __func__);
    platform_driver_unregister(&virt_gate_clk_driver);
    platform_device_unregister(&virt_gate_clk_device);
}

module_init(virt_gate_clk_init);
module_exit(virt_gate_clk_exit);
#endif
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt gate clk driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");