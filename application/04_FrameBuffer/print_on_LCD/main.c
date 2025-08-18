#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <sys/mman.h>
#define argb8888_to_rgb565(color) ({ \
unsigned int temp = (color); \
((temp & 0xF80000UL) >> 8) | \
((temp & 0xFC00UL) >> 5) | \
((temp & 0xF8UL) >> 3); \
})

static int width;
static int height;
static unsigned short* screen_base = NULL;


static void fill_lcd(unsigned int start_x, unsigned int end_x,
unsigned int start_y, unsigned int end_y,
unsigned int color)
{
    unsigned short rgb565_color = argb8888_to_rgb565(color);
    
    if (end_x >= width) end_x = width - 1;
    if (end_y >= height) end_y = height - 1;

    /*填充颜色*/
    unsigned long temp = start_y * width;

    for (; start_y <= end_y ; start_y++, temp += width) {

        for (int x = start_x; x <= end_x; ++x) {
            screen_base[temp + x] = rgb565_color; 
        }

    }

}

int main(int argc, char *argv[])
{
    
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    unsigned int screen_size;
    /*打开设备文件*/
    int fd = open("/dev/fb0", O_RDWR);
    
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);
    width = fb_var.xres;
    height = fb_var.yres;

    /*映射lCD的缓存区*/
    screen_size = fb_fix.line_length * fb_var.yres;
    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (screen_base == NULL) {
        perror("mmap error");
        close(fd);
        exit(-1);
    }

    /*画一个矩形*/
    int h = height / 2;
    int w = width / 2;
    fill_lcd(0, width-1, 0, height-1, 0x0);
    fill_lcd(0, w - 1, 0, h - 1, 0xff0000);
    fill_lcd(0, w - 1, h, height -1, 0x00ff00);
    munmap(screen_base, screen_size);
    close(fd);
    return 0;
}