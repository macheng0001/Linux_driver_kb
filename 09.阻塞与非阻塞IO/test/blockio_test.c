#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

static pthread_t pthread[2];
static void *pthead_fn1(void *arg)
{
    int fd, key_value, ret;

    fd = open("/dev/block_io", O_RDWR);
    if (fd < 0) {
        printf("Can't open device\n");
        return NULL;
    }

    while (1) {
        key_value = 0;
        ret = read(fd, &key_value, sizeof(key_value));
        if (ret < 0) {
            perror("read failed");
        }

        if (key_value) {
            printf("key press\n");
        }
    }

    close(fd);
}

static void *pthead_fn2(void *arg)
{
    while (1) {
        sleep(5);
        pthread_kill(pthread[0], SIGCHLD);
    }
}

void sig_handler(int sigo) { printf("sig_handler\n"); }

int main(int argc, char **argv)
{
    int ret;
    struct sigaction act;

    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    ret = pthread_create(&pthread[0], NULL, pthead_fn1, NULL);
    if (ret != 0)
        pthread_exit(NULL);

    ret = pthread_create(&pthread[1], NULL, pthead_fn2, NULL);
    if (ret != 0)
        pthread_exit(NULL);

    pthread_join(pthread[0], NULL);
    pthread_join(pthread[1], NULL);

    return 0;
}
