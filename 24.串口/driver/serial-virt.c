#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/tty_flip.h>

#define PORT_VIRT 128

struct virt_uart_port {
    struct uart_port port;
};

static unsigned int virt_uart_tx_empty(struct uart_port *port)
{
    struct circ_buf *xmit = &port->state->xmit;
    dev_info(port->dev, "%s\n", __FUNCTION__);

    return uart_circ_empty(xmit);
}

static void virt_uart_stop_tx(struct uart_port *port)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);
}

static void virt_uart_start_tx(struct uart_port *port)
{
    struct circ_buf *xmit = &port->state->xmit;
    int count = 0;
    dev_info(port->dev, "%s\n", __FUNCTION__);

    if (port->x_char) {
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        return;
    }

    count = port->fifosize;
    do {
        uart_insert_char(port, 0, 0, xmit->buf[xmit->tail], TTY_NORMAL);
        // tty_insert_flip_char(&port->state->port, xmit->buf[xmit->tail],
        //                    TTY_NORMAL);

        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
        if (uart_circ_empty(xmit))
            break;
    } while (--count > 0);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
        uart_write_wakeup(port);
    }

    tty_flip_buffer_push(&port->state->port);
}

static void virt_uart_stop_rx(struct uart_port *port)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);
}

static int virt_uart_startup(struct uart_port *port)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);

    return 0;
}

static void virt_uart_shutdown(struct uart_port *port)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);
}

static void virt_uart_set_termios(struct uart_port *port,
                                  struct ktermios *termios,
                                  struct ktermios *oldtermios)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);
}

static void virt_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);
}

static unsigned int virt_uart_get_mctrl(struct uart_port *port)
{
    dev_info(port->dev, "%s\n", __FUNCTION__);

    return 0;
}

static struct uart_ops virt_uart_ops = {
    .tx_empty = virt_uart_tx_empty,
    .stop_tx = virt_uart_stop_tx,
    .start_tx = virt_uart_start_tx,
    .stop_rx = virt_uart_stop_rx,
    .startup = virt_uart_startup,
    .shutdown = virt_uart_shutdown,
    .set_termios = virt_uart_set_termios,
    .set_mctrl = virt_uart_set_mctrl,
    .get_mctrl = virt_uart_get_mctrl,
};

static struct uart_driver virt_uart_driver = {
    .owner = THIS_MODULE,
    .driver_name = "virt_uart",
    .dev_name = "vttyS",
    .cons = NULL,
    .nr = 2,
};

static int virt_uart_probe(struct platform_device *pdev)
{
    int ret;
    struct virt_uart_port *vp;
    struct uart_port *u;
    unsigned int index;
    struct device_node *np = pdev->dev.of_node;

    vp = devm_kzalloc(&pdev->dev, sizeof(*vp), GFP_KERNEL);
    if (!vp) {
        dev_err(&pdev->dev, "Failed to allocate memory for vp\n");
        return -ENOMEM;
    }

    ret = of_property_read_u32(np, "port-index", &index);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read port index\n");
        return ret;
    }

    u = &vp->port;
    u->ops = &virt_uart_ops;
    u->dev = &pdev->dev;
    u->line = index;
    // type必须设置，否则在uart_port_startup()中检测port->type ==
    // PORT_UNKNOWN会导致set_bit(TTY_IO_ERROR, &tty->flags)，
    // 导致读写时因判断tty_io_error(tty)出现错误不能读写
    u->type = PORT_VIRT;
    u->fifosize = 32;

    ret = uart_add_one_port(&virt_uart_driver, u);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add uart port, err = %d\n", ret);
        return ret;
    }

    platform_set_drvdata(pdev, vp);

    return 0;
}

static int virt_uart_remove(struct platform_device *pdev)
{
    struct virt_uart_port *vp = platform_get_drvdata(pdev);
    struct uart_port *u = &vp->port;

    uart_remove_one_port(&virt_uart_driver, u);

    return 0;
}

static const struct of_device_id virt_uart_dt_ids[] = {
    {
        .compatible = "xm,virt-uart",
    },
    {},
};
MODULE_DEVICE_TABLE(of, virt_uart_dt_ids);

static struct platform_driver virt_uart_platform_driver = {
    .driver =
        {
            .name = "serial-virt",
            .of_match_table = virt_uart_dt_ids,
        },
    .probe = virt_uart_probe,
    .remove = virt_uart_remove,
};

static int __init virt_uart_init(void)
{
    int ret;

    ret = uart_register_driver(&virt_uart_driver);
    if (ret < 0) {
        pr_err("Could not register %s driver\n", virt_uart_driver.driver_name);
        return ret;
    }

    ret = platform_driver_register(&virt_uart_platform_driver);
    if (ret < 0) {
        pr_err("Uart platform driver register failed, err = %d\n", ret);
        uart_unregister_driver(&virt_uart_driver);
        return ret;
    }

    return 0;
}
static void __exit virt_uart_exit(void)
{
    platform_driver_unregister(&virt_uart_platform_driver);
    uart_unregister_driver(&virt_uart_driver);
}
module_init(virt_uart_init);
module_exit(virt_uart_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virtual serial driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");