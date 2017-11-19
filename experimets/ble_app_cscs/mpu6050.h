#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mpu6050_init(void);


uint32_t mpu6050_register_write(uint8_t register_address, const uint8_t value);

bool mpu6050_register_read(uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes);


#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H */
