// some helper functions based on Mastering Algorithms with C: Useful Techniques from Sorting to Encryption
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "CycleTimer.h"

#include "compress.h"

int bit_get(const unsigned char *bits, int pos)
{
    unsigned char  mask;
    int            i;

    mask = 0x80;
    for(i = 0; i < (pos % 8); i++)
        mask = mask >> 1;
    return (((mask & bits[(int)(pos / 8)]) == mask) ? 1 : 0);
}

void bit_set(unsigned char *bits, int pos, int state)
{
    unsigned char mask;
    int           i;

    mask = 0x80;
    for(i = 0; i < (pos % 8); i++)
        mask=mask>>1;

    if(state)
        bits[pos / 8] = bits[pos / 8] | mask;
    else
        bits[pos / 8] = bits[pos / 8] & (~mask);

    return;
}

static int compare_win(const unsigned char *window, const unsigned char *buffer,
                       int *offset, unsigned char *next)
{
    int match,longest,i,j,k;

    *offset = 0;

    longest = 0;
    *next = buffer[0];

    for(k=0; k<LZ77_WINDOW_SIZE; k++)
    {
        i = k;
        j = 0;
        match = 0;

        while(i<LZ77_WINDOW_SIZE && j<LZ77_BUFFER_SIZE - 1)
        {
            if(window[i] != buffer[j])
                break;

            match++;
            i++;
            j++;
        }

        if(match > longest)
        {
            *offset = k;
            longest = match;
            *next = buffer[j];
        }
    }
    return longest;
}

int lz77_compress(const unsigned char *original,unsigned char **compressed,int size)
{
    unsigned char     window[LZ77_WINDOW_SIZE],
                      buffer[LZ77_BUFFER_SIZE],
                      *comp,
                      *temp,
                      next;
    int               offset,
                      length,
                      remaining,
                      hsize,
                      ipos,
                      opos,
                      i;
    *compressed = NULL;

    hsize = sizeof(int);
    if((comp = (unsigned char *)malloc(hsize)) == NULL)
        return -1;
    memcpy(comp,&size,sizeof(int));

    memset(window, 0 , LZ77_WINDOW_SIZE);
    memset(buffer, 0 , LZ77_BUFFER_SIZE);

    ipos = 0;

    for(i=0; i<LZ77_BUFFER_SIZE && ipos < size; i++)
    {
        buffer[i] = original[ipos];
        ipos++;
    }

    opos = hsize * 8;
    remaining = size;

    while(remaining > 0)
    {
        unsigned int token;
        int tbits;

        if((length = compare_win(window,buffer,&offset,&next)) != 0)
        {
            token = 0x00000001 << (LZ77_PHRASE_BITS - 1);

            token = token | (offset << (LZ77_PHRASE_BITS - LZ77_TYPE_BITS - LZ77_WINOFF_BITS));

            token = token | (length << (LZ77_PHRASE_BITS - LZ77_TYPE_BITS - LZ77_WINOFF_BITS - LZ77_BUFLEN_BITS));

            token = token | next;

            tbits = LZ77_PHRASE_BITS;
        }
        else
        {
            token = 0x00000000;

            token = token | next;

            tbits = LZ77_SYMBOL_BITS;
        }

        token = htonl(token);

        for(i=0; i<tbits; i++)
        {
            if(opos % 8 == 0)
            {
                if((temp = (unsigned char *)realloc(comp,(opos / 8) + 1)) == NULL)
                {
                    free(comp);
                    return -1;
                }
                comp = temp;
            }

            int tpos = (sizeof(unsigned int ) * 8) - tbits + i;
            bit_set(comp,opos,bit_get((unsigned char *)&token,tpos));
            opos++;
        }
        length++;

        memmove(&window[0],&window[length],LZ77_WINDOW_SIZE - length);
        memmove(&window[LZ77_WINDOW_SIZE - length],&buffer[0],length);
        memmove(&buffer[0],&buffer[length],LZ77_BUFFER_SIZE - length);
        for(i = LZ77_BUFFER_SIZE - length; i<LZ77_BUFFER_SIZE && ipos <size; i++)
        {
            buffer[i] = original[ipos];
            ipos++;
        }

        remaining = remaining - length;
    }
    *compressed = comp;
    return ((opos - 1) / 8) + 1;
}

int lz77_uncompress(const unsigned char *compressed,unsigned char **original)
{
    unsigned char window[LZ77_WINDOW_SIZE],
                  buffer[LZ77_BUFFER_SIZE],
                  *orig,
                  *temp,
                  next;
    int           offset,
                  length,
                  remaining,
                  hsize,
                  size,
                  ipos,
                  opos,
                  tpos,
                  state,
                  i;
    *original = orig = NULL;

    hsize = sizeof(int);
    memcpy(&size,compressed,sizeof(int));

    memset(window, 0, LZ77_WINDOW_SIZE);
    memset(buffer, 0, LZ77_BUFFER_SIZE);

    ipos = hsize * 8;
    opos = 0;
    remaining = size;

    while(remaining > 0)
    {
        state = bit_get(compressed,ipos);
        ipos++;

        if(state == 1)
        {
            memset(&offset, 0, sizeof(int));

            for(i=0; i<LZ77_WINOFF_BITS; i++)
            {
                tpos = (sizeof(int)*8) - LZ77_WINOFF_BITS + i;
                bit_set((unsigned char *)&offset, tpos, bit_get(compressed,ipos));
                ipos++;
            }

            memset(&length, 0, sizeof(int));
            for(i=0; i<LZ77_BUFLEN_BITS; i++)
            {
                tpos = (sizeof(int)*8) - LZ77_BUFLEN_BITS + i;
                bit_set((unsigned char *)&length, tpos, bit_get(compressed,ipos));
                ipos++;
            }

            next = 0x00;
            for(i=0; i<LZ77_NEXT_BITS; i++)
            {
                tpos = (sizeof(unsigned char)*8) - LZ77_NEXT_BITS + i;
                bit_set((unsigned char *)&next, tpos, bit_get(compressed,ipos));
                ipos++;
            }

            offset = ntohl(offset);
            length = ntohl(length);

            i=0;
            if(opos>0)
            {
                if((temp = (unsigned char *)realloc(orig,opos+length+1)) == NULL)
                {
                    free(orig);
                    return 1;
                }
                orig = temp;
            }
            else
            {
                if((orig = (unsigned char *)malloc(length+1)) == NULL)
                    return -1;
            }

            while(i<length && remaining>0)
            {
                orig[opos] = window[offset + i];
                opos++;
                buffer[i] = window[offset + i];
                i++;

                remaining --;
            }

            if(remaining > 0)
            {
                orig[opos] = next;
                opos++;

                buffer[i] = next;

                remaining--;
            }
            length++;
        }
        else
        {
            next = 0x00;
            for(i=0; i<LZ77_NEXT_BITS; i++)
            {
                tpos = (sizeof(unsigned char)*8) - LZ77_NEXT_BITS + i;
                bit_set((unsigned char *)&next, tpos, bit_get(compressed,ipos));
                ipos++;
            }

            if(opos > 0)
            {
                if((temp = (unsigned char*)realloc(orig,opos+1)) == NULL)
                {
                    free(orig);
                    return -1;
                }
                orig = temp;
            }
            else
            {
                if((orig = (unsigned char *)malloc(1)) == NULL)
                return -1;
            }
            orig[opos] = next;
            opos++;

            if(remaining > 0)
                buffer[0] = next;
            remaining--;
            length = 1;
        }
        memmove(&window[0], &window[length],LZ77_WINDOW_SIZE - length);
        memmove(&window[LZ77_WINDOW_SIZE - length], &buffer[0], length);
    }
    *original = orig;

    return  opos;
}

int lz77_compress_without_header(const unsigned char *original,unsigned char **compressed,int size)
{
    unsigned char     window[LZ77_WINDOW_SIZE],
                      buffer[LZ77_BUFFER_SIZE],
                      *comp,
                      *temp,
                      next;
    int               offset,
                      length,
                      remaining,
                      ipos,
                      opos,
                      i;
    *compressed = NULL;

    comp = NULL;

    memset(window, 0 , LZ77_WINDOW_SIZE);
    memset(buffer, 0 , LZ77_BUFFER_SIZE);

    ipos = 0;

    for(i=0; i<LZ77_BUFFER_SIZE && ipos < size; i++)
    {
        buffer[i] = original[ipos];
        ipos++;
    }

    opos = 0;
    remaining = size;

    while(remaining > 0)
    {
        unsigned int token;
        int tbits;

        if((length = compare_win(window,buffer,&offset,&next)) != 0)
        {
            token = 0x00000001 << (LZ77_PHRASE_BITS - 1);

            token = token | (offset << (LZ77_PHRASE_BITS - LZ77_TYPE_BITS - LZ77_WINOFF_BITS));

            token = token | (length << (LZ77_PHRASE_BITS - LZ77_TYPE_BITS - LZ77_WINOFF_BITS - LZ77_BUFLEN_BITS));

            token = token | next;

            tbits = LZ77_PHRASE_BITS;
        }
        else
        {
            token = 0x00000000;

            token = token | next;

            tbits = LZ77_SYMBOL_BITS;
        }

        token = htonl(token);

        for(i=0; i<tbits; i++)
        {
            if(opos % 8 == 0)
            {
                if((temp = (unsigned char *)realloc(comp,(opos / 8) + 1)) == NULL)
                {
                    free(comp);
                    return -1;
                }
                comp = temp;
            }

            int tpos = (sizeof(unsigned int ) * 8) - tbits + i;
            bit_set(comp,opos,bit_get((unsigned char *)&token,tpos));
            opos++;
        }
        length++;

        memmove(&window[0],&window[length],LZ77_WINDOW_SIZE - length);
        memmove(&window[LZ77_WINDOW_SIZE - length],&buffer[0],length);
        memmove(&buffer[0],&buffer[length],LZ77_BUFFER_SIZE - length);
        for(i = LZ77_BUFFER_SIZE - length; i<LZ77_BUFFER_SIZE && ipos <size; i++)
        {
            buffer[i] = original[ipos];
            ipos++;
        }

        remaining = remaining - length;
    }
    *compressed = comp;
    return ((opos - 1) / 8) + 1;
}

int* parallel_lz77_compress(const unsigned char *original,unsigned char **compressed,int size, int num_threads) {
    int *ret = new int[num_threads];
    #pragma omp parallel for
    for (int thread_id = 0; thread_id < num_threads; thread_id++) {
        double start_time = CycleTimer::currentSeconds();
        const unsigned char * original_local = original + size / num_threads * thread_id;
        unsigned char **compressed_local = compressed + thread_id;
        int size_local = thread_id == (num_threads - 1) ? size - (size / num_threads) * (num_threads - 1) : size / num_threads;
        if (thread_id == 0) {
            ret[thread_id] = lz77_compress_without_header(original_local, compressed_local, size_local);
        } else {
            ret[thread_id] = lz77_compress_without_header(original_local, compressed_local, size_local);
        }
        double end_time = CycleTimer::currentSeconds();
        printf("Thread %d compress time: %.3f ms\n", thread_id, 1000.f * (end_time - start_time));
    }
    return ret;
}