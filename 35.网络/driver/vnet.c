#include <linux/module.h>
#include <linux/netdevice.h>

struct vnet_priv {
};

static struct net_device *vnet_dev;

static int vnet_open(struct net_device *dev)
{
    netif_start_queue(dev);
    return 0;
}

static int vnet_release(struct net_device *dev)
{
    netif_stop_queue(dev);
    return 0;
}

int vnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    netif_rx(skb);
    return NETDEV_TX_OK;
}

static const struct net_device_ops vnet_ops = {
    .ndo_open = vnet_open,
    .ndo_stop = vnet_release,
    .ndo_start_xmit = vnet_start_xmit,
};

static void vnet_setup(struct net_device *dev)
{
    ether_setup(dev);
    dev->flags |= IFF_NOARP;
    dev->netdev_ops = &vnet_ops;
}

static int vnet_init(void)
{
    int ret;

    vnet_dev = alloc_netdev(sizeof(struct vnet_priv), "vnet", NET_NAME_UNKNOWN,
                            vnet_setup);
    if (!vnet_dev)
        return -ENOMEM;

    ret = register_netdev(vnet_dev);
    if (ret)
        goto free_netdev;

    return 0;

free_netdev:
    free_netdev(vnet_dev);
    return ret;
}

static void vnet_exit(void)
{
    unregister_netdev(vnet_dev);
    free_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt net device driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");