#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/iio/events.h>
#include <linux/iio/types.h>
#include <limits.h>
#include <pthread.h>

#define ATTR_MAX 128

struct iio_device {
    char dev_name[32]; //设备名称
    int dev_fd;        //设备句柄
    int event_fd;      //设备事件句柄
};

static int sysfs_read_attr(const char *device, const char *attr, char *buf)
{
    char path[NAME_MAX];
    char *p;
    FILE *f;

    snprintf(path, NAME_MAX, "%s/%s", device, attr);

    if (!(f = fopen(path, "r")))
        return -1;
    p = fgets(buf, ATTR_MAX, f);
    fclose(f);
    if (!p)
        return -1;

    /* Last byte is a '\n'; chop that off */
    p[strlen(buf) - 1] = '\0';

    return 0;
}

static int sysfs_write_attr(const char *device, const char *attr,
                            const char *buf)
{
    char path[NAME_MAX];
    int ret;
    FILE *f;

    snprintf(path, NAME_MAX, "%s/%s", device, attr);

    if (!(f = fopen(path, "w")))
        return -1;
    ret = fputs(buf, f);
    if (fclose(f) < 0)
        return -1;
    if (ret < 0)
        return ret;

    return 0;
}

int iio_device_init(struct iio_device *iio_dev)
{
    int fd, event_fd, ret;

    fd = open(iio_dev->dev_name, O_RDWR);
    if (fd < 0) {
        return fd;
    }

    ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
    printf("ret:%d,%d\n", ret, fd);
    if (ret == -1 || event_fd == -1) {
        close(fd);
        return ret;
    }

    iio_dev->dev_fd = fd;
    iio_dev->event_fd = event_fd;

    return 0;
}

int iio_device_release(struct iio_device *iio_dev)
{
    close(iio_dev->event_fd);
    close(iio_dev->dev_fd);

    return 0;
}

static const char *const iio_ev_dir_text[] = {[IIO_EV_DIR_EITHER] = "either",
                                              [IIO_EV_DIR_RISING] = "rising",
                                              [IIO_EV_DIR_FALLING] = "falling"};

static void print_event(struct iio_event_data *event)
{
    enum iio_chan_type type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event->id);
    enum iio_modifier mod = IIO_EVENT_CODE_EXTRACT_MODIFIER(event->id);
    enum iio_event_type ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event->id);
    enum iio_event_direction dir = IIO_EVENT_CODE_EXTRACT_DIR(event->id);
    int chan = IIO_EVENT_CODE_EXTRACT_CHAN(event->id);
    int chan2 = IIO_EVENT_CODE_EXTRACT_CHAN2(event->id);
    bool diff = IIO_EVENT_CODE_EXTRACT_DIFF(event->id);

    if (type != IIO_LIGHT || ev_type != IIO_EV_TYPE_THRESH) {
        printf("Unknown event: time: %lld, id: %llx\n", event->timestamp,
               event->id);

        return;
    }

    printf("Event: time: %lld, type: %s", event->timestamp, "illuminance");

    if (chan >= 0) {
        printf(", channel: %d", chan);
        if (diff && chan2 >= 0)
            printf("-%d", chan2);
    }

    printf(", evtype: %s", "thresh");

    if (dir != IIO_EV_DIR_NONE)
        printf(", direction: %s", iio_ev_dir_text[dir]);

    printf("\n");
}

static void *read_event(void *arg)
{
    struct iio_device *iio_device = (struct iio_device *)arg;
    int ret;
    struct iio_event_data event;

    //设置上下阈值
    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance0_thresh_falling_value", "10") < 0) {
        perror("set illuminance0 falling thresh failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance0_thresh_rising_value", "20") < 0) {
        perror("set illuminance0 rising thresh failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance1_thresh_falling_value", "30") < 0) {
        perror("set illuminance1 falling thresh failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance1_thresh_rising_value", "40") < 0) {
        perror("set illuminance1 rising thresh failed");
    }

    //使能事件
    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance0_thresh_falling_en", "1") < 0) {
        perror("enable illuminance0 falling event failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/events",
                         "in_illuminance1_thresh_falling_en", "1") < 0) {
        perror("enable illuminance1 falling event failed");
    }

    while (1) {
        ret = read(iio_device->event_fd, &event, sizeof(event));
        if (ret < 0 || ret != sizeof(event)) {
            perror("Reading event failed");
        } else {
            print_event(&event);
        }
    }
}

static void *read_buffer(void *arg)
{
    struct iio_device *iio_device = (struct iio_device *)arg;
    int ret, i;
    char data[100];

    //使能通道
    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/scan_elements",
                         "in_illuminance0_en", "1") < 0) {
        perror("enable illuminance0 channel failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/scan_elements",
                         "in_illuminance1_en", "1") < 0) {
        perror("enable illuminance1 channel failed");
    }

    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/scan_elements",
                         "in_timestamp_en", "1") < 0) {
        perror("enable timestamp channel failed");
    }

    //创建触发器
    if (sysfs_write_attr("/sys/bus/iio/devices/iio_sysfs_trigger",
                         "add_trigger", "0") < 0) {
        perror("create sysfs trigger failed");
    }

    //绑定触发器
    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/trigger/",
                         "current_trigger", "sysfstrig0") < 0) {
        perror("attach trigger failed");
    }

    //使能缓冲
    if (sysfs_write_attr("/sys/bus/iio/devices/iio:device1/buffer", "enable",
                         "1") < 0) {
        perror("enable buffer failed");
    }

    while (1) {
        ret = read(iio_device->dev_fd, data, 100);
        if (ret < 0) {
            perror("Reading event failed");
        } else {
            for (i = 0; i < ret; i++)
                printf("data[%d]= %x\n", i, data[i]);
        }
    }
}

static void *sysfs_trigger(void *arg)
{
    while (1) {
        sleep(1);
        //触发触发器
        if (sysfs_write_attr("/sys/bus/iio/devices/trigger0", "trigger_now",
                             "1") < 0) {
            perror("trigger failed");
        }
    }
}

int main()
{
    struct iio_device iio_device;
    int ret, i;

    pthread_t event_thread, buffer_thread, trigger_thread;

    strcpy(iio_device.dev_name, "/dev/iio:device1");
    if (iio_device_init(&iio_device) < 0) {
        perror("iio device init failed");
        exit(-1);
    }

    pthread_create(&event_thread, NULL, read_event, &iio_device);
    pthread_create(&buffer_thread, NULL, read_buffer, &iio_device);
    pthread_create(&trigger_thread, NULL, sysfs_trigger, &iio_device);
    pthread_join(event_thread, NULL);
    pthread_join(buffer_thread, NULL);
    pthread_join(trigger_thread, NULL);

    iio_device_release(&iio_device);

    return 0;
}