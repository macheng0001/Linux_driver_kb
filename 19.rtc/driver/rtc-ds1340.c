#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/bcd.h>

#define REG_DS1340_SECS 0x00
#define REG_DS1340_MIN 0x01
#define REG_DS1340_HOUR 0x02
#define REG_DS1340_WDAY 0x03
#define REG_DS1340_MDAY 0x04
#define REG_DS1340_MONTH 0x05
#define REG_DS1340_YEAR 0x06
#define REG_DS1340_CONTROL 0x07
#define REG_DS1340_MAX 0x08

struct ds1340_priv {
    struct i2c_client *client;
    struct regmap *regmap;
    struct rtc_device *rtc;
};

static bool ds1340_is_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg <= REG_DS1340_YEAR;
}

static const struct regmap_config ds1340_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = REG_DS1340_MAX,
    .volatile_reg = ds1340_is_volatile_reg,
    .val_format_endian = REGMAP_ENDIAN_BIG,
    .cache_type = REGCACHE_RBTREE,
};

static int ds1340_get_time(struct device *dev, struct rtc_time *t)
{
    /* 1.分析rtc_read_time()可知dev指向rtc_device->dev.parent,也就是注册rtc设备时提供的父设备，
     *   rtc操作函数集的其他回调的参数dev均是如此。
     * 2.probe中已经通过i2c_set_clientdata()设置私有数据，所以可用dev_get_drvdata()获取私有数据。
     * 3.注意注册rtc设备时devm_regmap_init_i2c()内部调用了__rtc_read_alarm()执行了读取时间/读取
     *   闹钟的回调，所以如果这两个回调使用dev_get_drvdata()获取私有数据，那么设置私有数据必须在注册
     *   rtc设备前执行，否则会因为得到的是空指针导致程序崩溃。
     */
    struct ds1340_priv *priv = dev_get_drvdata(dev);
    int ret, tmp;
    unsigned char buf[7];

    ret = regmap_bulk_read(priv->regmap, REG_DS1340_SECS, buf, sizeof(buf));
    if (ret < 0) {
        return ret;
    }

    t->tm_sec = bcd2bin(buf[REG_DS1340_SECS] & 0x7f);
    t->tm_min = bcd2bin(buf[REG_DS1340_MIN] & 0x7f);
    tmp = buf[REG_DS1340_HOUR] & 0x3f;
    t->tm_hour = bcd2bin(tmp);
    t->tm_wday =
        bcd2bin(buf[REG_DS1340_WDAY] & 0x07) - 1; // ds1340星期范围为[1,7]
    t->tm_mday = bcd2bin(buf[REG_DS1340_MDAY] & 0x3f);
    tmp = buf[REG_DS1340_MONTH] & 0x1f;
    t->tm_mon = bcd2bin(tmp) - 1; // ds1340月份范围为[1,12]
    //时间应为20XX年
    t->tm_year = bcd2bin(buf[REG_DS1340_YEAR]) + 100;

    return 0;
}

static int ds1340_set_time(struct device *dev, struct rtc_time *t)
{
    struct ds1340_priv *priv = dev_get_drvdata(dev);
    int ret, tmp;
    unsigned char buf[7];

    buf[REG_DS1340_SECS] = bin2bcd(t->tm_sec);
    buf[REG_DS1340_MIN] = bin2bcd(t->tm_min);
    buf[REG_DS1340_HOUR] = bin2bcd(t->tm_hour);
    buf[REG_DS1340_WDAY] = bin2bcd(t->tm_wday + 1);
    buf[REG_DS1340_MDAY] = bin2bcd(t->tm_mday);
    buf[REG_DS1340_MONTH] = bin2bcd(t->tm_mon + 1);
    //时间应为20XX年
    tmp = t->tm_year - 100;
    buf[REG_DS1340_YEAR] = bin2bcd(tmp);

    ret = regmap_bulk_write(priv->regmap, REG_DS1340_SECS, buf, sizeof(buf));
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static const struct rtc_class_ops ds1340_rtc_ops = {
    .read_time = ds1340_get_time,
    .set_time = ds1340_set_time,
};

static int ds1340_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    struct ds1340_priv *priv;

    priv = devm_kzalloc(&client->dev, sizeof(struct ds1340_priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&client->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    priv->client = client;
    i2c_set_clientdata(client, priv);

    priv->regmap = devm_regmap_init_i2c(client, &ds1340_regmap_config);
    if (IS_ERR(priv->regmap))
        return PTR_ERR(priv->regmap);

    priv->rtc = devm_rtc_device_register(&client->dev, client->name,
                                         &ds1340_rtc_ops, THIS_MODULE);
    if (IS_ERR(priv->rtc)) {
        return PTR_ERR(priv->rtc);
    }

    return 0;
}

static int ds1340_remove(struct i2c_client *client)
{
    struct ds1340_priv *priv;

    priv = i2c_get_clientdata(client);

    return 0;
}

static const struct i2c_device_id ds1340_ids[] = {{"ds1340", 0}, {}};
MODULE_DEVICE_TABLE(i2c, ds1340_ids);

static struct i2c_driver ds1340_driver = {
    .driver =
        {
            .name = "rtc-ds1340",
        },
    .probe = ds1340_probe,
    .remove = ds1340_remove,
    .id_table = ds1340_ids,
};

module_i2c_driver(ds1340_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("ds1340 rtc driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
