#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/blkdev.h>

//#define NOUSE_IO_SCHEDULE

#define VIRT_DISK_NAME "virt_disk"
#define SECTOR_SIZE 512
#define SECTOR_NUM 2048

struct virt_disk {
    struct gendisk *disk;
    unsigned int major;
    unsigned char *virt_buffer;
};

#ifndef NOUSE_IO_SCHEDULE
static DEFINE_SPINLOCK(virt_disk_lock);
#endif

#ifndef NOUSE_IO_SCHEDULE
static void virt_disk_request(struct request_queue *rq)
{
    struct request *req;
    unsigned long len;
    struct virt_disk *vd;
    void *buf;
    char *start;

    req = blk_fetch_request(rq);
    while (req) {
        vd = req->rq_disk->private_data;
        buf = bio_data(req->bio);
        start = vd->virt_buffer + (int)blk_rq_pos(req) * SECTOR_SIZE;
        len = blk_rq_cur_bytes(req);

        if (rq_data_dir(req) == READ) {
            printk("virt_disk_request read\n");
            memcpy(buf, start, len);
        } else {
            printk("virt_disk_request write\n");
            memcpy(start, buf, len);
        }
        if (!__blk_end_request_cur(req, 0))
            req = blk_fetch_request(rq);
    }
}
#else
static blk_qc_t virt_disk_make_request(struct request_queue *queue,
                                       struct bio *bio)
{
    struct bio_vec vec;
    struct bvec_iter iter;
    void *buf;
    char *start;
    struct virt_disk *vd = bio->bi_bdev->bd_disk->private_data;

    start = vd->virt_buffer + bio->bi_iter.bi_sector * SECTOR_SIZE;
    bio_for_each_segment(vec, bio, iter)
    {
        buf = page_address(vec.bv_page) + vec.bv_offset;
        if (bio_data_dir(bio) == READ) {
            printk("virt_disk_make_request read\n");
            memcpy(buf, start, vec.bv_len);
        } else {
            printk("virt_disk_make_request write\n");
            memcpy(start, buf, vec.bv_len);
        }
        start += vec.bv_len;
    }
    bio_endio(bio);

    return BLK_QC_T_NONE;
}
#endif

static struct block_device_operations virt_disk_fops = {
    .owner = THIS_MODULE,
};

static int virt_disk_probe(struct platform_device *pdev)
{
    int ret;
    struct virt_disk *vd;

    vd = devm_kzalloc(&pdev->dev, sizeof(*vd), GFP_KERNEL);
    if (!vd) {
        dev_err(&pdev->dev, "Failed to allocate memory for vd\n");
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, vd);

    vd->virt_buffer =
        devm_kzalloc(&pdev->dev, SECTOR_SIZE * SECTOR_NUM, GFP_KERNEL);
    if (!vd->virt_buffer) {
        dev_err(&pdev->dev, "Failed to allocate memory for virt buffer\n");
        return -ENOMEM;
    }

    ret = register_blkdev(0, VIRT_DISK_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to register major\n");
        return ret;
    }

    vd->major = ret;
    vd->disk = alloc_disk(1);
    if (!vd->disk) {
        dev_err(&pdev->dev, "Failed to allocate disk\n");
        ret = -ENOMEM;
        goto err_alloc_disk;
    }
#ifdef NOUSE_IO_SCHEDULE
    vd->disk->queue = blk_alloc_queue(GFP_KERNEL);
#else
    vd->disk->queue = blk_init_queue(virt_disk_request, &virt_disk_lock);
#endif
    if (!vd->disk->queue) {
        dev_err(&pdev->dev, "Failed to init block queue\n");
        ret = -ENOMEM;
        goto err_init_queue;
    }
#ifdef NOUSE_IO_SCHEDULE
    blk_queue_make_request(vd->disk->queue, virt_disk_make_request);
#endif
    vd->disk->fops = &virt_disk_fops;
    vd->disk->major = vd->major;
    vd->disk->first_minor = 0;
    vd->disk->private_data = vd;
    sprintf(vd->disk->disk_name, VIRT_DISK_NAME);
    set_capacity(vd->disk, SECTOR_NUM);
    add_disk(vd->disk);
    return 0;

err_alloc_disk:
    unregister_blkdev(vd->major, VIRT_DISK_NAME);
err_init_queue:
    put_disk(vd->disk);
    return ret;
}

static int virt_disk_remove(struct platform_device *pdev)
{
    struct virt_disk *vd = platform_get_drvdata(pdev);

    del_gendisk(vd->disk);
    blk_cleanup_queue(vd->disk->queue);
    put_disk(vd->disk);
    unregister_blkdev(vd->major, VIRT_DISK_NAME);

    return 0;
}

static const struct of_device_id virt_disk_dt_ids[] = {
    {
        .compatible = "xm,virt-disk",
    },
    {},
};
MODULE_DEVICE_TABLE(of, virt_disk_dt_ids);

static struct platform_driver virt_disk_platform_driver = {
    .driver =
        {
            .name = "virt-disk",
            .of_match_table = virt_disk_dt_ids,
        },
    .probe = virt_disk_probe,
    .remove = virt_disk_remove,
};

module_platform_driver(virt_disk_platform_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virtual block device driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");