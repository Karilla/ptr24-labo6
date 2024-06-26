#include <complex.h>
#include <math.h>
#include <stdio.h>

#include <alchemy/task.h>

#include "video_setup.h"
#include "video_utils.h"
#include "grayscale.h"

#define VIDEO_PROCESSING_FLAG (1 << 0)

#define IMAGE_SIZE (HEIGHT * WIDTH * BYTES_PER_PIXEL)

#define EVENT_TIMEOUT (1000000000 / 5) // 0.2s

#define THREEE_SECONDS 3000000000

void missed_deadline(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    if(priv->state == NORMAL) {
        rt_printf("Missed deadline: switch to degraded\n");
        priv->state = DEGRADED;
    } else if(priv->state == DEGRADED) {
        rt_printf("Missed deadline: switch to stopped\n");
        priv->state = STOPPED;
    }
    rt_alarm_start(&priv->alarm, THREEE_SECONDS, TM_INFINITE);
}

void alarm_handler(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    if(priv->state == DEGRADED) {
        rt_printf("Revert back to normal\n");
        priv->state = NORMAL;
    } else if(priv->state == STOPPED) {
        rt_printf("Revert back to degraded\n");
        priv->state = DEGRADED;
    }
    rt_alarm_start(&priv->alarm, THREEE_SECONDS, TM_INFINITE);
}

void video_acquisition_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    uint64_t period_in_ns = S_IN_NS / FRAMERATE;

    FILE *file = fopen(VIDEO_FILENAME, "rb");
    if (!file)
    {
        printf("Error: Couldn't open raw video file.\n");
        return;
    }

    rt_task_set_periodic(NULL, TM_NOW, period_in_ns);

    unsigned buff_nb = 0;

    while (priv->ctl->running)
    {
        fseek(file, 0, SEEK_SET);

        for (int i = 0; i < NB_FRAMES; i++)
        {

            if (!priv->ctl->running)
            {
                break;
            }

            rt_alarm_start(&priv->missed_deadline, period_in_ns + 1000, TM_INFINITE);

            // Copy the data from the file to a buffer
            unsigned read = fread(priv->buffer + (IMAGE_SIZE * (buff_nb % NB_VIDEO_BUFFERS)),
                                  IMAGE_SIZE, 1, file);
            if (!read)
            {
                break;
            }

            buff_nb++;

            rt_event_signal(&priv->event, VIDEO_PROCESSING_FLAG);
            rt_task_wait_period(NULL);
            rt_alarm_stop(&priv->missed_deadline);
        }
    }

    fclose(file);

    rt_printf("Terminating video task.\n");
}

void video_processing_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;
    struct img_1D_t src;
    struct img_1D_t dst;

    src.width = dst.width = WIDTH;
    src.height = dst.height = HEIGHT;
    src.components = dst.components = BYTES_PER_PIXEL;
    dst.data = priv->output;

    unsigned buff_nb = 0;

    while (priv->ctl->running)
    {

        rt_event_clear(&priv->event, VIDEO_PROCESSING_FLAG, NULL);

        int ret = rt_event_wait(&priv->event, VIDEO_PROCESSING_FLAG, NULL, EV_ALL, EVENT_TIMEOUT);
        if (ret == -ETIMEDOUT)
        {
            continue;
        }

        src.data = priv->buffer + (IMAGE_SIZE * (buff_nb++ % NB_VIDEO_BUFFERS));

        rgba_to_grayscale32(&src, &dst);

        // Copy the data from the buffer to the video buffer
        memcpy(get_video_buffer(), dst.data, IMAGE_SIZE);
    }
}