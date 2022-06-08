/* #define VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/utsname.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/usb/gadget.h>
#include <asm/uaccess.h>
#include <linux/usb/composite.h>

#define BUFLEN 4096

struct f_g_char_priv {
    struct usb_function function;

    struct usb_ep *in_ep;
    struct usb_ep *out_ep;
    struct usb_request *in_req, *out_req;

    wait_queue_head_t r_wq;
    bool r_able;
    wait_queue_head_t w_wq;
    bool w_able;

    struct completion gdt_completion;
    char data[BUFLEN];
};

static inline struct f_g_char_priv *func_to_priv(struct usb_function *f)
{
    return container_of(f, struct f_g_char_priv, function);
}

//接口描述符
static struct usb_interface_descriptor g_char_intf = {
    .bLength = sizeof g_char_intf,
    .bDescriptorType = USB_DT_INTERFACE,

    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
    /* .iInterface = DYNAMIC */
};

//全速设备端点描述符
static struct usb_endpoint_descriptor fs_in_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,

    .bEndpointAddress = USB_DIR_IN,
    .bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_out_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,

    .bEndpointAddress = USB_DIR_OUT,
    .bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_g_char_descs[] = {
    (struct usb_descriptor_header *)&g_char_intf,
    (struct usb_descriptor_header *)&fs_out_desc,
    (struct usb_descriptor_header *)&fs_in_desc,
    NULL,
};

//高速设备端点描述符
static struct usb_endpoint_descriptor hs_in_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,

    .bmAttributes = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_out_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,

    .bmAttributes = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_descriptor_header *hs_g_char_descs[] = {
    (struct usb_descriptor_header *)&g_char_intf,
    (struct usb_descriptor_header *)&hs_in_desc,
    (struct usb_descriptor_header *)&hs_out_desc,
    NULL,
};

static struct usb_string strings_g_char[] = {
    [0].s = "gadget char data", {} /* end of list */
};

static struct usb_gadget_strings stringtab_g_char = {
    .language = 0x0409, /* en-us */
    .strings = strings_g_char,
};

static struct usb_gadget_strings *g_char_strings[] = {
    &stringtab_g_char,
    NULL,
};

#define DRIVER_VENDOR_NUM 0xfff0
#define DRIVER_PRODUCT_NUM 0xfff0

static struct usb_device_descriptor device_desc = {
    .bLength = sizeof device_desc,
    .bDescriptorType = USB_DT_DEVICE,

    .bcdUSB = cpu_to_le16(0x0200),
    .bDeviceClass = USB_CLASS_VENDOR_SPEC,

    .idVendor = cpu_to_le16(DRIVER_VENDOR_NUM),
    .idProduct = cpu_to_le16(DRIVER_PRODUCT_NUM),
    .bNumConfigurations = 1,
};

static struct usb_string strings_dev[] = {
    [USB_GADGET_MANUFACTURER_IDX].s = "",
    [USB_GADGET_PRODUCT_IDX].s = "Gadget char",
    [USB_GADGET_SERIAL_IDX].s = "0123456789.0123456789.0123456789",
    {} /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
    .language = 0x0409, /* en-us */
    .strings = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
    &stringtab_dev,
    NULL,
};

static struct usb_request *alloc_ep_req(struct usb_ep *ep, size_t len)
{
    struct usb_request *req;

    req = usb_ep_alloc_request(ep, GFP_ATOMIC);
    if (req) {
        req->length =
            usb_endpoint_dir_out(ep->desc) ? usb_ep_align(ep, len) : len;
        req->buf = kmalloc(req->length, GFP_ATOMIC);
        if (!req->buf) {
            usb_ep_free_request(ep, req);
            req = NULL;
        }
    }
    return req;
}

void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
    kfree(req->buf);
    usb_ep_free_request(ep, req);
}

static void disable_ep(struct usb_composite_dev *cdev, struct usb_ep *ep)
{
    int value;

    if (ep->driver_data) {
        value = usb_ep_disable(ep);
        ep->driver_data = NULL;
    }
}

void disable_endpoints(struct usb_composite_dev *cdev, struct usb_ep *in,
                       struct usb_ep *out)
{
    disable_ep(cdev, in);
    disable_ep(cdev, out);
}

/*-------------------------------------------------------------------------*/

static int __init g_char_f_bind(struct usb_configuration *c,
                                struct usb_function *f)
{
    struct usb_composite_dev *cdev = c->cdev;
    struct f_g_char_priv *priv = func_to_priv(f);
    int id;

    id = usb_interface_id(c, f);
    if (id < 0)
        return id;
    g_char_intf.bInterfaceNumber = id;

    priv->in_ep = usb_ep_autoconfig(cdev->gadget, &fs_in_desc);
    if (!priv->in_ep) {
    autoconf_fail:
        return -ENODEV;
    }
    priv->in_ep->driver_data = cdev; /* claim */

    priv->out_ep = usb_ep_autoconfig(cdev->gadget, &fs_out_desc);
    if (!priv->out_ep)
        goto autoconf_fail;
    priv->out_ep->driver_data = cdev; /* claim */

    /* support high speed hardware */
    if (gadget_is_dualspeed(c->cdev->gadget)) {
        hs_in_desc.bEndpointAddress = fs_in_desc.bEndpointAddress;
        hs_out_desc.bEndpointAddress = fs_out_desc.bEndpointAddress;
        f->hs_descriptors = hs_g_char_descs;
    }

    return 0;
}

static void disable_g_char(struct f_g_char_priv *priv)
{
    struct usb_composite_dev *cdev;

    cdev = priv->function.config->cdev;
    disable_endpoints(cdev, priv->in_ep, priv->out_ep);
}

static void g_char_complete(struct usb_ep *ep, struct usb_request *req)
{
    int status = req->status;
    struct f_g_char_priv *priv = ep->driver_data;

    switch (status) {
    case 0:             /* normal completion? */
    case -ECONNABORTED: /* hardware forced ep reset */
    case -ECONNRESET:   /* request dequeued */
    case -ESHUTDOWN:    /* disconnect from host */
    case -EOVERFLOW:
    case -EREMOTEIO:
    default:
        break;
    }
    if (ep == priv->out_ep) {
        priv->r_able = 1;
        wake_up_interruptible(&priv->r_wq);

    } else {
        priv->w_able = 1;
        wake_up_interruptible(&priv->w_wq);
    }
}

static int enable_endpoint(struct usb_composite_dev *cdev,
                           struct f_g_char_priv *priv, struct usb_ep *ep)
{
    int result = 0;

    result = config_ep_by_speed(cdev->gadget, &priv->function, ep);
    if (result)
        goto out;

    result = usb_ep_enable(ep);
    if (result < 0)
        goto out;

    ep->driver_data = priv;
out:
    return result;
}

static int enable_g_char(struct usb_composite_dev *cdev,
                         struct f_g_char_priv *priv)
{
    int ret = 0;

    ret = enable_endpoint(cdev, priv, priv->in_ep);
    if (ret < 0)
        goto err_enable_in;

    ret = enable_endpoint(cdev, priv, priv->out_ep);
    if (ret < 0)
        goto err_enable_out;

    priv->out_req = alloc_ep_req(priv->in_ep, BUFLEN);
    if (!priv->out_req) {
        goto err_alloc_in;
    }
    priv->out_req->complete = g_char_complete;

    priv->in_req = alloc_ep_req(priv->out_ep, BUFLEN);
    if (!priv->in_req) {
        goto err_alloc_out;
    }
    priv->in_req->complete = g_char_complete;

    return ret;
err_alloc_out:
    free_ep_req(priv->in_ep, priv->out_req);
err_alloc_in:
    usb_ep_disable(priv->out_ep);
err_enable_out:
    usb_ep_disable(priv->in_ep);
err_enable_in:
    return ret;
}

static int g_char_f_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
    struct f_g_char_priv *priv = func_to_priv(f);
    struct usb_composite_dev *cdev = f->config->cdev;

    /* we know alt is zero */
    if (priv->in_ep->driver_data)
        disable_g_char(priv);
    return enable_g_char(cdev, priv);
}

static void g_char_f_disable(struct usb_function *f)
{
    struct f_g_char_priv *priv = func_to_priv(f);

    disable_g_char(priv);
}

static int g_char_open(struct inode *inode, struct file *file)
{
    struct miscdevice *miscdev = file->private_data;
    struct f_g_char_priv *priv = dev_get_drvdata(miscdev->this_device);

    file->private_data = priv;

    return 0;
}

static ssize_t g_char_read(struct file *file, char __user *buf, size_t count,
                           loff_t *f_pos)
{

    int ret;
    struct f_g_char_priv *priv = file->private_data;
    struct usb_request *req = priv->in_req;

    priv->r_able = 0;
    ret = usb_ep_queue(priv->out_ep, req, GFP_ATOMIC);
    if (ret) {
        goto fail;
    }

    if (wait_event_interruptible(priv->r_wq, priv->r_able)) {
        usb_ep_dequeue(priv->out_ep, req);
        return -ERESTARTSYS;
    }

    count = (count < req->actual) ? count : req->actual;
    if (copy_to_user(buf, req->buf, count)) {
        ret = -EFAULT;
        goto fail;
    }

    return count;

fail:
    return ret;
}

static ssize_t g_char_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *f_pos)
{
    int ret;
    struct f_g_char_priv *priv = file->private_data;
    struct usb_request *req = priv->out_req;

    priv->w_able = 0;
    req->length = (count < BUFLEN) ? count : BUFLEN;

    if (copy_from_user(req->buf, buf, req->length)) {
        ret = -EFAULT;
        return ret;
    }

    ret = usb_ep_queue(priv->in_ep, req, GFP_ATOMIC);
    if (ret) {
        return ret;
    }

    if (wait_event_interruptible(priv->w_wq, priv->w_able)) {
        usb_ep_dequeue(priv->in_ep, req);
        return -ERESTARTSYS;
    }

    return req->actual;
}

static int g_char_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations g_char_fops = {
    .owner = THIS_MODULE,
    .open = g_char_open,
    .read = g_char_read,
    .write = g_char_write,
    .release = g_char_release,
};

static struct miscdevice g_char_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "g_char",
    .fops = &g_char_fops,
};

static void g_char_f_unbind(struct usb_configuration *c, struct usb_function *f)
{
    misc_deregister(&g_char_misc);
    kfree(func_to_priv(f));
}

static int g_char_bind_config(struct usb_configuration *c)
{
    int ret;
    struct f_g_char_priv *priv;

    priv = kzalloc(sizeof *priv, GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    init_completion(&priv->gdt_completion);

    priv->function.name = "g_char";
    priv->function.fs_descriptors = fs_g_char_descs;
    priv->function.bind = g_char_f_bind;
    priv->function.unbind = g_char_f_unbind;
    priv->function.set_alt = g_char_f_set_alt;
    priv->function.disable = g_char_f_disable;

    ret = usb_add_function(c, &priv->function);
    if (ret)
        goto err_add_function;

    ret = misc_register(&g_char_misc);
    if (ret) {
        goto err_register;
    }
    dev_set_drvdata(g_char_misc.this_device, priv);
    init_waitqueue_head(&priv->r_wq);
    init_waitqueue_head(&priv->w_wq);

    return 0;

err_register:
err_add_function:
    kfree(priv);
    return ret;
}

static int g_char_setup(struct usb_configuration *c,
                        const struct usb_ctrlrequest *ctrl)
{
    return 0;
}

static struct usb_configuration g_char_configuration = {
    .label = "g_char",
    .strings = g_char_strings,
    .setup = g_char_setup,
    .bConfigurationValue = 3,
    .bmAttributes = USB_CONFIG_ATT_SELFPOWER,
    /* .iConfiguration = DYNAMIC */
};

static int __init g_char_bind(struct usb_composite_dev *cdev)
{
    struct usb_gadget *gadget = cdev->gadget;
    int ret = 0;

    ret = usb_string_ids_tab(cdev, strings_dev);
    if (ret < 0)
        return ret;

    device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
    device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
    device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

    return usb_add_config(cdev, &g_char_configuration, g_char_bind_config);
}

static int g_char_unbind(struct usb_composite_dev *cdev)
{
    return 0;
}

static struct usb_composite_driver g_char_driver = {
    .name = "g_char",
    .dev = &device_desc,
    .bind = g_char_bind,
    .strings = dev_strings,
    .max_speed = USB_SPEED_SUPER,
    .unbind = g_char_unbind,
};
module_usb_composite_driver(g_char_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("gadget char driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");