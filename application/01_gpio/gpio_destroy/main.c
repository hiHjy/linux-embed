#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#define GPIO_PATH_EXPORT "/sys/class/gpio/export"
#define GPIO_PATH_ROOT "/sys/class/gpio/"

static void gpio_init (char *gpio_num, char* direction) 
{
    
    int fd = open(GPIO_PATH_EXPORT, O_WRONLY);
    if (fd < 0) {
        perror("open GPIO_PATH_EXPORT error");
        exit(1);
    }
    printf("open GPIO_PATH_EXPORT success\n");

    int ret = write(fd, gpio_num, strlen(gpio_num));
    if (ret < 0) {
        perror("write gpio_num to export error");
        exit(1);
    }
    printf("write %s success\n", gpio_num);

    char path[50];
    memset(path, 0, sizeof(path));

    strcpy(path, GPIO_PATH_ROOT);//"/sys/class/gpio/"
    char gpio_x[10];
    memset(gpio_x, 0, sizeof(gpio_x));
    strcpy(gpio_x, "gpio"); //gpio
    strcat(gpio_x, gpio_num);//gpiox
    strcat(path, gpio_x);
    printf("init gpio%s success!\n", gpio_x);
    
    
    if (strcmp(direction, "out") == 0) {
        char tmp[50];
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, path);
        strcat(tmp, "/direction");
    

        int gpio_direction_fd = open(tmp, O_WRONLY);
        if (gpio_direction_fd < 0) {
            perror("open direction_path error");
            exit(1);
        }
        
        ret = write(gpio_direction_fd, direction, strlen(direction));
        if (ret < 0) {
            perror("write to direction error");
            exit(1);
        }
        printf("direction: out\n");
        close(gpio_direction_fd);

    } else {
        printf("direction: in\n");
    }
   
    close(fd);
}


int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <gpio_num> <direction>\n", argv[0]);
        exit(1);
    }
    gpio_init(argv[1], argv[2]);
   
    return 0;
}