#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>

extern struct blocking_notifier_head test_notifier_chain;

static int __init notifier_chain_test_init(void)
{
    int ret = blocking_notifier_call_chain(&test_notifier_chain, 123,
                                           "notifier_chain test");
    return notifier_to_errno(ret);
}

static void __exit notifier_chain_test_exit(void)
{
    return;
}

module_init(notifier_chain_test_init);
module_exit(notifier_chain_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("notifier_chain test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");