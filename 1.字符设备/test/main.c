#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int fd, i;
    char buf[10];

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }

    for (i = 0; i < 10; i++)
        buf[i] = i + 1;
    write(fd, buf, 10);

    close(fd);
    return 0;
}
