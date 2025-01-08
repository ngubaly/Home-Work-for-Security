#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include <stdint.h>

// Define the Chip Select (CS) pin for SPI
#define IMU_CS_PIN       5       // Change this to the actual CS pin connected to the IMU
#define READ_BIT         0x80    // Read operation indicator

// ICM-42686 Register Addresses (Assumed based on typical IMU layouts)
// Please refer to the actual datasheet for precise addresses
#define REG_PWR_MGMT_1    0x06    // Power Management Register
#define REG_ACCEL_XOUT_H  0x00    // Accelerometer X-axis High Byte
#define REG_GYRO_XOUT_H   0x06    // Gyroscope X-axis High Byte
#define REG_DEVICE_ID     0x75    // Device ID Register
#define REG_RESET         0x07    // Reset Register

// Expected Device ID for ICM-42686
#define DEVICE_ID_ICM42686 0x43   // Example Device ID (verify from datasheet)

// Function Prototypes
static inline void cs_select();
static inline void cs_deselect();
static void imu_reset();
static void imu_init();
static uint8_t imu_read_device_id();
static void read_register(uint8_t add_reg, uint8_t *buf, uint16_t len);
static void imu_read_raw(int16_t acceleration[3], int16_t gyroscope[3]);

// Function to select the CS pin (active low)
static inline void cs_select(){
    gpio_put(IMU_CS_PIN, 0);
}

// Function to deselect the CS pin
static inline void cs_deselect(){
    gpio_put(IMU_CS_PIN, 1);
}

// Function to reset the IMU
static void imu_reset(){
    uint8_t reset_cmd[] = {REG_RESET, 0x01}; // Writing 1 to reset register
    cs_select();
    spi_write_blocking(spi_default, reset_cmd, 2);
    cs_deselect();
    sleep_ms(10); // Wait for reset to complete
}

// Function to initialize the IMU
static void imu_init(){
    uint8_t buf[2];

    // Wake up the device by clearing the sleep bit in the power management register
    buf[0] = REG_PWR_MGMT_1;
    buf[1] = 0x00; // Assuming 0x00 wakes up the device (verify from datasheet)
    cs_select();
    spi_write_blocking(spi_default, buf, 2);
    cs_deselect();
    sleep_ms(10); // Wait for power management settings to take effect

    // Additional configuration can be added here as needed
    // For example, setting sample rates, enabling specific sensors, etc.
}

// Function to read the device ID for verification
static uint8_t imu_read_device_id(){
    uint8_t device_id;
    read_register(REG_DEVICE_ID, &device_id, 1);
    return device_id;
}

// Function to read data from a specified register
static void read_register(uint8_t add_reg, uint8_t *buf, uint16_t len){
    uint8_t tx[len + 1];
    tx[0] = add_reg | READ_BIT;  // Set the read bit

    // Initialize dummy bytes for reading
    for(int i = 1; i <= len; i++) tx[i] = 0x00;

    cs_select();
    spi_write_read_blocking(spi_default, tx, buf, len + 1);
    cs_deselect();

    // Diagnostic print (optional, can be commented out after debugging)
    printf("Read from register 0x%02X: ", add_reg & 0x7F);
    for(int i = 1; i <= len; i++) {
        printf("0x%02X ", buf[i-1]);
    }
    printf("\n");
}

// Function to read raw accelerometer and gyroscope data
static void imu_read_raw(int16_t acceleration[3], int16_t gyroscope[3]){
    uint8_t buffer[6];

    // Read accelerometer data starting from REG_ACCEL_XOUT_H
    read_register(REG_ACCEL_XOUT_H, buffer, 6); // 6 bytes for X, Y, Z axes
    for(int i = 0; i < 3; ++i){
        acceleration[i] = ((int16_t)(buffer[i * 2] << 8) | buffer[(i*2) + 1]);
    }

    // Read gyroscope data starting from REG_GYRO_XOUT_H
    read_register(REG_GYRO_XOUT_H, buffer, 6); // 6 bytes for X, Y, Z axes
    for(int i = 0 ; i < 3; i++){
        gyroscope[i] = ((int16_t)(buffer[i * 2] << 8) | buffer[i*2 + 1]);
    }
}

int main() {
    // Initialize standard I/O for UART communication
    stdio_init_all();
    sleep_ms(1000); // Allow time for UART to initialize

    // Initialize SPI at 500 kHz (adjust if necessary)
    spi_init(spi_default, 500 * 1000);       
    
    // Set SPI GPIO pins
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);  // MISO
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI); // SCK
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);  // MOSI
    bi_decl(bi_3pins_with_func(
        PICO_DEFAULT_SPI_RX_PIN, 
        PICO_DEFAULT_SPI_TX_PIN, 
        PICO_DEFAULT_SPI_SCK_PIN, 
        GPIO_FUNC_SPI
    ));

    // Initialize CS pin
    gpio_init(IMU_CS_PIN);
    gpio_set_dir(IMU_CS_PIN, GPIO_OUT);
    gpio_put(IMU_CS_PIN, 1); // Deselect IMU
    bi_decl(bi_1pin_with_name(IMU_CS_PIN, "SPI CS"));

    // Configure SPI format: 8 bits, Mode 0, MSB first
    spi_set_format(spi_default, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);  

    // Reset and initialize the IMU
    imu_reset();
    imu_init();

    // Verify device ID
    uint8_t device_id = imu_read_device_id();
    if(device_id != DEVICE_ID_ICM42686){
        printf("Device ID mismatch! Expected 0x%02X, Got 0x%02X\n", DEVICE_ID_ICM42686, device_id);
        while (1) {
            sleep_ms(1000); // Halt execution
        }
    }
    printf("IMU Initialization Successful. Device ID: 0x%02X\n", device_id);

    int16_t accel[3], gyro[3];
    while (true)
    {   
        imu_read_raw(accel, gyro);

        // Print raw accelerometer data
        printf("Acc. X = %d, Y = %d, Z = %d\n", accel[0], accel[1], accel[2]);

        // Print raw gyroscope data
        printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);

        sleep_ms(1000); // Delay between readings
    }
}
