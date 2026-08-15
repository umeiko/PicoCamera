/**
 * @file sccb.h
 * @brief Shared SCCB (I2C) access layer.
 *
 * Sensors differ in register address width (OV2640: 8-bit, OV3660: 16-bit);
 * both variants are provided here so sensor modules stay pure register logic.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SCCB bus.
 * @param i2c_port  RP2040 I2C peripheral number: 0 or 1
 * @param pin_sda   SDA GPIO
 * @param pin_scl   SCL GPIO
 * @param freq_hz   Bus frequency (100000 is a safe default)
 * @return 0 on success
 */
int sccb_init(int i2c_port, int pin_sda, int pin_scl, uint32_t freq_hz);

/** Release the SCCB bus pins and peripheral. */
void sccb_deinit(void);

/** 8-bit register address access (e.g. OV2640). Return 0 on success. */
int  sccb_write8(uint8_t slave_addr, uint8_t reg, uint8_t value);
int  sccb_read8(uint8_t slave_addr, uint8_t reg, uint8_t *value);

/** 16-bit register address access (e.g. OV3660). Return 0 on success. */
int  sccb_write16(uint8_t slave_addr, uint16_t reg, uint8_t value);
int  sccb_read16(uint8_t slave_addr, uint16_t reg, uint8_t *value);

/** Write a {reg, value} list terminated by {0xFF, 0xFF} (8-bit addresses). */
void sccb_write_list8(uint8_t slave_addr, const uint8_t (*regs)[2]);

/**
 * Write a {reg, value} list terminated by reg == 0xFFFF (16-bit addresses).
 * reg == 0xFFFE is a delay marker, value gives milliseconds.
 */
void sccb_write_list16(uint8_t slave_addr, const uint16_t (*regs)[2]);

#ifdef __cplusplus
}
#endif
