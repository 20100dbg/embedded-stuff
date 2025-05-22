#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ztimer.h"
#include "stdio_base.h"


int main(void)
{
    stdio_init();

    size_t max_len = 240;
    char msg[max_len];

    //stdio_write("hello from board\n", 17);
    //stdio_write("this is a very very very very long string, because i feel like it", 66);

    char xxx[] = {0x01, 0x02, 0x03 };
    stdio_write(xxx, 3);

    while (1) {
        
        //checking for input data
        if (stdio_available()) {
            
            //read
            ssize_t count = stdio_read(msg, max_len);
            
            //we need to append null byte to terminate
            //msg[count++] = '\x00';

            stdio_write(msg, count);
        }
        
        //absolutely NO sleep because we could (and will) miss some chars
        //ztimer_sleep(ZTIMER_MSEC, 1);
    }
    return 0;
}
