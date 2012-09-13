/*
 * Copyright 2012 Aalborg University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Aalborg University ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL Aalborg University OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the Homeport project.
*/
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

//XXX
#include <stdio.h>

#include "rbuf.h"

#define page_size sysconf(_SC_PAGESIZE)

size_t rbuf_init(rbuf *buffer, size_t size, enum buf_type type) 
{
    size += page_size - (size % page_size);

    buffer->size = size;
    buffer->type = type;

    buffer->read_count = 0;
    buffer->write_count = 0;

    buffer->term = '\n';

#ifdef SHM
    char path[] = "/dev/shm/rb-XXXXXX";
#else
    char path[] = "/tmp/rb-XXXXXX";
#endif /* SHM */

    if((buffer->fd = mkstemp(path)) < 0)
        goto error;

    if(unlink(path))
        goto error;

    if(ftruncate(buffer->fd, buffer->size) < 0)
        goto error;

    if((buffer->p = mmap(NULL, 2 * buffer->size, PROT_NONE, 
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
        goto error;

    if(mmap(buffer->p, buffer->size, PROT_READ | PROT_WRITE, 
                MAP_FIXED | MAP_SHARED, buffer->fd, 0) == MAP_FAILED)
        goto error;

    if(mmap(buffer->p + buffer->size, buffer->size, PROT_READ | PROT_WRITE, 
                MAP_FIXED | MAP_SHARED, buffer->fd, 0) == MAP_FAILED)
        goto error;

    return size;

error:
    close(buffer->fd);
    return -1;
}

int rbuf_destroy(rbuf *buffer) 
{
    int ret;

    ftruncate(buffer->fd, 0);
    ret = munmap(buffer->p, buffer->size * 2);
    close(buffer->fd);

    return ret;
}

size_t rbuf_write(rbuf *buffer, void *in, size_t length)
{
    if(buffer->type == STRING)
        length += 1;

    if(length > rbuf_free(buffer))
        length = rbuf_free(buffer);

    memcpy(rbuf_writePtr(buffer), in, length);

    buffer->write_count += length;

    return length;
}

int rbuf_read(rbuf *buffer, void *out, size_t length) {
    void *ret;

    if(length > rbuf_count(buffer)) {
        length = rbuf_count(buffer);
    }

    switch(buffer->type) {
        case BYTE:
            memcpy(out, rbuf_readPtr(buffer), length);
            break;
        case STRING:
            ret = memccpy(out, rbuf_readPtr(buffer), '\0', length);

            if(ret == NULL) {
                ((char *) out)[length - 1] = '\0';
            }
            else {
                length = ret - out;
            }
            break;
        case STREAM:
            ret = memccpy(out, rbuf_readPtr(buffer), buffer->term, length);

            if(ret == NULL) {
                return -1;
            }
            else {
                length = ret - out;
                ((char *) out)[length - 1] = '\0';
            }
            break;
        default:
            return -1;
    }

    buffer->read_count += length;

    return 0;
}

int main(void)
{
    rbuf r;

    printf("size: %lu\n", rbuf_init(&r, 4, STRING));
    printf("writen: %lu\n", rbuf_write(&r, "test", strlen("test")));
    printf("writen: %lu\n", rbuf_write(&r, "hest", strlen("hest")));

    char out[10];
    size_t ret = rbuf_read(&r, out, 10);
    printf("read: %s, %lu\n", out, ret);
    ret = rbuf_read(&r, out, 10);
    printf("read: %s, %lu\n", out, ret);

    rbuf_destroy(&r);
}

