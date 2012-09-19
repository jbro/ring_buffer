#include <assert.h>

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

    // Test reading and writing.
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

    // Test STRING
    {

    }

    // Test STREAM
    {

    }

    // All tests passed succesfully!
    return 0;
}
