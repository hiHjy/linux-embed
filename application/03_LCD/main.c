#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include "tslib/tslib.h"
enum {
    TOUCH_UP = 0,
    TOUCH_DOWN,
   
};
int main(int argc, char *argv[])
{
    
    struct tsdev* lcd = ts_setup(NULL, 0);
    assert(lcd != NULL && "ts_open");
    volatile int prev_stat = TOUCH_UP;
    
    
    // int ts_read_mt(struct tsdev *ts, struct ts_sample_mt **samp, int max_slots, int nr)
    struct ts_sample samp;
    /*读数据*/
    while (1) {

        if (ts_read(lcd, &samp, 1) < 0) {
            perror("ts_read faild");
            continue;

        }

        if (samp.pressure > 0) {
            if (prev_stat == TOUCH_UP) {
                printf("按下:%d,%d\n", samp.x, samp.y);
                prev_stat = TOUCH_DOWN;
            }
                
            else if (prev_stat == TOUCH_DOWN) {
                printf("移动:%d,%d\n", samp.x, samp.y);
                
            }
            
        } else {
            if (prev_stat != TOUCH_UP) {
                printf("松开\n");
                prev_stat = TOUCH_UP;
            }
            
        }
        // if (ts_read(lcd, &samp, 1) >= 0) {
        //     if (samp.pressure > 0) {
        //         if (pressure > 0) {
        //             printf("移动：%d,%d\n", samp.x, samp.y);
        //         } else {
                    
        //             printf("按下：%d,%d\n", samp.x, samp.y);
                
        //         }
        //     } else {
        //         printf("松开\n");
        //     }

        // } else {
        //     perror("ts_read error");
        // }
        // pressure = samp.pressure;
        

    }
    
    
    ts_close(lcd); 
    return 0;
}