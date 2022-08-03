#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>

BLOCKING_NOTIFIER_HEAD(test_notifier_chain);
EXPORT_SYMBOL_GPL(test_notifier_chain);

static int notifier_call(struct notifier_block *nb, unsigned long event,
                         void *v)
{
    printk("%s priority:%d,event:%ld,pointer:%s\n", __func__, nb->priority,
           event, (char *)v);
    return notifier_from_errno(0);
}

struct notifier_block notifier_block1 = {
    .notifier_call = notifier_call,
    .priority = 0,
};

struct notifier_block notifier_block2 = {
    .notifier_call = notifier_call,
    .priority = 1,
};

struct notifier_block notifier_block3 = {
    .notifier_call = notifier_call,
    .priority = 2,
};

static int __init notifier_chain_test_init(void)
{
    int ret;

    ret = blocking_notifier_chain_register(&test_notifier_chain,
                                           &notifier_block1);
    if (ret < 0) {
        printk("blocking notifier register failed\n");
        return ret;
    }

    ret = blocking_notifier_chain_register(&test_notifier_chain,
                                           &notifier_block2);
    if (ret < 0) {
        printk("blocking notifier register failed\n");
        return ret;
    }

    ret = blocking_notifier_chain_register(&test_notifier_chain,
                                           &notifier_block3);
    if (ret < 0) {
        printk("blocking notifier register failed\n");
        return ret;
    }

    return 0;
}

static void __exit notifier_chain_test_exit(void)
{
    blocking_notifier_chain_unregister(&test_notifier_chain, &notifier_block1);
    blocking_notifier_chain_unregister(&test_notifier_chain, &notifier_block2);
    blocking_notifier_chain_unregister(&test_notifier_chain, &notifier_block3);

    return;
}

module_init(notifier_chain_test_init);
module_exit(notifier_chain_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("notifier_chain test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");