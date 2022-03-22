#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd;
    const char magic = 'V';
    const char *dev_name = argv[1];
    unsigned int timeout = 0, pretimeout = 0;
    unsigned int fire = 5;

    setbuf(stdout, NULL);

    if (!dev_name) {
        printf("please input watchdog dev\n");
        exit(-1);
    }

    fd = open(dev_name, O_WRONLY);
    if (fd == -1) {
        printf("Watchdog device not enabled.\n");
        exit(-1);
    }

    ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
    printf("watchdog default timeout:%ds\n", timeout);

    pretimeout = timeout / 3;
    ioctl(fd, WDIOC_SETPRETIMEOUT, &pretimeout);

    while (fire) {
        ioctl(fd, WDIOC_KEEPALIVE, NULL);
        printf(".");
        sleep(1);
        fire--;
    }

    // write(fd, &magic, 1);

    close(fd);
    return 0;
}