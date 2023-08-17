#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <rtl-sdr.h>
#include "sdrbuffer.h"
#define DUMMYBYTES 262144

int main(int argc, char **argv)
{
    uint32_t devcount, index, setfreq, setrate, setgain;
    char manufact[256], product[256], serial[256], option, fname[20] = "datartl_";
    int res, nread;
    static rtlsdr_dev_t *dev = NULL;
    struct sdrbuf *rtlbuffer = malloc(sizeof(struct sdrbuf));
    struct timespec acqtime;

    index = 0; //default
    setfreq = 100000000; //default
    setrate = 2000000; //default
    setgain = 192; //default

    while ((option = getopt (argc, argv, "d:f:s:g:")) != -1)
    {
        switch (option)
        {
            case 'd':
                index = (uint32_t) strtol(optarg, NULL, 10);
                break;
            case 'f':
                setfreq = (uint32_t) strtol(optarg, NULL, 10);
                break;
            case 's':
                setrate = (uint32_t) strtol(optarg, NULL, 10);
                break;         
            case 'g':
                setgain = (uint32_t) strtol(optarg, NULL, 10);
                break;           
            default:
                fprintf(stderr, "Can't decipher the arguments, exiting ....\n");
                exit(EXIT_FAILURE);        
        }
    }

    devcount = rtlsdr_get_device_count();
    if (devcount<1)
    {
        fprintf(stderr, "No devices found ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "\nFound %d device(s) and attempting to open device indexed %d\n", devcount, index);

    res = rtlsdr_open(&dev, index);
    if (res != 0)
    {
        fprintf(stderr, "The selected device could not be opened ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }

    res = rtlsdr_get_device_usb_strings(index, manufact, product, serial);
    rtlbuffer->serial_no = (uint32_t) strtol(serial, NULL, 10);
    fprintf(stdout, "Manufacturer : %s\n", manufact);
    fprintf(stdout, "Product : %s\n", product);
    fprintf(stdout, "Device serial Number is : %d\n", rtlbuffer->serial_no);

    res = rtlsdr_set_sample_rate(dev, setrate);
    if (res != 0)
    {
        fprintf(stderr, "Could not set the requested sample rate ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }
    rtlbuffer->srate = rtlsdr_get_sample_rate(dev);
    fprintf(stdout, "Sample rate achieved is %d Hz\n",  rtlbuffer->srate);

    res = rtlsdr_set_center_freq(dev, setfreq);
    if (res != 0)
    {
        fprintf(stderr, "Could not tune to requested Freq ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }
    rtlbuffer->cfreq = rtlsdr_get_center_freq(dev);
    fprintf(stdout, "Tuned to %d Hz\n", rtlbuffer->cfreq);
    
    res = rtlsdr_set_tuner_gain(dev, setgain);
    if (res != 0)
    {
        fprintf(stderr, "Could not set the requested tuner gain ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Set gain to %d 10th dB\n", setgain);

    res = rtlsdr_reset_buffer(dev);
    usleep(5000);
    uint8_t dummybuffer[DUMMYBYTES];
    res = rtlsdr_read_sync(dev, dummybuffer, DUMMYBYTES, &nread); // Dummy read    
    if (nread != DUMMYBYTES) 
    {
        fprintf(stderr, "Error, can't read reliably ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }

    res = rtlsdr_reset_buffer(dev);
    assert(res==0);

    rtlbuffer->bufsize = (uint32_t) BUFSIZE;
    clock_gettime(CLOCK_REALTIME, &acqtime);
    rtlbuffer->tv_sec  = (uint32_t)acqtime.tv_sec;
    rtlbuffer->tv_nsec = (uint32_t)acqtime.tv_nsec;

    for (int i=0; i < (int)(BUFSIZE/BLOCKSIZE); i++)
    {
        res = rtlsdr_read_sync(dev, (rtlbuffer->buffer + (i*BLOCKSIZE)), BLOCKSIZE, &nread);
        // assert(res==0);
    }

    fprintf(stdout, "Acquired data at %d seconds and %d nsecs \n",  rtlbuffer->tv_sec,  rtlbuffer->tv_nsec);
    rtlsdr_close(dev);

    strcat(fname, serial);
    strcat(fname, ".bin");
    fprintf(stdout, "Writing data into a binary file %s\n", fname);
    FILE * binfile= fopen(fname, "wb");
    if (binfile != NULL) 
    {
        fwrite(rtlbuffer, sizeof(struct sdrbuf), 1, binfile); 
        fclose(binfile);
    }

    free (rtlbuffer);
    fprintf(stdout, "Exiting ! \n");
    return 0;
}
