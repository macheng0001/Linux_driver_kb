#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

int main(int argc, char *argv[])
{
    int fd = -1, ret;
    int version;
    char name[20];
    struct input_event ev;
    int i = 0;

    if ((fd = open("/dev/input/event2", O_RDONLY)) < 0) {
        perror("open error");
        exit(1);
    }

    while (1) {
        ret = read(fd, &ev, sizeof(struct input_event));
        if (ret < 0) {
            perror("read error");
            exit(1);
        }

        if (EV_KEY == ev.type) {
            printf("event=%s,value=%d\n",
                   ev.code == BTN_0 ? "BTN_0" : "UNKNOWEN", ev.value);
        } else if (EV_SYN == ev.type) {
        } else {
            printf("UNKNOWEN event type:%x\n", ev.type);
        }
    }

    close(fd);

    return 0;
}
