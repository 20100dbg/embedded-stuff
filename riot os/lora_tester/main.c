#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/lora.h"
#include "net/netdev/lora.h"
#include "sx126x_params.h"
#include "sx126x_netdev.h"
#include "ztimer.h"
#include "periph/gpio.h"
#include "stdio_base.h"
//#include "shell.h"


#define SX126X_MSG_QUEUE        (8U)
#define SX126X_STACKSIZE        (THREAD_STACKSIZE_DEFAULT)
#define SX126X_MSG_TYPE_ISR     (0x3456)
#define SX126X_MAX_PAYLOAD_LEN  (128U)

static char stack[SX126X_STACKSIZE];
static kernel_pid_t _recv_pid;
static char message[SX126X_MAX_PAYLOAD_LEN];
static sx126x_t sx126x;


struct s_config {
    uint32_t frequency;
    uint16_t bandwith;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    bool integrity_check;
    uint16_t preamble;
    uint8_t payload_length;
    int8_t tx_power;
} config;

static void receive_callback(char *message, unsigned int len, int rssi, int snr,
                             long unsigned int toa)
{
    //stdio_write(message, len);

/*    
    printf("Received: \"%s\" (%" PRIuSIZE " bytes) - [RSSI: %i, SNR: %i, TOA: %" PRIu32 "ms]\n",
           message, len, rssi, snr, toa);
*/  
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
        msg_t msg;
        msg.type = SX126X_MSG_TYPE_ISR;
        if (msg_send(&msg, _recv_pid) <= 0) {
            puts("sx126x_netdev: possibly lost interrupt.");
        }
    }
    else if (event == NETDEV_EVENT_RX_COMPLETE)
    {
        size_t len = dev->driver->recv(dev, NULL, 0, 0);
        netdev_lora_rx_info_t packet_info;
        dev->driver->recv(dev, message, len, &packet_info);
        receive_callback(message, len, packet_info.rssi, packet_info.snr,
                         sx126x_get_lora_time_on_air_in_ms(&sx126x.pkt_params,&sx126x.mod_params));
        netopt_state_t state = NETOPT_STATE_RX;
        dev->driver->set(dev, NETOPT_STATE, &state, sizeof(state));
    }
}

void *_recv_thread(void *arg)
{
    netdev_t *netdev = arg;
    static msg_t _msg_queue[SX126X_MSG_QUEUE];
    msg_init_queue(_msg_queue, SX126X_MSG_QUEUE);

    while (1) {
        msg_t msg;
        msg_receive(&msg);
        if (msg.type == SX126X_MSG_TYPE_ISR) {
            netdev->driver->isr(netdev);
        }
    }
}

static int read_config(netdev_t *netdev)
{
    netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &config.frequency, sizeof(uint32_t));
    uint8_t bw;
    netdev->driver->get(netdev, NETOPT_BANDWIDTH, &bw, sizeof(uint8_t));
    switch (bw) {
        case LORA_BW_125_KHZ: config.bandwith = 125; break;
        case LORA_BW_250_KHZ: config.bandwith = 250; break;
        case LORA_BW_500_KHZ: config.bandwith = 500; break;
        default: config.bandwith = 125; break;
    }
    netdev->driver->get(netdev, NETOPT_SPREADING_FACTOR, &config.spreading_factor, sizeof(uint8_t));
    netdev->driver->get(netdev, NETOPT_CODING_RATE, &config.coding_rate, sizeof(uint8_t));
    netopt_enable_t crc;
    netdev->driver->get(netdev, NETOPT_INTEGRITY_CHECK, &crc, sizeof(netopt_enable_t));
    config.integrity_check = (crc == NETOPT_ENABLE);
    netdev->driver->get(netdev, NETOPT_PREAMBLE_LENGTH, &config.preamble, sizeof(uint16_t));
    netdev->driver->get(netdev, NETOPT_PDU_SIZE, &config.payload_length, sizeof(uint16_t));
    netdev->driver->get(netdev, NETOPT_TX_POWER, &config.tx_power, sizeof(int8_t));
    return 0;
}

static int write_config(netdev_t *netdev)
{
    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &config.frequency, sizeof(uint32_t));
    uint8_t bw_val = 0;
    switch (config.bandwith) {
        case 125: bw_val = LORA_BW_125_KHZ; break;
        case 250: bw_val = LORA_BW_250_KHZ; break;
        case 500: bw_val = LORA_BW_500_KHZ; break;
        default: bw_val = LORA_BW_125_KHZ; break;
    }
    netdev->driver->set(netdev, NETOPT_BANDWIDTH, &bw_val, sizeof(uint8_t));
    netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &config.spreading_factor, sizeof(uint8_t));
    netdev->driver->set(netdev, NETOPT_CODING_RATE, &config.coding_rate, sizeof(uint8_t));
    netopt_enable_t enable = config.integrity_check ? NETOPT_ENABLE : NETOPT_DISABLE;
    netdev->driver->set(netdev, NETOPT_INTEGRITY_CHECK, &enable, sizeof(netopt_enable_t));
    netdev->driver->set(netdev, NETOPT_PREAMBLE_LENGTH, &config.preamble, sizeof(uint16_t));
    netdev->driver->set(netdev, NETOPT_PDU_SIZE, &config.payload_length, sizeof(uint16_t));
    netdev->driver->set(netdev, NETOPT_TX_POWER, &config.tx_power, sizeof(int8_t));
    return 0;
}


int send(netdev_t *netdev, char *data, int count)
{
    netopt_state_t state = NETOPT_STATE_STANDBY;
    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));
    ztimer_sleep(ZTIMER_MSEC, 50); // ne pas descendre en dessous de 20

    iolist_t iolist = {
        .iol_base = data,
        .iol_len = count
    };

    if (netdev->driver->send(netdev, &iolist) == -ENOTSUP) {
        return -1;
    }

    ztimer_sleep(ZTIMER_MSEC, 50); // ne pas descendre en dessous de 20
    state = NETOPT_STATE_IDLE;
    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));

    return 0;
}


void handle_config(netdev_t *netdev, char *msg, ssize_t count) {

    const char *separators = ";=";
    char *param = strtok(msg, separators);

    while (param != NULL) {

        char *param_str = strtok(NULL, separators);
        int value = atoi(param_str);
        printf("%s / %d\n", param, value);

        if (strcmp("#frq", param) == 0) {
            config.frequency = value * 1000000;
        }
        else if (strcmp("bw", param) == 0) {
            config.bandwith = value;
        }
        else if (strcmp("sf", param) == 0) {
            config.spreading_factor = value;
        }
        else if (strcmp("cr", param) == 0) {
            config.coding_rate = value;
        }
        else if (strcmp("tx", param) == 0) {
            config.tx_power = value;
        }

        param = strtok(NULL, separators);
    }

    write_config(netdev);
}

void handle_config2(netdev_t *netdev, char *msg, ssize_t count) {
    printf("msg=%s\n", msg);
}


static void print_config()
{
    printf("---------------\n");
    printf("frequency=%ld\n", config.frequency);
    printf("bandwith=%d\n", config.bandwith);
    printf("spreading_factor=%d\n", config.spreading_factor);
    printf("coding_rate=%d\n", config.coding_rate);
    //printf("integrity_check=%s\n", config.integrity_check ? "enabled":"disabled");
    //printf("preamble=%d\n", config.preamble);
    //printf("payload_length=%d\n", config.payload_length);
    printf("tx_power=%d\n", config.tx_power);
    printf("---------------\n");
}

/*
int read_stdio(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    return 0;
}

static const shell_command_t shell_commands[] = {
    { "", "", read_stdio },
    { NULL, NULL, NULL }
};
*/

int main(void)
{
    stdio_init();
    sx126x_setup(&sx126x, &sx126x_params[0], 0);
    netdev_t *netdev = &sx126x.netdev;
    netdev->driver = &sx126x_driver;
    netdev->event_callback = _event_cb;
    

    if (netdev->driver->init(netdev) < 0) {
        return 1;
    }

    config.frequency = 433000000; 
    config.bandwith = 125;
    config.spreading_factor = 7;
    config.coding_rate = 1;
    config.tx_power = 10;

    config.integrity_check = true;
    config.preamble = 10;
    config.payload_length = 32;

    write_config(netdev);

    netopt_state_t state = NETOPT_STATE_IDLE;
    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));
    _recv_pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1, 0, _recv_thread, netdev, "recv_thread");

    if (_recv_pid <= KERNEL_PID_UNDEF) {
        return 1;
    }

    size_t max_len = 240;
    char msg[max_len];

    //char msg[] = {0x23, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 };
    //ssize_t count2 = 9;

/*
    int uart_dev = 0;
    uint32_t uart_baud = 115200;
    soft_uart_init(uart_dev, uart_baud, rx_cb, (void *)(intptr_t)uart_dev);
*/

    puts("hello");
/*
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
*/

    while (1) {
        
        if (stdio_available()) {
            ztimer_sleep(ZTIMER_MSEC, 10);

            ssize_t count = stdio_read(msg, max_len);

            printf("%s\n", msg);

            for (int i = 0; i < count; ++i) {
                printf("%c", msg[i]);
            }

            if (msg[0] == '#') {
                handle_config(netdev, msg, count);

                read_config(netdev);
                print_config();
            }
            else {
                send(netdev, msg, count);
                puts("sending");
            }
        }
        
        
        //handle_config2(netdev, msg, count2);
        
        ztimer_sleep(ZTIMER_MSEC, 1);
    }
    return 0;
}
