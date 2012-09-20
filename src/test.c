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
#include <assert.h>
#include <string.h>

#include "rbuf.h"

#define page_size sysconf(_SC_PAGESIZE)

int main(void)
{
    rbuf b;

    // test rounding.
    {
        assert(rbuf_init(&b, 0, BYTE) == -1);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, page_size/2, BYTE) == page_size);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, page_size/2 + 1, BYTE) == page_size);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, page_size, BYTE) == page_size);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, page_size * 2, BYTE) == page_size * 2);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, page_size * 3, BYTE) == page_size * 3);
        rbuf_destroy(&b);

        assert(rbuf_init(&b, (page_size * 3) - (page_size / 3), BYTE) == 
                page_size * 3);
        rbuf_destroy(&b);
    }

    // Test BYTE
    {
        rbuf_init(&b, page_size, BYTE);

        // fill buffer.
        for (int i = 0; i < b.size; i++) {
            rbuf_write(&b, "f", 1);
        }
        assert(rbuf_free(&b) == 0);

        // overfill buffer.
        assert(rbuf_write(&b, "f", 1) == 0);
        assert(rbuf_free(&b) == 0);

        // empty buffer.
        char c;
        for (int i = 0; i < b.size; i++) {
            rbuf_read(&b, &c, 1);
            assert(c == 'f');
        }
        assert(rbuf_count(&b) == 0);

        // read from empty buffer.
        assert(rbuf_read(&b, &c, 1) == 0);
        assert(rbuf_count(&b) == 0);

        rbuf_destroy(&b);
    }

    // Test STRING.
    {
        rbuf_init(&b, page_size, STRING);

        static char data[] = "test string";

        // fill buffer.
        while(rbuf_write(&b, data, strlen(data))); 
        assert(rbuf_free(&b) == 0);

        // overfill buffer.
        assert(rbuf_write(&b, data, strlen(data)) == 0);
        assert(rbuf_free(&b) == 0);

        // empty buffer
        char out[strlen(data) + 1];
        size_t read, old_read;
        while((read = rbuf_read(&b, out, strlen(data) + 1))) {
            old_read = read;
            assert(strcmp(out, data) <= 0);
        }

        assert(rbuf_count(&b) == 0);
        assert(out[old_read - 1] == '\0');
        assert((strlen(out) + 1) == old_read);
        assert((page_size % (strlen(data) + 1) == old_read));

        // read from empty buffer.
        assert(rbuf_read(&b, out, strlen(data) + 1) == 0);

        rbuf_destroy(&b);
    }

    // Test STREAM
    {
        rbuf_init(&b, page_size, STREAM);

        static char data[] = "test stream\n";

        // fill buffer.
        while(rbuf_write(&b, data, strlen(data)));
        assert(rbuf_free(&b) == 0);

        // overfill buffer.
        assert(rbuf_write(&b, data, strlen(data)) == 0);
        assert(rbuf_free(&b) == 0);

        // empty buffer
        char out[strlen(data)];

        while(rbuf_read(&b, out, strlen(data)) != -1) {
            assert(strncmp(out, data, strlen(out)) == 0);
        }
        assert(rbuf_count(&b) == page_size % strlen(data));

        // read from empty buffer.
        assert(rbuf_read(&b, out, strlen(data)) == -1);

        // Terminate end stream in buffer.
        assert(rbuf_count(&b) > 0);
        assert(rbuf_write(&b, data + rbuf_count(&b), 
                    strlen(data) - (page_size % strlen(data))));

        assert(rbuf_read(&b, out, strlen(data)) != -1);
        assert(strncmp(out, data, strlen(out)) == 0);

        rbuf_destroy(&b);
    }

    // All tests passed succesfully!
    return 0;
}
