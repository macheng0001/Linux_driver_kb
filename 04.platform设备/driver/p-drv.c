#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>

static int platform_test_probe(struct platform_device *pdev)
{
    int ret;
    unsigned int array[4], data, i;
    const char *str;
    struct device_node *np;

    printk("dts test init\n");
    np = pdev->dev.of_node;

    ret = of_property_read_u32_array(np, "dts-test-u32-array", array, 4);
    if (ret) {
        printk("failed to read array property:%d\n", ret);
        return ret;
    } else {
        for (i = 0; i < 4; i++) {
            printk("dts-test-u32-array[%d] = %x\n", i, array[i]);
        }
    }

    ret = of_property_read_u32(np, "dts-test-u32", &data);
    if (ret) {
        printk("failed to read u32 property:%d\n", ret);
        return ret;
    } else {
        printk("dts-test-u32:%x\n", data);
    }

    ret = of_property_read_string(np, "dts-test-string", &str);
    if (ret) {
        printk("failed to read string property:%d\n", ret);
        return ret;
    } else {
        printk("dts-test-string:%s\n", str);
    }

    if (of_property_read_bool(np, "dts-test-bool")) {
        printk("succeeded to read bool property\n");
    } else {
        printk("failed to read bool property\n");
        return -EINVAL;
    }

    return 0;
}

static int platform_test_remove(struct platform_device *dev)
{
    return 0;
}

static const struct of_device_id platform_test_of_match[] = {
    {.compatible = "dts-test"}, {/* Sentinel */}};

static struct platform_driver platform_test_driver = {
    .probe = platform_test_probe,
    .remove = platform_test_remove,
    .driver =
        {
            .name = "platform-test",
            .of_match_table = platform_test_of_match,
        },
};

module_platform_driver(platform_test_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("platform test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
