#include <linux/phy.h>

static int virt_phy_config_init(struct phy_device *phydev)
{
    printk("%s\n", __func__);
    return 0;
}

static int virt_phy_config_aneg(struct phy_device *phydev)
{
    printk("%s\n", __func__);
    return 0;
}

static int virt_phy_read_status(struct phy_device *phydev)
{
    printk("%s\n", __func__);
    phydev->speed = SPEED_100;
    phydev->duplex = DUPLEX_FULL;
    return 0;
}

static int virt_phy_probe(struct phy_device *phydev)
{
    printk("%s\n", __func__);
    return 0;
}

static int virt_phy_aneg_done(struct phy_device *phydev)
{
    printk("%s\n", __func__);
    return 1;
}

static struct phy_driver virt_phy_driver[] = {{
    .phy_id = 0x12345678,
    .name = "virt_phy",
    .phy_id_mask = 0xffffffff,
    .features = PHY_BASIC_FEATURES,
    .flags = PHY_HAS_INTERRUPT,
    .config_init = virt_phy_config_init,
    .config_aneg = virt_phy_config_aneg,
    .read_status = virt_phy_read_status,
    .probe = virt_phy_probe,
    .aneg_done = virt_phy_aneg_done,
}};

module_phy_driver(virt_phy_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virtual phy driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");