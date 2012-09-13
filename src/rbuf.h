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

size_t rbuf_init(rbuf *buffer, size_t size, enum buf_type type);
int rbuf_destroy(rbuf *buffer);

#define rbuf_count(b)    (b->write_count - b->read_count)
#define rbuf_free(b)     (b->size - rbuf_count(b))
#define rbuf_isEmpty(b)  (rbuf_count(b) == 0)
#define rbuf_isFull(b)   (rbuf_count(b) == b->size)
#define rbuf_writePtr(b) (b->p + (b->write_count % b->size))
#define rbuf_readPtr(b)  (b->p + (b->read_count % b->size))

size_t rbuf_write(rbuf *buffer, void *in, size_t length);
int rbuf_read(rbuf *buffer, void *out, size_t length);
#endif /* RBUF_H */
