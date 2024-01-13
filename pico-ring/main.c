#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define LED_COUNT 8

volatile uint8_t data_buf[LED_COUNT*4+(LED_COUNT)+8];
char gcode_buf[256];
volatile uint8_t gcode_len = 0;

void led_write(uint32_t val, uint8_t seg) {
    uint32_t a, b, c;
    int i = 0, d;

    if(val > 0x2fd) val = 0x2fd;
    if(seg == 0) seg = 0xff;

    // Simple dithering
    a = val / 3;
    b = ((val - a) / 2);
    c = val - (a + b);

    // APA102 Start Frame
    data_buf[i++] = 0x00;
    data_buf[i++] = 0x00;
    data_buf[i++] = 0x00;
    data_buf[i++] = 0x00;

    d = 0;
    while(i < LED_COUNT*4+4) {
        if(seg & (1 << d)) {
            data_buf[i++] = 0xf0;
            data_buf[i++] = a;
            data_buf[i++] = b;
            data_buf[i++] = c;
        } else {
            data_buf[i++] = 0xf0;
            data_buf[i++] = 0x00;
            data_buf[i++] = 0x00;
            data_buf[i++] = 0x00;
        }
        d++;
    }

    // Padding bits to ensure all packages get data clocked out to them
    while(i < sizeof(data_buf)) {
        data_buf[i++] = 0x00;
    }

    dma_channel_set_read_addr(0, data_buf, true);
    dma_channel_wait_for_finish_blocking(0);
}

void parse_gcode() {
    int i, cmd = -1, value = -1, segment = 0xff;
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
                case 150: {
                    while((ptr != NULL)) {
                        ptr = strtok(NULL, " ");
                        if(ptr != NULL) {
                            switch(*ptr) {
                                case 'Q':
                                    segment = strtoul(ptr+1, NULL, 10);
                                    break;
                                case 'P':
                                    if(strchr(ptr+1, '.')) {
                                        value = (int)(765.0f * strtof(ptr+1, NULL));
                                    } else {
                                        value = strtoul(ptr+1, NULL, 10);
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    if(value != -1) {
                         led_write(value, segment);
                    }
                    break;
                }
                case 112: {
                    led_write(0, 0);
                    break;
                }
                case 115: {
                    printf("FIRMWARE_NAME: PicoPnP Ring Controller FIRMWARE_VERSION: 1.0 ELECTRONICS: PicoPnP FIRMWARE_DATE: 2023-01-12\r\n");
                    break;
                }
            }
            break;
    }
}

int main() {
    stdio_init_all();

    // add program information for picotool
    bi_decl(bi_program_name("PicoPnP Ring Controller"));
    bi_decl(bi_2pins_with_func(0, 1, GPIO_FUNC_UART));
    bi_decl(bi_2pins_with_func(2, 3, GPIO_FUNC_SPI));

    dma_channel_claim(0);

    spi_init(spi0, 8000000);
    spi_set_format(spi0, 8, 1, 1, SPI_MSB_FIRST);
    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);

    dma_channel_config dmaConfig = dma_channel_get_default_config(0);
    channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_8);
    channel_config_set_dreq(&dmaConfig, spi_get_dreq(spi0, true));
    channel_config_set_read_increment(&dmaConfig, true);
    channel_config_set_write_increment(&dmaConfig, false);
    dma_channel_configure(0, &dmaConfig, &spi_get_hw(spi0)->dr, NULL, sizeof(data_buf), false);

    // Turn off all LEDs
    led_write(0, 0);

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
