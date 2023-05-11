#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/timer.h>

struct proc_test_priv {
    struct timer_list timer;
    atomic_t timer_value;
} proc_test_priv;

static int proc_test_show(struct seq_file *seq, void *v)
{
    struct proc_test_priv *priv = seq->private;
    seq_printf(seq, "timer_value:%d\n", atomic_read(&priv->timer_value));
    return 0;
}

static int proc_test_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_test_show, PDE_DATA(inode));
}

static const struct file_operations proc_test_fops = {
    .owner = THIS_MODULE,
    .open = proc_test_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static void timer_handler(unsigned long data)
{
    struct proc_test_priv *priv = (struct proc_test_priv *)data;
    atomic_inc(&priv->timer_value);
    mod_timer(&priv->timer, jiffies + 1 * HZ);
    return;
}

static int proc_test_init(void)
{
    struct proc_dir_entry *dir, *entry;
    struct proc_test_priv *priv = &proc_test_priv;

    atomic_set(&priv->timer_value, 0);
    priv->timer.data = (unsigned long)priv;
    priv->timer.function = timer_handler;
    priv->timer.expires = jiffies + 1 * HZ;
    init_timer(&priv->timer);
    add_timer(&priv->timer);

    dir = proc_mkdir("myproc", NULL);
    if (!dir)
        return -ENOMEM;

    entry = proc_create_data("myfile", 0, dir, &proc_test_fops, priv);
    if (!entry)
        return -ENOMEM;

    return 0;
}

static void proc_test_exit(void)
{
    struct proc_test_priv *priv = &proc_test_priv;
    del_timer_sync(&priv->timer);
    remove_proc_subtree("myproc", NULL);
}

module_init(proc_test_init);
module_exit(proc_test_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("proc test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
