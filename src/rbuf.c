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

#include "rbuf.h"

#define page_size sysconf(_SC_PAGESIZE)

size_t rbuf_init(rbuf *buffer, size_t size, enum buf_type type) 
{
    // Round up to multiple of page size.
    size += page_size - ((size % page_size) ? (size % page_size) : page_size);

    buffer->size = size;
    buffer->type = type;

    buffer->read_count = 0;
    buffer->write_count = 0;

    // Default character for STREAMs
    buffer->term = '\n';

    // Set file path depending on whether shared memory is compiled in or not. 
#ifdef SHM
    char path[] = "/dev/shm/rb-XXXXXX";
#else
    char path[] = "/tmp/rb-XXXXXX";
#endif /* SHM */

    // Create a temporary file for mmap backing.
    if((buffer->fd = mkstemp(path)) < 0)
        goto error;

    // Remove file from filesystem. Note the file is still open by the 
    // proccess.
    // XXX there might be a security problem with this, if so, use umaks 0600.
    if(unlink(path))
        goto error;

    // Resize file to buffer size.
    if(ftruncate(buffer->fd, buffer->size) < 0)
        goto error;

    // Map twice the buffer size.
    if((buffer->p = mmap(NULL, 2 * buffer->size, PROT_NONE, 
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
        goto error;

    // Map the temporary file into the first half of the above mapped space.
    if(mmap(buffer->p, buffer->size, PROT_READ | PROT_WRITE, 
                MAP_FIXED | MAP_SHARED, buffer->fd, 0) == MAP_FAILED)
        goto error;

    // Map the temporary file into the second half of the mapped space.
    // This creates two consecutive copies of the same physical memory, thus
    // allowing contiues reads and writes of the buffer.
    if(mmap(buffer->p + buffer->size, buffer->size, PROT_READ | PROT_WRITE, 
                MAP_FIXED | MAP_SHARED, buffer->fd, 0) == MAP_FAILED)
        goto error;

    return size;

    // Error handling
error:
    // Truncate file to zero, to avoid writing back memory to file, on munmap.
    ftruncate(buffer->fd, 0);
    // Unmap the mapped virtual memmory.
    munmap(buffer->p, buffer->size * 2);
    // Close the backing file.
    close(buffer->fd);
    return -1;
}

int rbuf_destroy(rbuf *buffer) 
{
    int ret;

    // Truncate file to zero, to avoid writing back memory to file, on munmap.
    ftruncate(buffer->fd, 0);
    // Unmap the mapped virtual memmory.
    ret = munmap(buffer->p, buffer->size * 2);
    // Close the backing file.
    close(buffer->fd);

    return ret;
}

size_t rbuf_write(rbuf *buffer, void *in, size_t length)
{
    // Make room for the trailing '\0' in string buffers
    if(buffer->type == STRING)
        length += 1;

    // Make sure we don't put in more than there's room for, by writing no
    // more than there is free.
    if(length > rbuf_free(buffer))
        length = rbuf_free(buffer);

    // Copy in.
    memcpy(rbuf_writePtr(buffer), in, length);

    // Update write count
    buffer->write_count += length;

    return length;
}

int rbuf_read(rbuf *buffer, void *out, size_t length) {
    void *ret;

    // Make sure we do not read out more than there is actually in the buffer.
    if(length > rbuf_count(buffer)) {
        length = rbuf_count(buffer);
    }

    switch(buffer->type) {
        case BYTE:
            // Copy out for BYTE, nothing magic here.
            memcpy(out, rbuf_readPtr(buffer), length);
            break;
        case STRING:

            // Copy out until first '\0' byte, or length.
            ret = memccpy(out, rbuf_readPtr(buffer), '\0', length);

            if(ret == NULL) {
                // Make sure the data is '\0' terminated.
                ((char *) out)[length - 1] = '\0';
            }
            else {
                // Calculate actual read count.
                length = ret - out;
            }
            break;
        case STREAM:
            // Copy out until first buffer->term byte, or length.
            ret = memccpy(out, rbuf_readPtr(buffer), buffer->term, length);

            if(ret == NULL) {
                // If buffer->term was not found, return error.
                return -1;
            }
            else {
                // Substitute buffer->term with '\0' to create a string.
                length = ret - out;
                ((char *) out)[length - 1] = '\0';
            }
            break;
        default:
            return -1;
    }

    // Update read count.
    buffer->read_count += length;

    return length;
}

