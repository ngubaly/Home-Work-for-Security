#include "pico/stdlib.h"
#include "hardware/i2c.h"
#define ICM45686_ADDR 0x68  // Address when AP_AD0 is low
i2c_init(i2c1, 400 * 1000);  // Initialize I2C at 400kHz
gpio_set_function(20, GPIO_FUNC_I2C);  // Set GPIO20 to SDA
gpio_set_function(21, GPIO_FUNC_I2C);  // Set GPIO21 to SCL
gpio_pull_up(20);  // Enable pull-up resistors
gpio_pull_up(21);
static void write_register(uint8_t addr, uint8_t data) {
    i2c_write_blocking(i2c1, ICM45686_ADDR, &addr, 1, false);
    i2c_write_blocking(i2c1, ICM45686_ADDR, &data, 1, true);
}
static void read_registers(uint8_t addr, uint8_t *data, uint8_t length) {
    i2c_write_blocking(i2c1, ICM45686_ADDR, &addr, 1, false);
    i2c_read_blocking(i2c1, ICM45686_ADDR, data, length, false);
}
static void imu_reset() {
    write_register(0x10, 0x00);  // Reset register, normal mode with internal clock
    sleep_ms(100);  // Wait for reset
}

static void configure_sensor() {
    // Configure ACCEL_CONFIG0 for ±2g
    uint8_t accel_config = 0x00;
    write_register(0x17, accel_config);

    // Configure GYRO_CONFIG0 for ±250dps
    uint8_t gyro_config = 0x00;
    write_register(0x18, gyro_config);

    sleep_ms(100);  // Wait for configuration
}static void imu_read_raw(int16_t acceleration[3], int16_t gyroscope[3], int16_t *temp) {
    uint8_t buffer[6];

    // Read acceleration data
    read_registers(0x00, buffer, 6);
    acceleration[0] = (int16_t)((buffer[0] << 8) | buffer[1]);
    acceleration[1] = (int16_t)((buffer[2] << 8) | buffer[3]);
    acceleration[2] = (int16_t)((buffer[4] << 8) | buffer[5]);

    // Read gyroscope data
    read_registers(0x06, buffer, 6);
    gyroscope[0] = (int16_t)((buffer[0] << 8) | buffer[1]);
    gyroscope[1] = (int16_t)((buffer[2] << 8) | buffer[3]);
    gyroscope[2] = (int16_t)((buffer[4] << 8) | buffer[5]);

    // Read temperature data
    read_registers(0x0B, (uint8_t *)temp, 2);
}int main() {
    stdio_init_all();
    sleep_ms(1000);

    // Initialize I2C
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(20, GPIO_FUNC_I2C);
    gpio_set_function(21, GPIO_FUNC_I2C);
    gpio_pull_up(20);
    gpio_pull_up(21);

    // Reset the IMU
    imu_reset();

    // Configure sensor
    configure_sensor();

    int16_t accel[3], gyro[3], temp;

    while (true) {
        imu_read_raw(accel, gyro, &temp);

        // Scale the data
        float acc_x = (float)accel[0] / 16384.0 * 2; // Assuming ±2g
        float acc_y = (float)accel[1] / 16384.0 * 2;
        float acc_z = (float)accel[2] / 16384.0 * 2;

        float gyro_x = (float)gyro[0] / 32768.0 * 250; // Assuming ±250dps
        float gyro_y = (float)gyro[1] / 32768.0 * 250;
        float gyro_z = (float)gyro[2] / 32768.0 * 250;

        float temperature = (float)temp / 1024.0;

        printf("Acc. X = %.4f g, Y = %.4f g, Z = %.4f g\n", acc_x, acc_y, acc_z);
        printf("Gyro. X = %.4f dps, Y = %.4f dps, Z = %.4f dps\n", gyro_x, gyro_y, gyro_z);
        printf("Temp. = %.2f °C\n", temperature);

        sleep_ms(1000);
    }
}