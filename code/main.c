/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: main.c
 *
 * Description: setup of audio, video and control tasks that will run in
 * parallel. This application is used to overload the CPU and analyse the
 * tasks execution time.
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <cobalt/stdio.h>

#include <alchemy/task.h>
#include <alchemy/event.h>

#include "audio_utils.h"
#include "video_utils.h"
#include "io_utils.h"
#include "audio_setup.h"
#include "video_setup.h"
#include "control.h"
#include "busy_cpu.h"

// IOCTL defines
#define KEY0 (0x1 << 0)
#define KEY1 (0x1 << 1)
#define KEY2 (0x1 << 2)
#define KEY3 (0x1 << 3)

#define SW0 0x1

// Overloading 
#define OVERLOAD_BASE_VALUE 20
#define OVERLOAD_STEP 10
#define MAX_OVERLOAD 100
#define MIN_OVERLOAD 0

#define OVERLOADING_PERIOD      ((RTIME)(100000000UL))

// Control task defines
#define CTL_TASK_PERIOD 100000000 // 10HZ
#define CTL_TASK_PRIORITY 90

void ioctl_ctl_task(void *cookie)
{
    Ctl_data_t *priv = (Ctl_data_t *)cookie;

    rt_task_set_periodic(NULL, TM_NOW, CTL_TASK_PERIOD);

    int current_overload = OVERLOAD_BASE_VALUE; 
    unsigned prev_switches = read_switch(MMAP); 

    while (priv->running)
    {
        unsigned keys = read_key(MMAP);
        unsigned switches = read_switch(MMAP);
        unsigned changed_state; 

        // Check if the key0 is pressed
        if (keys & KEY0) {
            priv->running = false;
        }

        // Increment CPU overload
        if (keys & KEY2) {
            if(current_overload + OVERLOAD_STEP < MAX_OVERLOAD ) {
                current_overload += OVERLOAD_STEP;
            }
            rt_printf("Overload value = %d\n", current_overload);
        }

        // Decrement CPU overload 
        if (keys & KEY3) {
            if(current_overload - OVERLOAD_STEP > MIN_OVERLOAD ) {
                current_overload -= OVERLOAD_STEP;
            }
            rt_printf("Overload value = %d\n", current_overload);
        }

        changed_state = (switches ^ prev_switches) & SW0; 

        // Enable CPU overload 
        if (switches & SW0) {
            if(changed_state) {
                rt_printf("CPU overload: ON\n");
            }
            busy_cpu(current_overload * (OVERLOADING_PERIOD / 100));
        } else {
            // Disable CPU overload 
            if(changed_state) {
                rt_printf("CPU overload: OFF\n");
            }
        }

        prev_switches = switches;
        rt_task_wait_period(NULL);
    }
}

int main(int argc, char *argv[])
{
    int ret;

    printf("----------------------------------\n");
    printf("PTR24 - lab05\n");
    printf("----------------------------------\n");

    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Ioctl setup
    if (init_ioctl()) {
        perror("Could not init IOCTL...\n");
        exit(EXIT_FAILURE);
    }

    // Init structure used to control the program flow
    Ctl_data_t ctl;
    ctl.running = true;

    RT_TASK ioctl_ctl_rt_task;
    // Create the IOCTL control task
    if (rt_task_spawn(&ioctl_ctl_rt_task, "program control task", 0,
                      CTL_TASK_PRIORITY, T_JOINABLE,
                      ioctl_ctl_task, &ctl) != 0) {
        perror("Error while starting acquisition_task");
        exit(EXIT_FAILURE);
    }
    printf("Launched audio acquisition task\n");

    // Audio setup
    if (init_audio()) {
        perror("Could not init the audio...\n");
        exit(EXIT_FAILURE);
    }

    // Init private data used for the audio tasks
    Priv_audio_args_t priv_audio;
    priv_audio.samples_buf = (data_t *)malloc(FIFO_SIZE * NB_CHAN);
    priv_audio.ctl = &ctl;

    // Create the audio acquisition task
    if (rt_task_spawn(&priv_audio.acquisition_rt_task, "audio task", 0,
                      AUDIO_ACK_TASK_PRIORITY, T_JOINABLE,
                      acquisition_task, &priv_audio) != 0) {
        perror("Error while starting acquisition_task");
        exit(EXIT_FAILURE);
    }
    printf("Launched audio acquisition task\n");

    // Video setup
    ret = init_video();
    if (ret < 0) {
        perror("Could not init the video...\n");
        return ret;
    }

    // Init private data used for the video tasks
    Priv_video_args_t priv_video;

    // Allocating a circular buffer to retrieve the data (size is 2*image)
    priv_video.buffer = (uint8_t *)malloc(HEIGHT * WIDTH * BYTES_PER_PIXEL * NB_VIDEO_BUFFERS);
    priv_video.output = (uint8_t *)malloc(HEIGHT * WIDTH * BYTES_PER_PIXEL);
    priv_video.ctl = &ctl;

    if(rt_event_create(&priv_video.event, "event_processing_video_acq", 0, EV_FIFO)) {
        perror("Processing queue");
        exit(EXIT_FAILURE);
    }

    // Create the video acquisition task
    if (rt_task_spawn(&priv_video.rt_acq_task, "video acquisition task", 0, VIDEO_ACQ_TASK_PRIORITY,
                      T_JOINABLE, video_acquisition_task, &priv_video) != 0) {
        perror("Error while starting video acquisition task\n");
        exit(EXIT_FAILURE);
    }
    printf("Launched video acquisition task\n");

    // Create the video processing task
    if (rt_task_spawn(&priv_video.rt_proc_task, "video processing task", 0, VIDEO_ACQ_TASK_PRIORITY,
                      T_JOINABLE, video_processing_task, &priv_video) != 0) {
        perror("Error while starting video processing task\n");
        exit(EXIT_FAILURE);
    }
    printf("Launched video processing task\n");

    printf("----------------------------------\n");
    printf("SW0: Enable/disable CPU overload\n");
    printf("KEY3 pressed: Reduce CPU overload\n");
    printf("KEY2 pressed: Increase CPU overload\n");
    printf("Press KEY0 to exit the program\n");
    printf("----------------------------------\n");

    // Waiting for the end of the program (coming from ctrl + c)
    rt_task_join(&priv_audio.acquisition_rt_task);
    rt_task_join(&priv_video.rt_acq_task);
    rt_task_join(&priv_video.rt_proc_task);
    rt_task_join(&ioctl_ctl_rt_task);

    // Free all resources of the video/audio/ioctl
    clear_ioctl();
    clear_audio();
    clear_video();

    // Free everything else
    free(priv_audio.samples_buf);
    free(priv_video.buffer);
    free(priv_video.output);

    munlockall();

    rt_printf("Application has correctly been terminated.\n");

    return EXIT_SUCCESS;
}
