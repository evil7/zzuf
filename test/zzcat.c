/*
 *  zzcat - various cat reimplementations for testing purposes
 *  Copyright (c) 2006, 2007 Sam Hocevar <sam@zoy.org>
 *                All Rights Reserved
 *
 *  $Id$
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include "config.h"

#define _LARGEFILE64_SOURCE /* for lseek64() */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined HAVE_UNISTD_H
#   include <unistd.h>
#endif
#if defined HAVE_SYS_MMAN_H
#   include <sys/mman.h>
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static inline unsigned int myrand(void)
{
    static int seed = 1;
    int x, y;
    x = (seed + 0x12345678) << 11;
    y = (seed + 0xfedcba98) >> 21;
    seed = x * 1010101 + y * 343434;
    return seed;
}

int main(int argc, char *argv[])
{
    int64_t len;
    unsigned char *data;
    char const *name;
    FILE *stream;
    int i, j, fd;

    if(argc != 3)
        return EXIT_FAILURE;

    name = argv[2];

    /* Read the whole file */
    fd = open(name, O_RDONLY);
    if(fd < 0)
        return EXIT_FAILURE;
    len = lseek(fd, 0, SEEK_END);
    if(len < 0)
        return EXIT_FAILURE;
    data = malloc(len + 16); /* 16 safety bytes */
    lseek(fd, 0, SEEK_SET);
    read(fd, data, len);
    close(fd);

    /* Read shit here and there, using different methods */
    switch(atoi(argv[1]))
    {
    case 1: /* socket seeks and reads */
        fd = open(name, O_RDONLY);
        if(fd < 0)
            return EXIT_FAILURE;
        for(i = 0; i < 128; i++)
        {
            lseek(fd, myrand() % len, SEEK_SET);
            for(j = 0; j < 4; j++)
                read(fd, data + lseek(fd, 0, SEEK_CUR), myrand() % 4096);
#ifdef HAVE_LSEEK64
            lseek64(fd, myrand() % len, SEEK_SET);
            for(j = 0; j < 4; j++)
                read(fd, data + lseek(fd, 0, SEEK_CUR), myrand() % 4096);
#endif
        }
        close(fd);
        break;
    case 2: /* std streams seeks and reads */
        stream = fopen(name, "r");
        if(!stream)
            return EXIT_FAILURE;
        for(i = 0; i < 128; i++)
        {
            long int now;
            fseek(stream, myrand() % len, SEEK_SET);
            for(j = 0; j < 4; j++)
                fread(data + ftell(stream), myrand() % 4096, 1, stream);
            fseek(stream, myrand() % len, SEEK_SET);
            now = ftell(stream);
            for(j = 0; j < 16; j++)
                data[now + j] = getc(stream);
            now = ftell(stream);
            for(j = 0; j < 16; j++)
                data[now + j] = fgetc(stream);
        }
        fclose(stream);
        break;
    case 3: /* mmap() */
        fd = open(name, O_RDONLY);
        if(fd < 0)
            return EXIT_FAILURE;
#ifdef HAVE_MMAP
        for(i = 0; i < 128; i++)
        {
            char *map;
            int moff, mlen, pgsz = len + 1;
#ifdef HAVE_GETPAGESIZE
            pgsz = getpagesize();
#endif
            moff = len < pgsz ? 0 : (myrand() % (len / pgsz)) * pgsz;
            mlen = 1 + (myrand() % (len - moff));
            map = mmap(NULL, mlen, PROT_READ, MAP_PRIVATE, fd, moff);
            if(map == MAP_FAILED)
                return EXIT_FAILURE;
            for(j = 0; j < 128; j++)
            {
                int x = myrand() % mlen;
                data[moff + x] = map[x];
            }
            munmap(map, mlen);
        }
#endif
        close(fd);
        break;
    default:
        return EXIT_FAILURE;
    }

    /* Write what we have read */
    fwrite(data, len, 1, stdout);
    free(data);

    return EXIT_SUCCESS;
}
