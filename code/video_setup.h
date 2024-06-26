/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: video_setup.h
 *
 * Description: functions and defines used to launch Xenomai tasks to perform
 * the acquisition of the video data coming from a raw file and display it
 * on the VGA output.
 * The data is RGB0, so each pixel is coded on 32 bits.
 *
 ******************************************************************************/

#ifndef VIDEO_SETUP_H
#define VIDEO_SETUP_H

#include <stdbool.h>
#include <stdint.h>
#include <alchemy/alarm.h>
#include <alchemy/task.h>
#include <alchemy/event.h>

#include "control.h"

#define VIDEO_ACQ_TASK_PRIORITY 50
#define VIDEO_PROC_TASK_PRIORITY 30

// Video defines
#define WIDTH 320
#define HEIGHT 240
#define FRAMERATE 15
#define NB_FRAMES 300
#define BYTES_PER_PIXEL 4
#define NB_VIDEO_BUFFERS 2

#define VIDEO_FILENAME "output_video.raw"

#define S_IN_NS 1000000000UL

enum StateMachine {
    NORMAL = 0,
    DEGRADED,
    STOPPED
};

typedef enum StateMachine StateMachine;


typedef struct Priv_video_args
{
    Ctl_data_t *ctl;
    RT_TASK rt_acq_task;
    RT_TASK rt_proc_task;
    RT_EVENT event;
    RT_ALARM missed_deadline;
    RT_ALARM alarm;
    StateMachine state;
    uint8_t *buffer;
    uint8_t *output;
} Priv_video_args_t;

void missed_deadline(void *cookie);
void alarm_handler(void *cookie);

/**
 * \brief Read the video data from a file and writes it to the
 * DE1-SoC VGA output to display the RGB0 data. The size of the
 * picture displayed is locked to 320x240 and is only RGB0.
 *
 * \param cookie pointer to private data. Can be anything
 */
void video_acquisition_task(void *cookie);

/**
 * \brief Task waiting to be signaled. The processing will convert to
 * to grayscale and display the picture on the screen
 *
 * \param cookie pointer to private data. Can be anything
 */
void video_processing_task(void *cookie);

#endif // VIDEO_SETUP_H
