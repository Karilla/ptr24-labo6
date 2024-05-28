#ifndef BOARD_H
#define BOARD_H

#include <unistd.h>
#include <stdint.h>

#define INIT_IOCTL      1 << 0 
#define INIT_AUDIO      1 << 1 
#define INIT_VIDEO      1 << 2    

/**
 * \brief Initializes the board
 *
 * This function should be called at the beginning of the main(),
 * before any access is made to the various peripherals
 * 
 * \return 0 if success, negative value if error
 */
int init_board(unsigned init_opt_mask);

#endif // BOARD_H
