#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <rtl-sdr.h>
#include "sdrshmbufffer.h"

static volatile int run_var = 1;

void inter_handler(int dummy) 
{
    run_var = 0;
}

int main(int argc, char **argv)
{
    uint32_t devcount, index, setfreq, setrate, setgain;
    char manufact[256], product[256], serial[256], option, SHM_RBUFF[20] = "rtlbuff";
    int res, nread, fd, block_index;
    static rtlsdr_dev_t *dev = NULL;
    struct sdrbuf *rtlbuffer;
    struct timespec acqtime;
    uint32_t BUFFDEPTH = (uint32_t)(BUFSIZE/BLOCKSIZE);

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
    fprintf(stdout, "Manufacturer : %s\n", manufact);
    fprintf(stdout, "Product : %s\n", product);

    res = rtlsdr_set_sample_rate(dev, setrate);
    if (res != 0)
    {
        fprintf(stderr, "Could not set the requested sample rate ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }

    res = rtlsdr_set_center_freq(dev, setfreq);
    if (res != 0)
    {
        fprintf(stderr, "Could not tune to requested Freq ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }
    
    res = rtlsdr_set_tuner_gain(dev, setgain);
    if (res != 0)
    {
        fprintf(stderr, "Could not set the requested tuner gain ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }

    res = rtlsdr_reset_buffer(dev);
    usleep(5000);
    uint8_t dummybuffer[BLOCKSIZE];
    res = rtlsdr_read_sync(dev, dummybuffer, BLOCKSIZE, &nread); // Dummy read    
    if (nread != BLOCKSIZE) 
    {
        fprintf(stderr, "Error, can't read reliably ! Exiting ....\n");
        exit(EXIT_FAILURE);
    }

    res = rtlsdr_reset_buffer(dev);
    assert(res==0);

    strcat(SHM_RBUFF, serial);
    fd = shm_open(SHM_RBUFF, O_RDWR|O_CREAT, 0664);
    assert(fd!=-1);
    ftruncate(fd, BUFSIZE);
    rtlbuffer = (struct sdrbuf*) mmap(0, BUFSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    rtlbuffer->serial_no = (uint32_t) strtol(serial, NULL, 10);
    rtlbuffer->bufsize = (uint32_t) BUFSIZE;

    fprintf(stdout, "Device serial Number is : %d\n", rtlbuffer->serial_no);

    rtlbuffer->cfreq = rtlsdr_get_center_freq(dev);
    fprintf(stdout, "Tuned to %d Hz\n", rtlbuffer->cfreq);

    rtlbuffer->srate = rtlsdr_get_sample_rate(dev);
    fprintf(stdout, "Sample rate achieved is %d Hz\n",  rtlbuffer->srate);

    fprintf(stdout, "Gain set to %d 10th dB\n", setgain);

    block_index = 0;
    signal(SIGINT, inter_handler);
    while (run_var)
    { 
        clock_gettime(CLOCK_REALTIME, &acqtime);
        rtlbuffer->tv_sec  = (uint32_t)acqtime.tv_sec;
        rtlbuffer->tv_nsec = (uint32_t)acqtime.tv_nsec;
        rtlbuffer->blockindex = block_index;
        rtlsdr_read_sync(dev, (rtlbuffer->buffer + (block_index*BLOCKSIZE)), BLOCKSIZE, &nread);
        block_index = (block_index+1)%(BUFFDEPTH-1);
    }
    printf ("Signal caught, exiting the rtl2rbuff code \n");
    munmap(rtlbuffer, BUFSIZE);
    shm_unlink(SHM_RBUFF);
    close(fd);
    rtlsdr_close(dev);
    return 0;
}
