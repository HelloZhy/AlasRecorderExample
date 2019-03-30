#include "recorder.h"

void record(int time_ms, char* pbuf){
    long loops;
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    char* ptmpbuf;

    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, HW,
                        SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr,
                "unable to open pcm device: %s\n",
                snd_strerror(rc));
		return;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    /* CH channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, CH);

    /* FS bits/second sampling rate */
    val = FS;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    /* Set period size to FRAME frames. */
    frames = FRAME;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr,
                "unable to set hw parameters: %s\n",
                snd_strerror(rc));
		return;
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * CH * (DSIZE/8); /* DSIZE/8 bytes/sample, CH channels */
    ptmpbuf = (char *) malloc(size);

    /* Calculate loops */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);//return val(ns)
    loops = time_ms*1000/val;

	/* Allocate memory for pbuf*/
	pbuf = new char[size * loops];

    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(handle, ptmpbuf, frames);
        if (rc == -EPIPE) {
          /* EPIPE means overrun */
          fprintf(stderr, "overrun occurred\n");
          //prepared again
		  snd_pcm_prepare(handle);
        } else if (rc < 0) {
          fprintf(stderr,
                  "error from read: %s\n",
                  snd_strerror(rc));
        } else if (rc != (int)frames) {
          fprintf(stderr, "short read, read %d frames\n", rc);
        }
		printf("[%d]\tbuffer[0]: %d, buffer frames: %d\n",loops, *ptmpbuf, frames);
		memcpy(pbuf, ptmpbuf, size);
		pbuf += size;
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(ptmpbuf);
}
