#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define MEM_SIZE 1024

int main(int argc, char **argv)
{
    int fd, i;
    char buf_r[MEM_SIZE], buf_w[MEM_SIZE];

    for (i = 0; i < MEM_SIZE; i++)
        buf_w[i] = i + 1;

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, buf_w, MEM_SIZE);

    lseek(fd, 0, SEEK_SET);
    read(fd, buf_r, MEM_SIZE);

    assert(memcmp(buf_w, buf_r, MEM_SIZE) == 0);

    close(fd);
    return 0;
}
