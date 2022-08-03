#include <linux/init.h>
#include <linux/module.h>

struct p {
    unsigned int a;
    unsigned int b;
    char *c;
};

static int container_of_test_init(void)
{
    struct p p;
    char c[4];

    p.c = c;
    printk("address:\n\tp:%p\n\ta:%p\n\tb:%p\n\t&c:%p\n\tc:%p\n", &p, &p.a,
           &p.b, &p.c, p.c);
    printk("container_of a:%p,offset:%d\n", container_of(&p.a, struct p, a),
           offsetof(struct p, a));
    printk("container_of b:%p,offset:%d\n", container_of(&p.b, struct p, b),
           offsetof(struct p, b));
    printk("container_of c:%p,offset:%d\n", container_of(&p.c, struct p, c),
           offsetof(struct p, c));

    return 0;
}

static void container_of_test_exit(void)
{
    printk("container_of test exit\n");
}

module_init(container_of_test_init);
module_exit(container_of_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("container_of test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");