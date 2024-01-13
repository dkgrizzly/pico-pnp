#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"


// 7-bit address
#define MPR_ADDR 0x18
#define RELAY_ADDR 0x6D

uint8_t data_buf[4];
char gcode_buf[256];
volatile uint8_t gcode_len = 0;

void mpr_read(uint8_t bus) {
    if(bus > 1) return;

    data_buf[0] = 0xAA;
    data_buf[1] = 0x00;
    data_buf[2] = 0x00;
    if(i2c_write_blocking(bus ? i2c1 : i2c0, MPR_ADDR, data_buf, 3, false) != 3) return;
    do {
        i2c_read_blocking(bus ? i2c1 : i2c0, MPR_ADDR, data_buf, 1, false);
    } while (data_buf[0] & 0x20);
    i2c_read_blocking(bus ? i2c1 : i2c0, MPR_ADDR, data_buf, 4, false);
    printf("Sensor%d: %d\r\n", bus, (data_buf[1] << 16) | (data_buf[2] << 8) | data_buf[3]);
}

void relay_on(uint8_t bit) {
    if(bit > 3) return;

    data_buf[0] = bit + 0x5;
    if(i2c_write_blocking(i2c0, RELAY_ADDR, data_buf, 1, true) != 1) return;
    i2c_read_blocking(i2c0, RELAY_ADDR, data_buf, 1, false);
    if(!data_buf[0]) {
        data_buf[0] = bit + 0x1;
        i2c_write_blocking(i2c0, RELAY_ADDR, data_buf, 1, false);
    }
    printf("Relay%d: on\r\n", bit);
}

void relay_off(uint8_t bit) {
    if(bit > 3) return;

    data_buf[0] = bit + 0x5;
    if(i2c_write_blocking(i2c0, RELAY_ADDR, data_buf, 1, true) != 1) return;
    i2c_read_blocking(i2c0, RELAY_ADDR, data_buf, 1, false);
    if(data_buf[0]) {
        data_buf[0] = bit + 0x1;
        i2c_write_blocking(i2c0, RELAY_ADDR, data_buf, 1, false);
    }
    printf("Relay%d: off\r\n", bit);
}

void relay_all_off() {
    data_buf[0] = 0x0A;
    i2c_write_blocking(i2c0, RELAY_ADDR, data_buf, 1, false);

    printf("Relays: off\r\n");
}

void parse_gcode() {
    int i, cmd = -1, pin = -1, value = -1, sensor = -1;
    char *ptr;

    gcode_buf[gcode_len] = 0;

    // Calculate checksum, but only verify it if an asterisk is found
    uint8_t gcode_cksum = 0;
    for(i = 0; (i < gcode_len) && (gcode_buf[i] != '*'); i++) {
        gcode_cksum ^= gcode_buf[i];
    }
    if(gcode_buf[i] == '*') {
        if(gcode_cksum != strtoul((char*)(gcode_buf+i+1), NULL, 0)) {
            return;
        }
        gcode_buf[i] = 0;
        gcode_len = i;
    }

    ptr = strtok((char*)gcode_buf, " ");

    // Skip line numbers
    if((ptr != NULL) && (*ptr == 'N'))
        ptr = strtok(NULL, " ");

    if(ptr != NULL)
    switch(*ptr) {
        default:
            return;
        case 'G':
            cmd = strtoul(ptr+1, NULL, 10);
            break;
        case 'M':
            cmd = strtoul(ptr+1, NULL, 10);
            switch(cmd) {
                case 42: {
                    while((ptr != NULL)) {
                        ptr = strtok(NULL, " ");
                        if(ptr != NULL) {
                            switch(*ptr) {
                                case 'P':
                                    pin = strtoul(ptr+1, NULL, 10);
                                    break;
                                case 'S':
                                    value = strtoul(ptr+1, NULL, 10);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    if((pin != -1) && (value != -1)) {
                        if(value) {
                             relay_on(pin);
                        } else {
                             relay_off(pin);
                        }
                    }
                    break;
                }
                case 112: {
                    relay_all_off();
                    break;
                }
                case 115: {
                    printf("FIRMWARE_NAME: PicoPnP Vacuum Controller FIRMWARE_VERSION: 1.0 ELECTRONICS: PicoPnP FIRMWARE_DATE: 2023-01-11\r\n");
                    break;
                }
                case 308: {
                    while((ptr != NULL)) {
                        ptr = strtok(NULL, " ");
                        if(ptr != NULL) {
                            switch(*ptr) {
                                case 'S':
                                    sensor = strtoul(ptr+1, NULL, 10);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    if(sensor != -1) {
                        mpr_read(sensor);
                    }
                    break;
                }
            }
            break;
    }
}

int main() {
    stdio_init_all();

    i2c_init(i2c0, 400 * 1000);
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(5);
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_pull_up(6);
    gpio_set_function(7, GPIO_FUNC_I2C);
    gpio_pull_up(7);

    bi_decl(bi_program_name("PicoPnP Vacuum Controller"));
    bi_decl(bi_2pins_with_func(0, 1, GPIO_FUNC_UART));
    bi_decl(bi_2pins_with_func(4, 5, GPIO_FUNC_I2C));
    bi_decl(bi_2pins_with_func(6, 7, GPIO_FUNC_I2C));

    relay_all_off();

    while (1) {
        int c = getchar();
        if(c > 0) {
            switch(c) {
                case '\r':
                case '\n':
                    if(gcode_len > 0) parse_gcode();
                    gcode_len = 0;
                    break;
                default:
                    if(gcode_len < 254) {
                        gcode_buf[gcode_len] = c;
                        gcode_len++;
                    }
                    break;
            }
        }
    }
}
