#include "sccb.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

static i2c_inst_t *s_i2c = NULL;

int sccb_init(int i2c_port, int pin_sda, int pin_scl, uint32_t freq_hz) {
    if (i2c_port < 0 || i2c_port > 1 || pin_sda < 0 || pin_scl < 0) {
        return -1;
    }
    s_i2c = i2c_get_instance(i2c_port);

    gpio_set_function(pin_sda, GPIO_FUNC_I2C);
    gpio_set_function(pin_scl, GPIO_FUNC_I2C);
    gpio_pull_up(pin_sda);
    gpio_pull_up(pin_scl);

    i2c_init(s_i2c, freq_hz);
    return 0;
}

void sccb_deinit(void) {
    if (s_i2c) {
        i2c_deinit(s_i2c);
        s_i2c = NULL;
    }
}

int sccb_write8(uint8_t slave_addr, uint8_t reg, uint8_t value) {
    if (!s_i2c) return -1;
    uint8_t data[] = { reg, value };
    int ret = i2c_write_blocking(s_i2c, slave_addr, data, sizeof(data), false);
    return (ret == (int)sizeof(data)) ? 0 : -1;
}

int sccb_read8(uint8_t slave_addr, uint8_t reg, uint8_t *value) {
    if (!s_i2c || !value) return -1;
    // SCCB read: write register address with repeated start, then read
    int ret = i2c_write_blocking(s_i2c, slave_addr, &reg, 1, true);
    if (ret < 0) return -1;
    ret = i2c_read_blocking(s_i2c, slave_addr, value, 1, false);
    return (ret == 1) ? 0 : -1;
}

int sccb_write16(uint8_t slave_addr, uint16_t reg, uint8_t value) {
    if (!s_i2c) return -1;
    uint8_t data[] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), value };
    int ret = i2c_write_blocking(s_i2c, slave_addr, data, sizeof(data), false);
    return (ret == (int)sizeof(data)) ? 0 : -1;
}

int sccb_read16(uint8_t slave_addr, uint16_t reg, uint8_t *value) {
    if (!s_i2c || !value) return -1;
    uint8_t addr[] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    int ret = i2c_write_blocking(s_i2c, slave_addr, addr, sizeof(addr), true);
    if (ret < 0) return -1;
    ret = i2c_read_blocking(s_i2c, slave_addr, value, 1, false);
    return (ret == 1) ? 0 : -1;
}

void sccb_write_list8(uint8_t slave_addr, const uint8_t (*regs)[2]) {
    while (regs) {
        uint8_t reg = (*regs)[0];
        uint8_t value = (*regs)[1];
        if (reg == 0xFF && value == 0xFF) break;
        sccb_write8(slave_addr, reg, value);
        regs++;
    }
}

void sccb_write_list16(uint8_t slave_addr, const uint16_t (*regs)[2]) {
    while (regs) {
        uint16_t reg = (*regs)[0];
        uint8_t value = (uint8_t)(*regs)[1];
        if (reg == 0xFFFF) break;
        if (reg == 0xFFFE) {
            sleep_ms(value);
        } else {
            sccb_write16(slave_addr, reg, value);
        }
        regs++;
    }
}
