#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/input.h>


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(1);
    }
    
    int fd = open(argv[1], O_RDONLY);
    
    struct input_event in = {0};

    while (1) {
        read(fd, &in, sizeof(in));
        printf("type:%d code:%d value:%d\n", in.type, in.code, in.value);
    }
    
   close(fd);
    return 0;
}