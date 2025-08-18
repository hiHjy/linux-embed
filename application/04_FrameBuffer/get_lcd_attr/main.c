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
int main(int argc, char *argv[])
{
    /*打开设备文件*/
    int fd = open("/dev/fb0", O_RDWR);
    
    /*获取屏幕固定参数*/
    struct fb_fix_screeninfo fb_fix;
    int ret = ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);
    assert(ret != -1 && "ioctl FBIOGET_FSCREENINFO");

    /*获取屏幕可变参数*/
    struct fb_var_screeninfo fb_var;
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    assert(ret != -1 && "ioctl FBIOGET_VSCREENINFO");

    /*打印获得的参数*/
    printf("分辨率: %d*%d\n"
    "像素深度 bpp: %d\n"
    "一行的字节数: %d\n"
    "像素格式: R<%d %d> G<%d %d> B<%d %d>\n帧大小:%dMB\n",
    fb_var.xres, fb_var.yres, fb_var.bits_per_pixel,
    fb_fix.line_length,
    fb_var.red.offset, fb_var.red.length,
    fb_var.green.offset, fb_var.green.length,
    fb_var.blue.offset, fb_var.blue.length,
    fb_var.xres * fb_var.yres * 24 / 8 / 1024 / 1024);
    
    close(fd);
    return 0;
}