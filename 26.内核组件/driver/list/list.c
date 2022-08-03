#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>

LIST_HEAD(list_head);

struct data {
    unsigned int index;
    struct list_head list;
};
#define DATA_CNT 3
static int list_test_init(void)
{
    int i;
    struct data data[DATA_CNT], *p, *n;

    for (i = 0; i < DATA_CNT; i++) {
        data[i].index = i + 1;
    }

    printk("list_empty:%d\n", list_empty(&list_head));

    list_add(&data[0].list, &list_head);
    list_add_tail(&data[1].list, &list_head);
    list_add_tail(&data[2].list, &list_head);
    list_for_each_entry(p, &list_head, list)
    {
        printk("after add index:%d\n", p->index);
    }

    list_del(&data[0].list);
    list_for_each_entry(p, &list_head, list)
    {
        printk("after del index:%d\n", p->index);
    }

    list_for_each_entry_safe(p, n, &list_head, list)
    {
        list_del(&p->list);
    }

    printk("list_empty:%d\n", list_empty(&list_head));

    return 0;
}

static void list_test_exit(void)
{
    printk("list test exit\n");
}

module_init(list_test_init);
module_exit(list_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("list test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");