#include <linux/iio/events.h>
#include <linux/iio/iio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/kfifo_buf.h>

#define IIO_BUFFER_TRIGGERED

struct fake_iiodev_priv {
    unsigned char current_lux[2]; //当前光照度
    struct timer_list timer;      //用于模拟光照度变化的定时器
    unsigned int freq;            //光照度变化频率
    bool event[2];                //事件使能与否
    unsigned char high_thresh[2]; //阈值上限
    unsigned char low_thresh[2];  //阈值下限
    struct mutex lock;
    struct device *dev;
};

static const struct iio_event_spec fake_iiodev_event_spec[] = {
    {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_RISING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE) | BIT(IIO_EV_INFO_ENABLE),
    },
    {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_FALLING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE) | BIT(IIO_EV_INFO_ENABLE),
    },
};

static const struct iio_chan_spec fake_iiodev_channels[] = {
    {
        .type = IIO_LIGHT,
        .scan_index = 0,
        .indexed = 1,
        .channel = 0,
        .scan_type =
            {
                .sign = 'u',
                .realbits = 8,
                .storagebits = 8,
                .endianness = IIO_LE,
            },
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_INT_TIME),
        .event_spec = fake_iiodev_event_spec,
        .num_event_specs = ARRAY_SIZE(fake_iiodev_event_spec),
    },
    {
        .type = IIO_LIGHT,
        .scan_index = 1,
        .indexed = 1,
        .channel = 1,
        .scan_type =
            {
                .sign = 'u',
                .realbits = 8,
                .storagebits = 8,
                .endianness = IIO_LE,
            },
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        .event_spec = fake_iiodev_event_spec,
        .num_event_specs = ARRAY_SIZE(fake_iiodev_event_spec),
    },
    IIO_CHAN_SOFT_TIMESTAMP(2),
};

static int fake_iiodev_read_raw(struct iio_dev *iio,
                                struct iio_chan_spec const *chan, int *val,
                                int *val2, long mask)
{
    int ret;
    struct fake_iiodev_priv *priv = iio_priv(iio);

    mutex_lock(&priv->lock);
    switch (mask) {
    case IIO_CHAN_INFO_PROCESSED:
        *val2 = 0;
        *val = priv->current_lux[chan->scan_index];
        ret = IIO_VAL_INT;
        break;
    case IIO_CHAN_INFO_INT_TIME:
        *val2 = 0;
        *val = priv->freq;
        ret = IIO_VAL_INT;
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&priv->lock);

    return ret;
}

static int fake_iiodev_write_raw(struct iio_dev *iio,
                                 struct iio_chan_spec const *chan, int val,
                                 int val2, long mask)
{
    struct fake_iiodev_priv *priv = iio_priv(iio);
    int ret;

    if (mask != IIO_CHAN_INFO_INT_TIME)
        return -EINVAL;

    if (val > 5 || val < 1)
        ret = -EINVAL;

    mutex_lock(&priv->lock);
    priv->freq = val;
    mod_timer(&priv->timer, jiffies + priv->freq * HZ);
    mutex_unlock(&priv->lock);

    return 0;
}

static int fake_iiodev_write_raw_get_fmt(struct iio_dev *indio_dev,
                                         struct iio_chan_spec const *chan,
                                         long mask)
{
    if (mask != IIO_CHAN_INFO_INT_TIME)
        return -EINVAL;

    return IIO_VAL_INT;
}

static int fake_iiodev_read_event_config(struct iio_dev *iio,
                                         const struct iio_chan_spec *chan,
                                         enum iio_event_type type,
                                         enum iio_event_direction dir)
{
    struct fake_iiodev_priv *priv = iio_priv(iio);

    return priv->event[chan->scan_index];
}

static int fake_iiodev_write_event_config(struct iio_dev *iio,
                                          const struct iio_chan_spec *chan,
                                          enum iio_event_type type,
                                          enum iio_event_direction dir,
                                          int state)
{
    struct fake_iiodev_priv *priv = iio_priv(iio);

    mutex_lock(&priv->lock);
    priv->event[chan->scan_index] = state;
    mutex_unlock(&priv->lock);

    return 0;
}

static int fake_iiodev_read_event_value(struct iio_dev *iio,
                                        const struct iio_chan_spec *chan,
                                        enum iio_event_type type,
                                        enum iio_event_direction dir,
                                        enum iio_event_info info, int *val,
                                        int *val2)
{
    int ret = IIO_VAL_INT;
    struct fake_iiodev_priv *priv = iio_priv(iio);

    mutex_lock(&priv->lock);
    switch (dir) {
    case IIO_EV_DIR_RISING:
        *val2 = 0;
        *val = priv->high_thresh[chan->scan_index];
        break;
    case IIO_EV_DIR_FALLING:
        *val2 = 0;
        *val = priv->low_thresh[chan->scan_index];
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&priv->lock);

    return ret;
}

static int fake_iiodev_write_event_value(struct iio_dev *iio,
                                         const struct iio_chan_spec *chan,
                                         enum iio_event_type type,
                                         enum iio_event_direction dir,
                                         enum iio_event_info info, int val,
                                         int val2)
{
    int ret = 0;
    struct fake_iiodev_priv *priv = iio_priv(iio);

    mutex_lock(&priv->lock);
    switch (dir) {
    case IIO_EV_DIR_RISING:
        priv->high_thresh[chan->scan_index] = val;
        break;
    case IIO_EV_DIR_FALLING:
        priv->low_thresh[chan->scan_index] = val;
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&priv->lock);

    return ret;
}

static const struct iio_info fake_iiodev_info = {
    .driver_module = THIS_MODULE,
    .read_raw = fake_iiodev_read_raw,
    .write_raw = fake_iiodev_write_raw,
    .write_raw_get_fmt = fake_iiodev_write_raw_get_fmt,
    .read_event_config = fake_iiodev_read_event_config,
    .write_event_config = fake_iiodev_write_event_config,
    .read_event_value = fake_iiodev_read_event_value,
    .write_event_value = fake_iiodev_write_event_value,
};

static void lux_auto_change(unsigned long arg)
{
    struct iio_dev *iio = (struct iio_dev *)arg;
    struct fake_iiodev_priv *priv = iio_priv(iio);
    int i;
    unsigned char data[ALIGN(2, sizeof(s64)) + sizeof(s64)] = {0};

    mutex_lock(&priv->lock);
    priv->current_lux[0]++;
    priv->current_lux[1] = priv->current_lux[0] / 2;
    printk("lux_auto_change:%d,%d\n", priv->current_lux[0],
           priv->current_lux[1]);

    for (i = 0; i < 2; i++) {
        if (priv->event[i]) {
            if (priv->current_lux[i] == priv->high_thresh[i])
                iio_push_event(iio,
                               IIO_UNMOD_EVENT_CODE(IIO_LIGHT, i,
                                                    IIO_EV_TYPE_THRESH,
                                                    IIO_EV_DIR_RISING),
                               iio_get_time_ns(iio));
            if (priv->current_lux[i] == priv->low_thresh[i])
                iio_push_event(iio,
                               IIO_UNMOD_EVENT_CODE(IIO_LIGHT, i,
                                                    IIO_EV_TYPE_THRESH,
                                                    IIO_EV_DIR_FALLING),
                               iio_get_time_ns(iio));
        }
    }
#ifndef IIO_BUFFER_TRIGGERED
    for (i = 0; i < 2; i++) {
        data[i] = priv->current_lux[i];
    }
    iio_push_to_buffers_with_timestamp(iio, data, iio_get_time_ns(iio));
#endif

    mod_timer(&priv->timer, jiffies + priv->freq * HZ);
    mutex_unlock(&priv->lock);

    return;
}

static irqreturn_t fake_iiodev_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = (struct iio_poll_func *)p;
    struct iio_dev *iio = pf->indio_dev;
    struct fake_iiodev_priv *priv = iio_priv(iio);
    unsigned char data[ALIGN(2, sizeof(s64)) + sizeof(s64)] = {0};
    int i;

    mutex_lock(&priv->lock);
    for (i = 0; i < 2; i++) {
        data[i] = priv->current_lux[i];
    }
    mutex_unlock(&priv->lock);
    iio_push_to_buffers_with_timestamp(iio, data, iio_get_time_ns(iio));

    iio_trigger_notify_done(iio->trig);

    return IRQ_HANDLED;
}

static int fake_iiodev_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
    struct iio_dev *iio;
    struct fake_iiodev_priv *priv;
    struct device *dev = &client->dev;
    struct iio_buffer *buffer;
    int ret;

    iio = devm_iio_device_alloc(dev, sizeof(*priv));
    if (!iio)
        return -ENOMEM;

    priv = iio_priv(iio);
    priv->dev = dev;

    mutex_init(&priv->lock);
    i2c_set_clientdata(client, iio);

    iio->name = client->name;
    iio->channels = fake_iiodev_channels;
    iio->num_channels = ARRAY_SIZE(fake_iiodev_channels);
    iio->dev.parent = dev;
    iio->modes = INDIO_DIRECT_MODE;
    iio->info = &fake_iiodev_info;

#ifdef IIO_BUFFER_TRIGGERED
    ret = devm_iio_triggered_buffer_setup(dev, iio, NULL,
                                          fake_iiodev_trigger_handler, NULL);
    if (ret)
        return ret;
#else
    iio->modes |= INDIO_BUFFER_SOFTWARE;
    buffer = devm_iio_kfifo_allocate(dev);
    if (!buffer)
        return -ENOMEM;

    iio_device_attach_buffer(iio, buffer);
#endif

    ret = devm_iio_device_register(dev, iio);
    if (ret) {
        dev_err(dev, "failed to register IIO device\n");
        return ret;
    }

    setup_timer(&priv->timer, lux_auto_change, (unsigned long)iio);
    priv->freq = 1; //默认1Hz
    mod_timer(&priv->timer, jiffies + priv->freq * HZ);

    return 0;
}

static int fake_iiodev_remove(struct i2c_client *client)
{
    struct iio_dev *iio = i2c_get_clientdata(client);
    struct fake_iiodev_priv *priv = iio_priv(iio);

    del_timer_sync(&priv->timer);

    return 0;
}

static const struct i2c_device_id fake_iiodev_id[] = {{"fake_iiodev", 0}, {}};
MODULE_DEVICE_TABLE(i2c, fake_iiodev_id);

static const struct of_device_id fake_iiodev_of_match[] = {
    {.compatible = "xm,fake_iiodev"}, {}};

static struct i2c_driver fake_iiodev_driver = {
    .probe = fake_iiodev_probe,
    .remove = fake_iiodev_remove,
    .id_table = fake_iiodev_id,
    .driver =
        {
            .name = "fake-iiodev",
            .of_match_table = of_match_ptr(fake_iiodev_of_match),
        },
};

module_i2c_driver(fake_iiodev_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("fake iio device driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");