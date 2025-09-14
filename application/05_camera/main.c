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
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <stdint.h>
#define V4L2_DEV_PATH "/dev/video1"
#define FRAMEBUFFER_COUNT 4
#define LCD_PATH "/dev/fb0"
static int lcd_width;
static int lcd_height;
static int lcd_fd = -1;
static unsigned short* screen_base;
static int width = 800;
static int height = 480;
static int v4l2_fd = -1;
struct cam_buf_info {
    unsigned long length;
    unsigned char* start;
};
static struct cam_buf_info buf_infos[FRAMEBUFFER_COUNT];

typedef struct camera_format {

    unsigned char description[32]; //字符串描述信息
    unsigned int pixelformat; //像素格式

} cam_fmt;


/*** 初始化摄像头 ***/
static void v4l2_dev_init()
{
    struct v4l2_capability cap = {0};
    printf("正在初始化v4l2设备...\n");

    /* 打开摄像头 */
    v4l2_fd = open(V4L2_DEV_PATH, O_RDWR);
    if (v4l2_fd < 0) {
        perror("open v4l2_dev error");
        exit(-1);
    }
    /*查询设备功能*/
    if (ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("ioctl VIDIOC_QUERYCAP error");
        close(v4l2_fd);
        exit(-1);
    }
    
    /*判断是否为视频采集设备*/
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        //如果不是视频采集设备 !(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities) 值为非0
        printf("非视频采集设备\n");
        close(v4l2_fd);
        exit(-1);
    }
    
    /*查询采集设备支持的所有像素格式及描述信息*/
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct camera_format camfmts[10] = {0};    
    while (ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        printf("index:%d 像素格式:0x%x, 描述信息:%s\n",
            fmtdesc.index,
            fmtdesc.pixelformat, 
            fmtdesc.description);
        /*将支持的像素格式存入结构体数组*/
        strcpy(camfmts[fmtdesc.index].description, fmtdesc.description);
        camfmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        fmtdesc.index++;
    }
    printf("已获取全部支持的格式\n");
    
    /* 枚举出摄像头所支持的所有视频采集分辨率 */

    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_frmivalenum frmival = {0};

    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    for (int i = 0; camfmts[i].pixelformat; ++i) {

        frmsize.index = 0;
        frmsize.pixel_format = camfmts[i].pixelformat;  // 设置要查询的像素格式
        frmival.pixel_format = camfmts[i].pixelformat;  // 设置要查询帧率的像素格式

        while (ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {

            printf("size<%d*%d> ",
            frmsize.discrete.width,//宽
            frmsize.discrete.height);//高
            frmsize.index++;

            /*查询帧率的像素格式*/
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {

                printf(" <%dfps> ", frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");

        }
        printf("\n");
    }


} 

static int v4l2_set_format()
{   
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm streamparm = {0};

    /* 设置帧格式 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat =  0x56595559;

    if (ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("ioctl VIDIOC_QUERYCAP error");
        close(v4l2_fd);
        return -1;
    }

    if (fmt.fmt.pix.pixelformat !=  0x56595559) {
        fprintf(stderr, "不支持YUYV\n");
        close(v4l2_fd);
        return -1;
    }

    printf("当前视频分辨率为<%d * %d>\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    
    /* 获取 streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm) < 0) {
        perror("ioctl VIDIOC_G_PARM error");
        close(v4l2_fd);
        return -1;
    }
    
    /*检测是否支持帧率设置*/
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        //走到这里表示支持帧率设置

        /*设置30fps*/
        printf("该v4l2设备支持帧率设置\n");
        streamparm.parm.capture.timeperframe.denominator = 30;
        streamparm.parm.capture.timeperframe.numerator = 1;
        if (ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm) < 0) {
            perror("ioctl VIDIOC_S_PARM error");
            close(v4l2_fd);
            return -1;
        }
        
    }

    return 0;
}

void fb_info ()
{
    // /*打开设备文件*/
    // int fd = open("/dev/fb0", O_RDWR);
    
    /*获取屏幕固定参数*/
    struct fb_fix_screeninfo fb_fix;
    int ret = ioctl(lcd_fd, FBIOGET_FSCREENINFO, &fb_fix);
    assert(ret != -1 && "ioctl FBIOGET_FSCREENINFO");

    /*获取屏幕可变参数*/
    struct fb_var_screeninfo fb_var;
    ret = ioctl(lcd_fd, FBIOGET_VSCREENINFO, &fb_var);
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
    
   
}

// static void fill_lcd(unsigned int start_x, unsigned int end_x,
// unsigned int start_y, unsigned int end_y,
// unsigned int color)
// {
//     unsigned short rgb565_color = argb8888_to_rgb565(color);
    
//     if (end_x >= lcd_width) end_x = lcd_width- 1;
//     if (end_y >= lcd_height) end_y = lcd_height - 1;

//     /*填充颜色*/
//     unsigned long temp = start_y *lcd_height;

//     for (; start_y <= end_y ; start_y++, temp += width) {

//         for (int x = start_x; x <= end_x; ++x) {
//             screen_base[temp + x] = rgb565_color; 
//         }

//     }

// }


static int v4l2_init_buffer()
{
    /*申请缓冲区*/
    struct v4l2_requestbuffers reqbuf = {0};
    struct v4l2_buffer buf = {0};
    reqbuf.count = FRAMEBUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        perror("ioctl VIDIOC_REQBUFS error");
        close(v4l2_fd);
        exit(-1);
    }

    /*建立内存映射*/
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; ++buf.index) {
        if (ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0) return -1;
        buf_infos[buf.index].length = buf.length;
        buf_infos[buf.index].start = mmap(NULL,  
            buf.length,
            PROT_READ |PROT_WRITE, 
            MAP_SHARED, 
            v4l2_fd,
            buf.m.offset
        );

        if (buf_infos[buf.index].start == MAP_FAILED) {
            fprintf(stderr, "index%d号缓冲区内存映射失败\n", buf.index);
            for (int i = 0; i < buf.index; ++i) {
                munmap(buf_infos[i].start, buf_infos[i].length);
                
               
            }
            close(v4l2_fd);
            return -1;
        }
        printf("内存建立映射完成\n");

    }

    
    for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; ++buf.index) {
        if (ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("ioctl VIDIOC_QBUF error");
            close(v4l2_fd);
            return -1;

        }

    }
    
    printf("帧缓存区已准备就绪!\n");
    return 0;

}



// 使用整数运算的优化版本
void yuyv_to_rgb565_optimized(char *yuyv, short *rgb, int width, int height)
{
    int i, j;
    int y0, y1, u, v;
    int r, g, b;
    
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i += 2) {
            int index = j * width * 2 + i * 2;
            y0 = yuyv[index];
            u  = yuyv[index + 1];
            y1 = yuyv[index + 2];
            v  = yuyv[index + 3];
            
            // 使用整数运算代替浮点运算
            // 转换第一个像素
            r = y0 + ((359 * (v - 128)) >> 8);      // 1.402 ≈ 359/256
            g = y0 - ((88 * (u - 128) + 183 * (v - 128)) >> 8); // 0.344≈88/256, 0.714≈183/256
            b = y0 + ((454 * (u - 128)) >> 8);      // 1.772 ≈ 454/256
            
            // 限制范围
            r = (r < 0) ? 0 : (r > 255) ? 255 : r;
            g = (g < 0) ? 0 : (g > 255) ? 255 : g;
            b = (b < 0) ? 0 : (b > 255) ? 255 : b;
            
            rgb[j * width + i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            
            // 转换第二个像素
            r = y1 + ((359 * (v - 128)) >> 8);
            g = y1 - ((88 * (u - 128) + 183 * (v - 128)) >> 8);
            b = y1 + ((454 * (u - 128)) >> 8);
            
            r = (r < 0) ? 0 : (r > 255) ? 255 : r;
            g = (g < 0) ? 0 : (g > 255) ? 255 : g;
            b = (b < 0) ? 0 : (b > 255) ? 255 : b;
            
            rgb[j * width + i + 1] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
    }
}

static void lcd_init() {
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    unsigned int screen_size;
    /*打开设备文件*/
    lcd_fd = open("/dev/fb0", O_RDWR);
    assert(lcd_fd > 0);
    ioctl(lcd_fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(lcd_fd, FBIOGET_FSCREENINFO, &fb_fix);
    printf("--lcd--<%d*%d>\n", fb_var.xres, fb_var.yres);

    lcd_width = fb_var.xres;
    lcd_height = fb_var.yres;

    /*映射lCD的缓存区*/
    screen_size = fb_fix.line_length * fb_var.yres;
    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if (screen_base == MAP_FAILED) {
        perror("mmap error");
        close(lcd_fd);
        exit(-1);
    }

   memset(screen_base, 0xFF, screen_size); 

}

static inline uint16_t yuv_to_rgb565(uint8_t y, uint8_t u, uint8_t v)
{
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;

    int r = (298*c + 409*e + 128) >> 8;
    int g = (298*c - 100*d - 208*e + 128) >> 8;
    int b = (298*c + 516*d + 128) >> 8;

    if(r<0) r=0; if(r>255) r=255;
    if(g<0) g=0; if(g>255) g=255;
    if(b<0) b=0; if(b>255) b=255;

    return (uint16_t)(((r & 0xF8)<<8) | ((g & 0xFC)<<3) | (b>>3));
}


static int v4l2_stream_on(void) 
{
    /* 打开摄像头、摄像头开始采集数据 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
        perror("ioctl VIDIOC_STREAMON error");
        close(v4l2_fd);
        return -1;
    }
    printf("开始视频采集\n");
    return 0;
}
// YUYV -> RGB565 单帧显示到 LCD
void display_yuyv_frame_on_lcd(uint8_t *yuyv_buf, uint16_t *lcd_mem, int width, int height, int lcd_line_bytes)
{
    for(int y = 0; y < height; y++) {
        uint8_t *src = yuyv_buf + y * width * 2;
        uint16_t *dst = (uint16_t*)((uint8_t*)lcd_mem + y * lcd_line_bytes);

        for(int x = 0; x < width; x += 2) {
            uint8_t y0 = src[0];
            uint8_t u  = src[1];
            uint8_t y1 = src[2];
            uint8_t v  = src[3];

            dst[0] = yuv_to_rgb565(y0, u, v);
            dst[1] = yuv_to_rgb565(y1, u, v);

            src += 4;
            dst += 2;
        }
    }
}




static void v4l2_print_on_lcd()
{
    struct fb_fix_screeninfo fb_fix;
ioctl(lcd_fd, FBIOGET_FSCREENINFO, &fb_fix);
int lcd_line_bytes = fb_fix.line_length;

while(1) {
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(ioctl(v4l2_fd, VIDIOC_DQBUF, &buf) < 0) { perror("DQBUF"); break; }

    display_yuyv_frame_on_lcd(buf_infos[buf.index].start, screen_base, width, height, lcd_line_bytes);

    if(ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) { perror("QBUF"); break; }
}

        
       
}

  


int main(int argc, char *argv[])
{
    v4l2_dev_init();
    lcd_init();
    v4l2_set_format();
    v4l2_init_buffer();
    
    v4l2_stream_on();
    
    v4l2_print_on_lcd();



    return 0;
}