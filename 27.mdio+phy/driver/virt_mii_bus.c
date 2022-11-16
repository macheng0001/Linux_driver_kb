#include <linux/init.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

struct virt_mii_bus_priv {
    struct mii_bus *bus;
    struct net_device *net_dev;
    struct phy_device *phy;
};

static int virt_mii_bus_read(struct mii_bus *bus, int phy_id, int phy_reg)
{
    printk("%s phy_id:%d phy_reg:%x\n", __func__, phy_id, phy_reg);
    if (phy_reg == MII_PHYSID1) {
        return 0x1234;
    }
    if (phy_reg == MII_PHYSID2) {
        return 0x5678;
    }
    return 0;
}
static int virt_mii_bus_write(struct mii_bus *bus, int phy_id, int phy_reg,
                              u16 phy_data)
{
    printk("%s\n", __func__);
    return 0;
}

static void virt_adjust_link(struct net_device *ndev)
{
    printk("%s\n", __func__);
    phy_print_status(ndev->phydev);
}

static int virt_mii_bus_probe(struct platform_device *pdev)
{
    struct virt_mii_bus_priv *priv;
    struct mii_bus *bus;
    struct phy_device *phy;
    struct net_device *net_dev;
    int ret, addr;
    struct device *dev = &pdev->dev;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        dev_err(dev, "failed to alloc priv\n");
        return -ENOMEM;
    }

    bus = devm_mdiobus_alloc(dev);
    if (!bus) {
        dev_err(dev, "failed to alloc mii bus\n");
        return -ENOMEM;
    }
    bus->name = "virt mii bus";
    bus->read = virt_mii_bus_read;
    bus->write = virt_mii_bus_write;
    bus->phy_mask = ~1; //只保留地址0的phy
    snprintf(bus->id, MII_BUS_ID_SIZE, "%s", bus->name);
    bus->priv = priv;
    ret = mdiobus_register(bus);
    if (ret < 0)
        return ret;
    priv->bus = bus;
    for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
        phy = mdiobus_get_phy(bus, addr);
        if (phy) {
            dev_err(dev, "phy[%d]: device %s, driver %s\n", phy->mdio.addr,
                    phydev_name(phy), phy->drv ? phy->drv->name : "unknown");
        }
    }

    net_dev = alloc_etherdev(0);
    if (!net_dev) {
        ret = -ENOMEM;
        goto unregister_mdiobus;
    }
    SET_NETDEV_DEV(net_dev, dev);
    priv->net_dev = net_dev;
    priv->phy = phy_connect(net_dev, "virt mii bus:00", &virt_adjust_link,
                            PHY_INTERFACE_MODE_MII);
    if (IS_ERR(priv->phy)) {
        dev_err(dev, "PHY connection failed\n");
        ret = PTR_ERR(priv->phy);
        goto free_netdev;
    }
    phy_start(priv->phy);
    platform_set_drvdata(pdev, priv);

    return 0;

free_netdev:
    free_netdev(priv->net_dev);
unregister_mdiobus:
    mdiobus_unregister(bus);
    return ret;
}

static int virt_mii_bus_remove(struct platform_device *pdev)
{
    struct virt_mii_bus_priv *priv = platform_get_drvdata(pdev);
    struct mii_bus *bus = priv->bus;

    phy_disconnect(priv->phy);
    free_netdev(priv->net_dev);
    mdiobus_unregister(bus);

    return 0;
}

static const struct of_device_id virt_mii_bus_of_match[] = {
    {.compatible = "xm,virt-mii-bus"},
};

static struct platform_driver virt_mii_bus_driver = {
    .probe = virt_mii_bus_probe,
    .remove = virt_mii_bus_remove,
    .driver =
        {
            .name = "virt-mii-bus",
            .of_match_table = virt_mii_bus_of_match,
        },
};

module_platform_driver(virt_mii_bus_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virtual mii bus driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");