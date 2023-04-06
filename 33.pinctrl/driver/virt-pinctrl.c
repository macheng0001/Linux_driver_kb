#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-utils.h"

struct virt_pinctrl_group {
    const char *name;
    const unsigned int *pins;
    const unsigned npins;
};

struct virt_pinmux_function {
    const char *name;
    const char *const *groups;
    unsigned ngroups;
};

struct virt_pinctrl_priv {
    struct pinctrl_dev *pctldev;
    const struct virt_pinctrl_group *groups;
    unsigned int ngroups;
    const struct virt_pinmux_function *funcs;
    unsigned int nfuncs;
};

static const struct pinctrl_pin_desc virt_pins[] = {
    PINCTRL_PIN(0, "VIO0"),
    PINCTRL_PIN(1, "VIO1"),
    PINCTRL_PIN(2, "VIO2"),
    PINCTRL_PIN(3, "VIO3"),
};

static const unsigned int i2c0_pins[] = {0, 1};
static const unsigned int i2c1_pins[] = {2, 3};
static const unsigned int spi0_pins[] = {0, 1, 2, 3};

#define VIRT_PINCTRL_GRP(nm)                                                   \
    {                                                                          \
        .name = #nm "_group", .pins = nm##_pins,                               \
        .npins = ARRAY_SIZE(nm##_pins),                                        \
    }

static const struct virt_pinctrl_group virt_pinctrl_group[] = {
    VIRT_PINCTRL_GRP(i2c0),
    VIRT_PINCTRL_GRP(i2c1),
    VIRT_PINCTRL_GRP(spi0),
};

static const char *const i2c0_group[] = {"i2c0_group"};
static const char *const i2c1_group[] = {"i2c1_group"};
static const char *const spi0_group[] = {"spi0_group"};

#define VIRT_PINMUX_FUNCTION(fname)                                            \
    {                                                                          \
        .name = #fname, .groups = fname##_group,                               \
        .ngroups = ARRAY_SIZE(fname##_group),                                  \
    }

static const struct virt_pinmux_function virt_pinmux_function[] = {
    VIRT_PINMUX_FUNCTION(i2c0),
    VIRT_PINMUX_FUNCTION(i2c1),
    VIRT_PINMUX_FUNCTION(spi0),
};

static int virt_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    printk("%s\n", __func__);
    return priv->ngroups;
}

static const char *virt_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
                                               unsigned selector)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    printk("%s: selector=%d\n", __func__, selector);
    return priv->groups[selector].name;
}

static int virt_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
                                       unsigned selector, const unsigned **pins,
                                       unsigned *num_pins)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    printk("%s: selector=%d\n", __func__, selector);
    *pins = priv->groups[selector].pins;
    *num_pins = priv->groups[selector].npins;

    return 0;
}

static const struct pinctrl_ops virt_pinctrl_ops = {
    .get_groups_count = virt_pinctrl_get_groups_count,
    .get_group_name = virt_pinctrl_get_group_name,
    .get_group_pins = virt_pinctrl_get_group_pins,
    .dt_node_to_map = pinconf_generic_dt_node_to_map_all,
    .dt_free_map = pinctrl_utils_free_map,
};

static const char *virt_pinmux_get_function_name(struct pinctrl_dev *pctldev,
                                                 unsigned selector)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    printk("%s: funcname=%s\n", __FUNCTION__, priv->funcs[selector].name);

    return priv->funcs[selector].name;
}

static int virt_pinmux_get_functions_count(struct pinctrl_dev *pctldev)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    printk("%s\n", __FUNCTION__);

    return priv->nfuncs;
}

static int virt_pinmux_get_function_groups(struct pinctrl_dev *pctldev,
                                           unsigned selector,
                                           const char *const **groups,
                                           unsigned *num_groups)
{
    struct virt_pinctrl_priv *priv = pinctrl_dev_get_drvdata(pctldev);
    const struct virt_pinmux_function *func = &priv->funcs[selector];
    printk("%s\n", __FUNCTION__);

    *groups = func->groups;
    *num_groups = func->ngroups;

    return 0;
}

static int virt_pinmux_set_mux(struct pinctrl_dev *pctldev,
                               unsigned func_selector, unsigned group_selector)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_pinmux_gpio_request_enable(struct pinctrl_dev *pctldev,
                                           struct pinctrl_gpio_range *range,
                                           unsigned pin)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static void virt_pinmux_gpio_disable_free(struct pinctrl_dev *pctldev,
                                          struct pinctrl_gpio_range *range,
                                          unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return;
}

static int virt_pinmux_free(struct pinctrl_dev *pctldev, unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_pinmux_request(struct pinctrl_dev *pctldev, unsigned offset)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static const struct pinmux_ops virt_pinmux_ops = {
    .get_functions_count = virt_pinmux_get_functions_count,
    .get_function_name = virt_pinmux_get_function_name,
    .get_function_groups = virt_pinmux_get_function_groups,
    .set_mux = virt_pinmux_set_mux,
    .gpio_request_enable = virt_pinmux_gpio_request_enable,
    .gpio_disable_free = virt_pinmux_gpio_disable_free,
    .strict = true,
    .free = virt_pinmux_free,
    .request = virt_pinmux_request,
};

static int virt_pinconf_cfg_get(struct pinctrl_dev *pctldev, unsigned pin,
                                unsigned long *config)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_pinconf_cfg_set(struct pinctrl_dev *pctldev, unsigned pin,
                                unsigned long *configs, unsigned num_configs)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int virt_pinconf_group_set(struct pinctrl_dev *pctldev,
                                  unsigned selector, unsigned long *configs,
                                  unsigned num_configs)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static const struct pinconf_ops virt_pinconf_ops = {
    .is_generic = true,
    .pin_config_get = virt_pinconf_cfg_get,
    .pin_config_set = virt_pinconf_cfg_set,
    .pin_config_group_set = virt_pinconf_group_set,
};

static struct pinctrl_desc virt_pinctrl_desc = {
    .name = "virt-pinctrl",
    .pins = virt_pins,
    .npins = ARRAY_SIZE(virt_pins),
    .pctlops = &virt_pinctrl_ops,
    .pmxops = &virt_pinmux_ops,
    .confops = &virt_pinconf_ops,
    .owner = THIS_MODULE,
};

static int virt_pinctrl_probe(struct platform_device *pdev)
{
    struct virt_pinctrl_priv *priv;

    printk("%s\n", __FUNCTION__);
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->groups = virt_pinctrl_group;
    priv->ngroups = ARRAY_SIZE(virt_pinctrl_group);
    priv->funcs = virt_pinmux_function;
    priv->nfuncs = ARRAY_SIZE(virt_pinmux_function);

    priv->pctldev = pinctrl_register(&virt_pinctrl_desc, &pdev->dev, priv);
    if (IS_ERR(priv->pctldev))
        return PTR_ERR(priv->pctldev);

    platform_set_drvdata(pdev, priv);
    return 0;
}

static int virt_pinctrl_remove(struct platform_device *pdev)
{
    struct virt_pinctrl_priv *priv = platform_get_drvdata(pdev);
    pinctrl_unregister(priv->pctldev);
    printk("%s\n", __FUNCTION__);
    return 0;
}

static const struct of_device_id virt_pinctrl_of_match[] = {
    {.compatible = "xm,virt-pinctrl"},
    {},
};

static struct platform_driver virt_pinctrl_driver = {
    .probe = virt_pinctrl_probe,
    .remove = virt_pinctrl_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "virt-pinctrl",
            .of_match_table = virt_pinctrl_of_match,
        },
};

module_platform_driver(virt_pinctrl_driver);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt pinctrl driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");