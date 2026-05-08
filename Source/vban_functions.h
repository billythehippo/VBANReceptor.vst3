#ifndef VBAN_FUNCTIONS_H
#define VBAN_FUNCTIONS_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <arpa/inet.h>
//#include <errno.h>
//#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>

#include "vban.h"
#include "ringbuffer.h"


static inline uint16_t int16betole(u_int16_t input)
{
    return ((((uint8_t*)&input)[0])<<8) + ((uint8_t*)&input)[1];
}


static inline uint32_t int32betole(uint32_t input)
{
    return (((((((uint8_t*)&input)[0]<<8)+((uint8_t*)&input)[1])<<8)+((uint8_t*)&input)[2])<<8)+((uint8_t*)&input)[3];
}


static inline void vban_inc_nuFrame(VBanHeader* header)
{
    header->nuFrame++;
}


static inline int vban_sample_convert(void* dstptr, uint8_t format_bit_dst, void* srcptr, uint8_t format_bit_src, int num)
{
    int ret = 0;
    
    union
    {
        int32_t tmp;
        int8_t tmp_bytes[4];
    };
    
    uint8_t* dptr;
    uint8_t* sptr;

    int dst_sample_size;
    int src_sample_size;

    if (format_bit_dst==format_bit_src)
    {
        memcpy(dstptr, srcptr, VBanBitResolutionSize[format_bit_dst]*num);
        return 0;
    }

    dptr = (uint8_t*)dstptr;
    sptr = (uint8_t*)srcptr;

    dst_sample_size = VBanBitResolutionSize[format_bit_dst];
    src_sample_size = VBanBitResolutionSize[format_bit_src];
    //fprintf(stderr, "%d %d\r\n", dst_sample_size, src_sample_size);

    switch (format_bit_dst)
    {
    case VBAN_BITFMT_8_INT:
        switch (format_bit_src)
        {
        case VBAN_BITFMT_16_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_24_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[sample*dst_sample_size] = sptr[2 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[sample*dst_sample_size] = sptr[3 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_FLOAT:
            for (int sample=0; sample<num; sample++)
            {
                tmp = (int32_t)roundf((float)(1<<31)*((float*)sptr)[sample]);
                dptr[0 + sample*dst_sample_size] = tmp_bytes[3];
            }
            break;
        default:
            fprintf(stderr, "Convert Error! Unsuppotred source format!%d\n", format_bit_src);
            ret = 1;
        }
        break;
    case VBAN_BITFMT_16_INT:
        switch (format_bit_src)
        {
        case VBAN_BITFMT_8_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_24_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
                dptr[1 + sample*dst_sample_size] = sptr[2 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = sptr[2 + sample*src_sample_size];
                dptr[1 + sample*dst_sample_size] = sptr[3 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_FLOAT:
            for (int sample=0; sample<num; sample++)
            {
                tmp = (int32_t)roundf((float)(1<<31)*((float*)sptr)[sample]);
                dptr[0 + sample*dst_sample_size] = tmp_bytes[2];
                dptr[1 + sample*dst_sample_size] = tmp_bytes[3];
            }
            break;
        default:
            fprintf(stderr, "Convert Error! Unsuppotred source format!%d\n", format_bit_src);
            ret = 1;
        }
        break;
    case VBAN_BITFMT_24_INT:
        switch (format_bit_src)
        {
        case VBAN_BITFMT_8_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = 0;
                dptr[2 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_16_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
                dptr[2 + sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
                dptr[1 + sample*dst_sample_size] = sptr[2 + sample*src_sample_size];
                dptr[2 + sample*dst_sample_size] = sptr[3 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_FLOAT:
            for (int sample=0; sample<num; sample++)
            {
                tmp = (int32_t)roundf((float)(1<<31)*((float*)sptr)[sample]);
                dptr[0 + sample*dst_sample_size] = tmp_bytes[1];
                dptr[1 + sample*dst_sample_size] = tmp_bytes[2];
                dptr[2 + sample*dst_sample_size] = tmp_bytes[3];
                //memcpy(&dptr[sample*dst_sample_size], &tmp_bytes[1], 3);
                //((int32_t*)dptr)[sample*dst_sample_size] = (int32_t)roundf((float)(1<<23)*((float*)sptr)[sample]);
            }
            break;
        default:
            fprintf(stderr, "Convert Error! Unsuppotred source format!%d\n", format_bit_src);
            ret = 1;
        }
        break;
    case VBAN_BITFMT_32_INT:
        switch (format_bit_src)
        {
        case VBAN_BITFMT_8_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = 0;
                dptr[2 + sample*dst_sample_size] = 0;
                dptr[3 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_16_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = 0;
                dptr[2 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
                dptr[3 + sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_24_INT:
            for (int sample=0; sample<num; sample++)
            {
                dptr[0 + sample*dst_sample_size] = 0;
                dptr[1 + sample*dst_sample_size] = sptr[0 + sample*src_sample_size];
                dptr[2 + sample*dst_sample_size] = sptr[1 + sample*src_sample_size];
                dptr[3 + sample*dst_sample_size] = sptr[2 + sample*src_sample_size];
            }
            break;
        case VBAN_BITFMT_32_FLOAT:
            for (int sample=0; sample<num; sample++)
            {
                tmp = (int32_t)roundf((float)(1<<31)*((float*)sptr)[sample]);
                dptr[0 + sample*dst_sample_size] = tmp_bytes[0];
                dptr[0 + sample*dst_sample_size] = tmp_bytes[1];
                dptr[1 + sample*dst_sample_size] = tmp_bytes[2];
                dptr[2 + sample*dst_sample_size] = tmp_bytes[3];
            }
            break;
        default:
            fprintf(stderr, "Convert Error! Unsuppotred source format!%d\n", format_bit_src);
            ret = 1;
        }
        break;
    case VBAN_BITFMT_32_FLOAT:
        switch (format_bit_src)
        {
        case VBAN_BITFMT_8_INT:
            for (int sample=0; sample<num; sample++)
            {
                ((float*)dptr)[sample] = (float)sptr[sample]/(float)(1<<7);
            }
            break;
        case VBAN_BITFMT_16_INT:
            for (int sample=0; sample<num; sample++)
            {
                ((float*)dptr)[sample] = (float)(((int16_t*)sptr)[sample])/(float)(1<<15);
            }
            break;
        case VBAN_BITFMT_24_INT:
            for (int sample=0; sample<num; sample++)
            {
                ((float*)dptr)[sample] = (float)((((int8_t)sptr[2 + sample*src_sample_size])<<16)+(sptr[1 + sample*src_sample_size]<<8)+sptr[0 + sample*src_sample_size])/(float)(1<<23);
            }
            break;
        case VBAN_BITFMT_32_INT:
            for (int sample=0; sample<num; sample++)
            {
                ((float*)dptr)[sample] = (float)(((int32_t*)sptr)[sample])/(float)(1<<31);
            }
            break;
        default:
            fprintf(stderr, "Convert Error! Unsuppotred source format!%d\n", format_bit_src);
            ret = 1;
        }
        break;
    default:
        fprintf(stderr, "Convert Error! Unsuppotred destination format!%d\n", format_bit_dst);
        ret = 1;
    }
    return ret;
}


static inline int vban_read_frame_from_ringbuffer(float* dst, ringbuffer_t* ringbuffer, int num)
{
    size_t size = num*sizeof(float);
    if (ringbuffer_read_space(ringbuffer)>=size)
    {
        ringbuffer_read(ringbuffer, (char*)dst, size);
        return 0;
    }
    return 1;
}


static inline int vban_add_frame_from_ringbuffer(float* dst, ringbuffer_t* ringbuffer, int num)
{
    float fsamples[256];
    size_t size = num*sizeof(float);
    if (ringbuffer_read_space(ringbuffer)>=size)
    {
        ringbuffer_read(ringbuffer, (char*)fsamples, size);
        for (int i=0; i<num; i++) dst[i] = (dst[i] + fsamples[i])/2;
        return 0;
    }
    return 1;
}


static inline int vban_get_format_SR(long host_samplerate)
{
    int i;
    for (i=0; i<VBAN_SR_MAXNUMBER; i++) if (host_samplerate==VBanSRList[i]) return i;
    return -1;
}


static inline uint vban_strip_vban_packet(uint8_t format_bit, uint16_t nbchannels)
{
    uint framesize = VBanBitResolutionSize[format_bit]*nbchannels;
    uint nframes = VBAN_DATA_MAX_SIZE/framesize;
    if (nframes>VBAN_SAMPLES_MAX_NB) nframes = VBAN_SAMPLES_MAX_NB;
    return nframes*framesize;
}


static inline uint vban_strip_vban_data(uint datasize, uint8_t format_bit, uint16_t nbchannels)
{
    uint framesize = VBanBitResolutionSize[format_bit]*nbchannels;
    uint nframes = datasize/framesize;
    if (nframes>VBAN_SAMPLES_MAX_NB) nframes = VBAN_SAMPLES_MAX_NB;
    return nframes*framesize;
}


static inline uint vban_calc_nbs(uint datasize, uint8_t resolution, uint16_t nbchannels)
{
    return datasize/VBanBitResolutionSize[resolution]*nbchannels;
}


static inline uint vban_packet_to_float_buffer(uint pktdatalen, uint8_t resolution)
{
    return sizeof(float)*pktdatalen/VBanBitResolutionSize[resolution];
}


static inline int file_exists(const char* __restrict filename)
{
    if (access(filename, F_OK)==0) return 1;
    return 0;
}


#endif
