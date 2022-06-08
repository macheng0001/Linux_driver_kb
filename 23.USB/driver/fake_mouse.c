#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/hid.h>

struct fake_mouse_priv {
    char *transfer_buffer;
    dma_addr_t transfer_dma;
    int buffer_len;
    struct urb *urb;
    struct usb_device *dev;
};

static void fake_mouse_callback(struct urb *urb)
{
    struct fake_mouse_priv *priv = urb->context;
    int ret;

    switch (urb->status) {
    case 0:
        break;
    case -ECONNRESET:
    case -ENOENT:
    case -ESHUTDOWN:
        printk("urb shutting down with status: %d\n", urb->status);
        return;
    default:
        printk("nonzero urb status received: %d\n", urb->status);
        goto resubmit;
    }

    //接收到的鼠标数据包含了按键状态和坐标变化以及滚轮变化
    //其中BYTE0 bit0-左键按下 bit1-右键按下 bit2-中键按下
    if (priv->transfer_buffer[0] & 0x01) {
        printk("left click\n");
    }
    if (priv->transfer_buffer[0] & 0x02) {
        printk("right click\n");
    }
    if (priv->transfer_buffer[0] & 0x04) {
        printk("wheel click\n");
    }

resubmit:
    ret = usb_submit_urb(priv->urb, GFP_ATOMIC);
    if (ret < 0) {
        printk("usb submit urb failed\n");
    }
}

static int fake_mouse_probe(struct usb_interface *interface,
                            const struct usb_device_id *id)
{
    struct fake_mouse_priv *priv;
    int ret;
    struct usb_endpoint_descriptor *endpoint;

    printk("fake mouse probe\n");
    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    usb_set_intfdata(interface, priv);
    priv->dev = interface_to_usbdev(interface);

    endpoint = &interface->cur_altsetting->endpoint[0].desc;
    priv->buffer_len = endpoint->wMaxPacketSize;
    priv->transfer_buffer = usb_alloc_coherent(priv->dev, priv->buffer_len,
                                               GFP_ATOMIC, &priv->transfer_dma);
    if (!priv->transfer_buffer) {
        ret = -ENOMEM;
        goto error_alloc_buffer;
    }

    priv->urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!priv->urb) {
        ret = -ENOMEM;
        goto error_alloc_urb;
    }

    usb_fill_int_urb(priv->urb, priv->dev,
                     usb_rcvintpipe(priv->dev, endpoint->bEndpointAddress),
                     priv->transfer_buffer, priv->buffer_len,
                     fake_mouse_callback, priv, endpoint->bInterval);
    priv->urb->transfer_dma = priv->transfer_dma;
    priv->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    ret = usb_submit_urb(priv->urb, GFP_KERNEL);
    if (ret < 0) {
        goto error_submit_urb;
    }

    return 0;

error_submit_urb:
    usb_free_urb(priv->urb);
error_alloc_urb:
    usb_free_coherent(priv->dev, priv->buffer_len, priv->transfer_buffer,
                      priv->transfer_dma);
error_alloc_buffer:
    kfree(priv);

    return ret;
}

static void fake_mouse_disconnect(struct usb_interface *interface)
{
    struct fake_mouse_priv *priv;
    printk("fake mouse disconnect\n");

    priv = usb_get_intfdata(interface);

    usb_kill_urb(priv->urb);
    usb_free_urb(priv->urb);
    usb_free_coherent(priv->dev, priv->buffer_len, priv->transfer_buffer,
                      priv->transfer_dma);
    kfree(priv);
}

static struct usb_device_id fake_mouse_table[] = {
    {USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
                        USB_INTERFACE_PROTOCOL_MOUSE)},
    {},
};

static struct usb_driver fake_mouse_driver = {
    .name = "fake_mouse",
    .probe = fake_mouse_probe,
    .disconnect = fake_mouse_disconnect,
    .id_table = fake_mouse_table,
};

module_usb_driver(fake_mouse_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("fake usb mouse driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");