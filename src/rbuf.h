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
#ifndef RBUF_H
#define RBUF_H

#include <unistd.h>

// Ring buffer implementation, using mmap as backend.

// There are three types of buffers:
// BYTE: is just normal ring buffer with regards to reads and writes
// STREAM: is a normal ring buffer with regards to read. But writes
//         only return until rbuf->term is seen in the buffer
// STRING: oprates on strings, so strlen should be used for size when writing.
//         When reading a string is returned, that is a series of characters
//         until and including the first '\0' in the buffer.
enum buf_type {BYTE, STREAM, STRING};

typedef struct {
    enum buf_type type;
    void *p;
    int fd;

    size_t size;
    char term;

    size_t read_count;
    size_t write_count;
} rbuf;

// Creates new ring buffer.
// PARAM:
//    rbuf, initialized pointer where the ring buffer struct will live.
//    size, requested size of the buffer, this is rounded up to the nearest
//          multiple of the page size.
//    typs, see explanation of enum.
// RETURN:
//    actual size of the buffer, after it has been rounded up.
size_t rbuf_init(rbuf *buffer, size_t size, enum buf_type type);

// Destroy ring buffer.
// PARAM:
//    rbuf, pointer to buffer struct.
// RETURN:
//    0 on success, -1 on failure, errno is set.
// NOTE:
//    Don't forget to deallocate buffer, if necessary.
int rbuf_destroy(rbuf *buffer);

// Returns number of bytes currently held by the buffer.
// NOTES:
//    this count includes '\0', if the buffer types is STRING.
#define rbuf_count(b)    ((b)->write_count - (b)->read_count)

// Returns how many bytes are availiable in the buffer.
#define rbuf_free(b)     ((b)->size - rbuf_count(b))

// Checks if the buffer is empty.
#define rbuf_isEmpty(b)  (rbuf_count(b) == 0)

// Checks if the buffer is full.
#define rbuf_isFull(b)   (rbuf_count(b) == b->size)

// Returns the current write pointer.
// NOTE:
//    Use with care, and remeber to update write count if data is inserted.
#define rbuf_writePtr(b) ((b)->p + ((b)->write_count % (b)->size))

// Returns the current read pointer.
// NOTE:
//    Use with care, and remember to update the read count if data is 
//    copied out.
#define rbuf_readPtr(b)  ((b)->p + ((b)->read_count % (b)->size))

// Write data to buffer.
// PARAM:
//    rbuf, pointer to buffer struct.
//    in, pointer to input data.
//    length, length of input data.
// RETURN:
//    The number of bytes written to the buffer, this migth be less than
//    length, if the buffer did not have enough room.
// NOTE:
//    This function does not care which type the buffer has, so you can
//    do things like: rbuffer_write(b, "test1\0test2", 11); on a buffer with,
//    type STRING, and two strings will be inserted into the buffer.
size_t rbuf_write(rbuf *buffer, void *in, size_t length);

// Read data from buffer.
// PARAM:
//    rbuf, pointer to buffer structure.
//    out, output buffer. out will be filled differently from the different 
//    buffer types. BYTE type will will copy out as many bytes as there are in
//    the buffer, up to length. STREAM will copy out until the first occurrence
//    of buffer->term, if buffer->term was not found within the length, do
//    nothing to the buffer, and return -1. STRING will copy out until the 
//    first '\0'.
//    length, number of bytes the output buffer will hold.
// RETURN:
//    Return value is the number of bytes read into out, or -1 if STREAM buffer,
//    and buffer->term was not found.
// NOTE:
//    For the buffers with type STRING the count includes the '\0' character.
//    If buffer->term was not found within length of a STREAM buffer, -1 is
//    returned, and the output buffer will be garbage.
int rbuf_read(rbuf *buffer, void *out, size_t length);
#endif /* RBUF_H */

