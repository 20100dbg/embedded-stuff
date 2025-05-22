#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ztimer.h"
#include "stdio_base.h"

#include "periph/gpio.h"


void do_something()
{
    printf("something");
}

int main(void)
{

    int pin_button = GPIO_PIN(1, 10);
    gpio_init(pin_button, GPIO_IN_PD);

    int pin_led = GPIO_PIN(1, 3);
    gpio_write(pin_led, true);


    while (1) {
        

        button_up = gpio_read(pin_button);

        if (button_was_up && !button_up) {
            ztimer_sleep(ZTIMER_MSEC, 10);

            button_up = gpio_read(pin_button);
            if (!button_up) {
                do_something();
            }
        }

        button_was_up = button_up;    
        
        ztimer_sleep(ZTIMER_MSEC, 1);
    }
    return 0;
}
