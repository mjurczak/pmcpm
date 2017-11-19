#include <stdbool.h>9
#include <stdint.h>

#include "nrf_drv_twi.h"
#include "mpu6050.h"
#include "app_util_platform.h"

#define ADDRESS_WHO_AM_I          (0x75U) // !< WHO_AM_I register identifies the device. Expected value is 0x68.
#define ADDRESS_SIGNAL_PATH_RESET (0x68U) // !<

#define MPU6050_REG_PWR_MGMT_1		(0x6B)
#define MPU6050_REG_CONFIG    		(0x1A)
#define MPU6050_REG_GYRO_CONFIG   (0x1B)
#define MPU6050_REG_SMPRT_DIV     (0x19)
#define MPU6050_REG_GYRO_ZOUT_H   (0x47)

static const uint8_t expected_who_am_i = 0x68U; // !< Expected value to get from WHO_AM_I register.
static const uint8_t              twi_address = 0x68;         
static const nrf_drv_twi_t        twi_instance = NRF_DRV_TWI_INSTANCE(0);
static const nrf_drv_twi_config_t twi_config = {
       .scl                = 4,
       .sda                = 3,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };


bool mpu6050_verify_product_id(void);
		
bool mpu6050_init(void)
{
    uint32_t errCode;

	  // Initialize TWI
	  errCode = nrf_drv_twi_init(&twi_instance,
                     &twi_config,
                     NULL,
                     NULL);
    APP_ERROR_CHECK(errCode); 
	
    nrf_drv_twi_enable(&twi_instance);
	
    // Do a reset on signal paths
    uint8_t reset_value = 0x04U | 0x02U | 0x01U; // Resets gyro, accelerometer and temperature sensor signal paths.
    errCode = mpu6050_register_write(ADDRESS_SIGNAL_PATH_RESET, reset_value);
    APP_ERROR_CHECK(errCode); 

	
		// Basic gyroscope configuration
		if (mpu6050_verify_product_id())
		{
			
			// Set DLPF to 5Hz lowpass filtering of gyro signal
			errCode = mpu6050_register_write(MPU6050_REG_CONFIG, 0x06);
      APP_ERROR_CHECK(errCode); 

			// Set clock source - gyro X 
			errCode = mpu6050_register_write(MPU6050_REG_PWR_MGMT_1, 0x01);
			APP_ERROR_CHECK(errCode); 

			// Set gyro full-scale range to 2000 deg/s
			// Clear self-test bits
			errCode = mpu6050_register_write(MPU6050_REG_GYRO_CONFIG, (0x03 << 3));
			APP_ERROR_CHECK(errCode); 

			// Set sampling rate divider (1kHz base frequency)
			// 10 Hz
			errCode = mpu6050_register_write(MPU6050_REG_SMPRT_DIV, 99);
      APP_ERROR_CHECK(errCode); 
		}
	
    return true;
}

bool mpu6050_verify_product_id(void)
{
    uint8_t who_am_i;

    if (mpu6050_register_read(ADDRESS_WHO_AM_I, &who_am_i, 1))
    {
        if (who_am_i != expected_who_am_i)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

int16_t mpu6050_read_gyro_z(void)
{
	int16_t result = 0;
	int32_t tmp;
	uint8_t rawData[2];
	
	mpu6050_register_read(MPU6050_REG_GYRO_ZOUT_H, rawData, 2);
	
	//result = (rawData[0] << 8) | rawData[1];
	
	tmp = (int16_t)(rawData[0] << 8 | rawData[1]);
	tmp *= 2000;
	tmp /= 32768;
	
	result = tmp;
	
	return result;
}

uint32_t mpu6050_register_write(uint8_t register_address, uint8_t value)
{
    uint8_t w2_data[2];

    w2_data[0] = register_address;
    w2_data[1] = value;
	
	  return nrf_drv_twi_tx(&twi_instance,
                          twi_address,
                          w2_data,
                          2,
                          false);
}

bool mpu6050_register_read(uint8_t register_address, uint8_t * destination, uint8_t number_of_bytes)
{
    uint32_t errCode;
	
	  errCode = nrf_drv_twi_tx(&twi_instance,
                          twi_address,
                          &register_address,
                          1,
                          true);
	  APP_ERROR_CHECK(errCode); 

    errCode = nrf_drv_twi_rx(&twi_instance,
                          twi_address,
                          destination,
                          number_of_bytes,
                          false);
	  APP_ERROR_CHECK(errCode); 

    return true;
}

