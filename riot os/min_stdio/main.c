#include <stdio.h>
#include "ztimer.h"
#include "stdio_base.h"


int main(void)
{
    stdio_init();

    size_t max_size = 256;
    char buffer[max_size];
    size_t current_size = 0;
    uint32_t timestamp = 0;

    while (1) {

        while (stdio_available()) {

            //read byte after byte, store it in buffer[]
            char c;
            ssize_t count = stdio_read(&c, 1);
            buffer[current_size] = c;
            current_size += count;

            timestamp = ztimer_now(ZTIMER_MSEC);
        }

        //no new data since 10ms, let's do something with it
        if (current_size > 0 && ztimer_now(ZTIMER_MSEC) - timestamp > 10) {
            stdio_write(buffer, current_size);

            //reset buffer
            current_size = 0;
        }

    }
    return 0;
}
