#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>

int netdevice_notifier(struct notifier_block *this, unsigned long event,
                       void *ptr)
{
    struct net_device *dev = netdev_notifier_info_to_dev(ptr);

    switch (event) {
    case NETDEV_UP:
        printk("[%s] is up\n", dev->name);
        break;
    case NETDEV_DOWN:
        printk("[%s] is down\n", dev->name);
        break;
    default:
        break;
    }

    return NOTIFY_DONE;
}

struct notifier_block netdev_notifier_block = {
    .notifier_call = netdevice_notifier,
};

static int __init netdev_chain_test_init(void)
{
    int ret;

    ret = register_netdevice_notifier(&netdev_notifier_block);
    if (ret < 0) {
        printk("netdevice notifier register failed\n");
        return ret;
    }

    return 0;
}

static void __exit netdev_chain_test_exit(void)
{
    unregister_netdevice_notifier(&netdev_notifier_block);

    return;
}

module_init(netdev_chain_test_init);
module_exit(netdev_chain_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("netdev_chain test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");