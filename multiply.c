#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define IMU_I2C_ADDR 0x68 // I2C address if AP_AD0 (Pin 1) is tied to GND
#define WHO_AM_I_REG 0x75 // WHO_AM_I register address

void i2c_init_custom() {
    i2c_init(I2C_PORT, 100 * 1000); // I2C at 100 kHz
    gpio_set_function(18, GPIO_FUNC_I2C); // GPIO 18 for SDA
    gpio_set_function(19, GPIO_FUNC_I2C); // GPIO 19 for SCL
    gpio_pull_up(18);
    gpio_pull_up(19);
}

uint8_t read_who_am_i() {
    uint8_t reg = WHO_AM_I_REG;
    uint8_t value;
    i2c_write_blocking(I2C_PORT, IMU_I2C_ADDR, &reg, 1, true); // Send register address
    i2c_read_blocking(I2C_PORT, IMU_I2C_ADDR, &value, 1, false); // Read value
    return value;
}

int main() {
    stdio_init_all();
    i2c_init_custom();

    while (true) {
        uint8_t who_am_i = read_who_am_i();
        printf("WHO_AM_I: 0x%02X\n", who_am_i);
        sleep_ms(1000);
    }
}
