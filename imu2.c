#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include <stdint.h>
#include "hardware/interrupt.h"
#include "hardware/gpio.h"

#define READ_BIT        0x80
#define FIFO_CONFIG0    0x06
#define FIFO_CONFIG1_0  0x07
#define FIFO_CONFIG1_1  0x08
#define FIFO_CONFIG2    0x09
#define FIFO_CONFIG3    0x0A
#define FIFO_CONFIG4    0x0B
#define INT1_CONFIG0    0x0F
#define PWR_MGMT0       0x10
#define ACCEL_CONFIG0   0x11
#define GYRO_CONFIG0    0x12
#define FIFO_DATA       0x07
#define FIFO_COUNT_0    0x22
#define FIFO_COUNT_1    0x23
#define INT1_STATUS0    0x32

// GPIO pin for IMU interrupt
#define IMU_INT_PIN     20

// Initialize SPI and GPIO
void initialize_spi_and_gpio() {
    spi_init(spi_default, 500 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));
    // Set up IMU interrupt pin
    gpio_set_dir(IMU_INT_PIN, GPIO_IN);
    gpio_pull_up(IMU_INT_PIN);
    interrupt_set_mask_enabled(IMU_INT_PIN, true);
    irq_set_exclusive_handler(IMU_INT_PIN, imu_interrupt_handler);
    irq_set_enabled(IMU_INT_PIN, true);
}
void configure_fifo_and_interrupts() {
    // Enable FIFO and set mode
    uint8_t fifo_config0 = 0x01; // Enable FIFO
    write_register(FIFO_CONFIG0, fifo_config0);
    // Set data sources (accel and gyro)
    uint8_t fifo_config2 = 0x03; // Accelerometer and Gyroscope
    write_register(FIFO_CONFIG2, fifo_config2);
    // Set watermark level to FIFO full
    uint8_t fifo_config1_0 = 0xFF; // Lower byte of watermark
    uint8_t fifo_config1_1 = 0xFF; // Upper byte of watermark
    write_register(FIFO_CONFIG1_0, fifo_config1_0);
    write_register(FIFO_CONFIG1_1, fifo_config1_1);
    // Enable FIFO full interrupt
    uint8_t int1_config0 = 0x10; // Enable FIFO full interrupt
    write_register(INT1_CONFIG0, int1_config0);
}
void configure_sensors() {
    // Set accelerometer and gyroscope data rates
    uint8_t accel_config0 = 0x00; // Set accelerometer data rate
    write_register(ACCEL_CONFIG0, accel_config0);
    uint8_t gyro_config0 = 0x00; // Set gyroscope data rate
    write_register(GYRO_CONFIG0, gyro_config0);
    // Enable sensors
    uint8_t pwr_mgt0 = 0x00; // Enable accelerometer and gyroscope
    write_register(PWR_MGMT0, pwr_mgt0);
}
void imu_interrupt_handler() {
    uint16_t fifo_count = read_fifo_count();
    if (fifo_count > 0) {
        uint8_t *fifo_data = read_fifo_data(fifo_count);
        parse_fifo_data(fifo_data, fifo_count);
        // Clear interrupt flag
        write_register(INT1_STATUS0, 0x00);
    }
}
uint16_t read_fifo_count() {
    uint8_t count_l = read_register(FIFO_COUNT_0);
    uint8_t count_h = read_register(FIFO_COUNT_1);
    return (count_h << 8) | count_l;
}

uint8_t* read_fifo_data(uint16_t count) {
    uint8_t *data = (uint8_t*)calloc(count, sizeof(uint8_t));
    cs_select();
    write_register(FIFO_DATA, data, count);
    cs_deselect();
    return data;
}
void parse_fifo_data(uint8_t *data, uint16_t count) {
    // Assuming data is in 16-bit little-endian format
    for (int i = 0; i < count; i += 6) {
        int16_t accel_x = (data[i+1] << 8) | data[i];
        int16_t accel_y = (data[i+3] << 8) | data[i+2];
        int16_t accel_z = (data[i+5] << 8) | data[i+4];
        // Process acceleration data
        // Similarly, extract gyro data if included
    }
}
int main() {
    stdio_init_all();
    initialize_spi_and_gpio();
    imu_reset();
    configure_fifo_and_interrupts();
    configure_sensors();
    while (true) {
        // Main loop can be used for other tasks
        sleep_ms(1000);
    }
}