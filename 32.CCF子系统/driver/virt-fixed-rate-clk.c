#include <linux/module.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <asm/uaccess.h>

static struct clk *osc;
static int virt_fixed_rate_clk_init(void)
{
    printk("%s\n", __func__);
    osc = clk_register_fixed_rate(NULL, "virt_osc", NULL, 0, 25000000);
    if (IS_ERR(osc)) {
        return PTR_ERR(osc);
    }

    return 0;
}

static void virt_fixed_rate_clk_exit(void)
{
    printk("%s\n", __func__);
    clk_unregister_fixed_rate(osc);
}

module_init(virt_fixed_rate_clk_init);
module_exit(virt_fixed_rate_clk_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt fixed rate clk driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");