#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>

static int dts_test_init(void)
{
  int ret;
  unsigned int array[4], data, i;
  const char *str;
  struct device_node *np;

  printk("dts test init\n");
  np = of_find_node_by_name(NULL, "dts_test");
  if(!np) {
    printk("failed to find dts_test node\n");
    return -ENODEV;
  }

  ret = of_property_read_u32_array(np, "dts-test-u32-array", array, 4);
  if(ret) {
    printk("failed to read array property:%d\n", ret);
    return ret;
  } else {
    for(i=0; i < 4; i++) {
      printk("dts-test-u32-array[%d] = %x\n", i, array[i]);
    }
  }

  ret = of_property_read_u32(np, "dts-test-u32", &data);
  if(ret) {
    printk("failed to read u32 property:%d\n", ret);
    return ret;
  } else {
    printk("dts-test-u32:%x\n", data);
  }

  ret = of_property_read_string(np, "dts-test-string", &str);
  if(ret) {
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

  return  0;
}

static void dts_test_exit(void)
{
  printk( "dts test exit\n");
}

module_init(dts_test_init);
module_exit(dts_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("dts test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
