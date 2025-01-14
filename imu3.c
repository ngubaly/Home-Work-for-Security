#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdint.h>

#define READ_BIT        0x80
#define FIFO_COUNT_H    0x72
#define FIFO_R_W        0x74
#define FIFO_CONFIG     0x1D
#define FIFO_ENABLE     0x21
#define INT_ENABLE      0x16
#define INT_STATUS      0x1A

// Function to write to a register
void write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    cs_select();
    spi_write_blocking(spi_default, buffer, 2);
    cs_deselect();
}

// Function to configure the FIFO
void configure_fifo() {
    // Disable FIFO first
    write_register(FIFO_CONFIG, 0x00);

    // Enable FIFO for accel, gyro, and temperature
    write_register(FIFO_ENABLE, 0x07);

    // Set FIFO to stream mode
    write_register(FIFO_CONFIG, 0x01);
}

// Function to configure interrupts
void configure_interrupts() {
    // Enable FIFO threshold interrupt on INT1
    write_register(INT_ENABLE, 0x10);
}

// Function to read data from FIFO
void read_fifo_data() {
    uint16_t fifo_count;
    uint8_t fifo_count_buf[2];
    uint8_t fifo_data[256]; // Adjust size based on FIFO size and watermark
    
    // Read FIFO count
    read_register(FIFO_COUNT_H, fifo_count_buf, 2);
    fifo_count = (fifo_count_buf[0] << 8) | fifo_count_buf[1];

    if (fifo_count > 0) {
        // Read data from FIFO
        read_register(FIFO_R_W, fifo_data, fifo_count);
        // Process the data as needed
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);

    spi_init(spi_default, 500 * 1000);

    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    configure_fifo();
    configure_interrupts();

    while (true) {
        if (gpio_get(INTERRUPT_PIN)) { // Check interrupt pin
            read_fifo_data();
        }
        sleep_ms(10);
    }
}
