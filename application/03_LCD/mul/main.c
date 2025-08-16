#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <linux/input.h>
#include <string.h>
#include "../tslib/tslib.h"
#include <sys/ioctl.h>
#define MAX_SAMP 5
enum {
    TOUCH_UP = 0,
    TOUCH_DOWN,
   
};
int main(int argc, char *argv[])
{
    
    struct tsdev* lcd = ts_setup(NULL, 0);
    assert(lcd != NULL && "ts_setup");
  

    /*获取最大触摸点*/
    struct input_absinfo slot;
    if (ioctl(ts_fd(lcd), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
        perror("ioctl error");
        ts_close(lcd);
        exit(-1); 
    }
    int max_slot = slot.maximum + 1 - slot.minimum;
    printf("max_slot:%d\n", max_slot);
    
    /*初始化按下状态*/
    int* prev_stat = malloc(sizeof(int) * max_slot);
    memset(prev_stat, 0, max_slot * sizeof(int));
    /*为最大触摸数量提前申请空间*/
    struct ts_sample_mt *samp = malloc(sizeof(struct ts_sample_mt) * max_slot);
    
    /*读数据*/
    while (1) {

        /* int ts_read_mt(struct tsdev *ts, struct ts_sample_mt **samp, int max_slots, int nr)*/
        if (ts_read_mt(lcd, &samp, max_slot, 1) < 0) {
            perror("ts_read_mt error");
            // free(samp);
            // ts_close(lcd);
            continue;
        }
        
        /*处理每一个触摸点*/
        for (int i = 0; i < max_slot; ++i) {
            if ((samp[i].valid > 0)) { //表示有更新
                /*保存slot号*/
                int slot_num = samp[i].slot;
                if (samp[i].pressure > 0) {
                    if (prev_stat[slot_num] == TOUCH_UP) {
                        printf("按下:slot[%d]  %d,%d\n", slot_num, samp[i].x, samp[i].y);
                        prev_stat[i] = TOUCH_DOWN;  
                    } else if (prev_stat[slot_num] == TOUCH_DOWN) {
                        printf("移动:slot[%d]  %d,%d\n", slot_num, samp[i].x, samp[i].y);
                    }
                    
                } else {
                    printf("松开\n");
                    prev_stat[i] = TOUCH_UP;
                }
            }   

        }

    }
    free(prev_stat);
    free(samp);
    ts_close(lcd); 
    return 0;
}