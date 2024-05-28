#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "io_utils.h"
#include "video_utils.h"
#include "audio_utils.h"
#include "board.h"

int init_board(unsigned init_opt_mask)
{
    // int ret; 
    // // Set the HEX to "PTR24-", this ensure the system is working as expected
    // unsigned hex_values[] = {HEX_DIGIT_4, HEX_DIGIT_2, 0x40, 0x50, 0x78, 0x73};

    // if(init_opt_mask && INIT_IOCTL) {
    //     ret = init_ioctl();
    //     if(ret < 0) {
    //         return ret;
    //     }

    //     for(int i = 0; i < NB_HEX; i++) {
    //         write_hex(IOCTL, i, hex_values[i]);
    //     }

    //     write_led(IOCTL, 0x0);

    //     write_gpio_en(IOCTL, 0, REG_LOW, 0);
    //     write_gpio_en(IOCTL, 0, REG_HIGH, 0);
    //     write_gpio_en(IOCTL, 1, REG_LOW, 0);
    //     write_gpio_en(IOCTL, 1, REG_HIGH, 0);
    // }

    // if(init_opt_mask && INIT_VIDEO) {
    //     init_video();
    // }

    return 0;
}

