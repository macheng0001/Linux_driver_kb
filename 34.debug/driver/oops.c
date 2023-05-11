#include <linux/module.h>
#include <linux/slab.h>

static unsigned int mode = 0;
module_param(mode, uint, 0);
MODULE_PARM_DESC(mode, "Oops test mode:\
    \n\t0:access null pointer access,\
    \n\t1:out-of-bounds on stack,\
    \n\t2:out-of-bounds to right,\
    \n\t3:out-of-bounds to left,\
    \n\t4:free already freed,\
    \n\t5:use-after-free\
    \n\t6:invalid-free\
    \n\tdefault=0");

static int oops_test_init(void)
{
    char *ptr;
    char a[3];
    size_t size = 32;

    switch (mode) {
    case 0:
        //访问空指针
        *(int *)0 = 0;
        break;
    case 1:
        //栈越界访问
        memset(a, 0, 8);
        break;
    case 2:
        //向右越界访问
        ptr = kmalloc(size, GFP_KERNEL);
        memset(ptr, 0x55, 80);
        kfree(ptr);
        break;
    case 3:
        //向左越界访问
        ptr = kmalloc(size, GFP_KERNEL);
        ptr[-1] = 0x55;
        kfree(ptr);
        break;
    case 4:
        //多次释放
        ptr = kmalloc(size, GFP_KERNEL);
        kfree(ptr);
        kfree(ptr);
        break;
    case 5:
        //释放后访问
        ptr = kmalloc(size, GFP_KERNEL);
        kfree(ptr);
        ptr[0] = 0;
        break;
    case 6:
        //无效释放
        ptr = kmalloc(size, GFP_KERNEL);
        kfree(ptr + 1);
        break;
    default:
        break;
    };

    return 0;
}

static void oops_test_exit(void)
{
}

module_init(oops_test_init);
module_exit(oops_test_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("Oops test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");