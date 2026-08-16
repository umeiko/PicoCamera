#include "pico_camera.h"

#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "driver/sccb.h"
#include "driver/xclk.h"
#include "driver/pio_capture.h"
#include "sensors/ov2640.h"
#include "sensors/ov3660.h"
#include "sensors/ov7670.h"

// Debug output: set to 1 for verbose logging on stdio
#define PICOCAM_DEBUG 0
#if PICOCAM_DEBUG
    #define PC_LOG(...) printf(__VA_ARGS__)
#else
    #define PC_LOG(...)
#endif

// ---------------------------------------------------------------------------
// Sensor registry - new models register an entry here
// ---------------------------------------------------------------------------

typedef struct {
    const camera_sensor_info_t *info;
    int (*detect)(sensor_t *sensor);
    int (*init)(sensor_t *sensor);
} sensor_entry_t;

static const camera_sensor_info_t s_info_ov2640 = {
    "OV2640", OV2640_SCCB_ADDR, OV2640_PID, FRAMESIZE_UXGA, true
};

static const camera_sensor_info_t s_info_ov3660 = {
    "OV3660", OV3660_SCCB_ADDR, OV3660_PID, FRAMESIZE_UXGA, true
};

static const camera_sensor_info_t s_info_ov7670 = {
    "OV7670", OV7670_SCCB_ADDR, OV7670_PID, FRAMESIZE_VGA, false  // no JPEG encoder
};

static const sensor_entry_t s_sensors[] = {
    { &s_info_ov2640, ov2640_detect, ov2640_init_sensor },
    { &s_info_ov3660, ov3660_detect, ov3660_init_sensor },
    { &s_info_ov7670, ov7670_detect, ov7670_init_sensor },
};

// ---------------------------------------------------------------------------
// Driver state
// ---------------------------------------------------------------------------

typedef struct {
    camera_fb_t fb;
    uint8_t *raw;       // malloc base pointer (for free)
    bool in_use;
} fb_slot_t;

static struct {
    bool inited;
    camera_config_t cfg;
    sensor_t sensor;
    const camera_sensor_info_t *info;
    fb_slot_t *fbs;
    size_t fb_count;
    size_t fb_cap;      // capacity in bytes of each buffer
} s_cam;

#define SCCB_FREQ_HZ        100000
#define DEFAULT_XCLK_HZ     10000000
#define CAPTURE_TIMEOUT_MS  2000

static size_t frame_bytes_for(pixformat_t fmt, framesize_t fs) {
    size_t w = resolution[fs].width;
    size_t h = resolution[fs].height;
    switch (fmt) {
        case PIXFORMAT_RGB565: return w * h * 2;
        case PIXFORMAT_JPEG:   return w * h / 4 + 8192;  // upper bound estimate
        default: return 0;
    }
}

static pico_camera_err_t validate_config(const camera_config_t *c) {
    if (!c) return PICO_CAMERA_ERR_INVALID_ARG;
    if (c->pin_xclk < 0 || c->pin_sccb_sda < 0 || c->pin_sccb_scl < 0 ||
        c->pin_vsync < 0 || c->pin_href < 0 || c->pin_pclk < 0 || c->pin_d0 < 0) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }
    // PIO "in pins" samples 8 consecutive GPIOs
    const int d[8] = { c->pin_d0, c->pin_d1, c->pin_d2, c->pin_d3,
                       c->pin_d4, c->pin_d5, c->pin_d6, c->pin_d7 };
    for (int i = 0; i < 8; i++) {
        if (d[i] != c->pin_d0 + i) {
            return PICO_CAMERA_ERR_INVALID_ARG;  // data pins must be consecutive
        }
    }
    if (c->frame_size >= FRAMESIZE_INVALID || c->pixel_format >= PIXFORMAT_INVALID) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }
    if (c->fb_count < 1) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }
    if (c->sccb_i2c_port < 0 || c->sccb_i2c_port > 1) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }
    return PICO_CAMERA_OK;
}

pico_camera_err_t pico_camera_init(const camera_config_t *config) {
    if (s_cam.inited) {
        return PICO_CAMERA_ERR_INVALID_STATE;
    }
    pico_camera_err_t err = validate_config(config);
    if (err != PICO_CAMERA_OK) {
        return err;
    }
    memcpy(&s_cam.cfg, config, sizeof(camera_config_t));

    // Master clock first: sensors need XCLK to answer on SCCB
    uint32_t xclk = config->xclk_freq_hz > 0 ? (uint32_t)config->xclk_freq_hz : DEFAULT_XCLK_HZ;
    if (xclk_start(config->pin_xclk, xclk) != 0) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }

    if (sccb_init(config->sccb_i2c_port, config->pin_sccb_sda, config->pin_sccb_scl, SCCB_FREQ_HZ) != 0) {
        return PICO_CAMERA_ERR_INVALID_ARG;
    }

    // Power control
    if (config->pin_pwdn >= 0) {
        gpio_init(config->pin_pwdn);
        gpio_set_dir(config->pin_pwdn, GPIO_OUT);
        gpio_put(config->pin_pwdn, 0);  // power on
        sleep_ms(10);
    }

    // Hardware reset
    if (config->pin_reset >= 0) {
        gpio_init(config->pin_reset);
        gpio_set_dir(config->pin_reset, GPIO_OUT);
        gpio_put(config->pin_reset, 0);
        sleep_ms(100);
        gpio_put(config->pin_reset, 1);
        sleep_ms(200);
    }

    // Probe the sensor registry
    memset(&s_cam.sensor, 0, sizeof(sensor_t));
    bool found = false;
    for (size_t i = 0; i < sizeof(s_sensors) / sizeof(s_sensors[0]); i++) {
        if (s_sensors[i].detect(&s_cam.sensor) == 0) {
            s_cam.info = s_sensors[i].info;
            PC_LOG("[PicoCamera] Detected: %s\n", s_cam.info->name);
            s_sensors[i].init(&s_cam.sensor);
            found = true;
            break;
        }
    }
    if (!found) {
        PC_LOG("[PicoCamera] ERROR: no supported sensor detected\n");
        return PICO_CAMERA_ERR_NOT_DETECTED;
    }

    // Reject requests the detected sensor cannot satisfy (esp32-camera parity):
    // JPEG on a sensor without a JPEG encoder is a hard error at init.
    if (config->pixel_format == PIXFORMAT_JPEG && !s_cam.info->support_jpeg) {
        PC_LOG("[PicoCamera] ERROR: JPEG format is not supported on %s\n", s_cam.info->name);
        pico_camera_deinit();
        return PICO_CAMERA_ERR_NOT_SUPPORTED;
    }
    // A frame size beyond the sensor's maximum is clamped, with a warning.
    if (config->frame_size > s_cam.info->max_size) {
        PC_LOG("[PicoCamera] WARNING: frame size exceeds %s max, clamping to %ux%u\n",
               s_cam.info->name,
               (unsigned)resolution[s_cam.info->max_size].width,
               (unsigned)resolution[s_cam.info->max_size].height);
        s_cam.cfg.frame_size = s_cam.info->max_size;
    }

    // Frame buffers
    s_cam.fb_cap = frame_bytes_for(s_cam.cfg.pixel_format, s_cam.cfg.frame_size);
    s_cam.fbs = (fb_slot_t *)calloc(config->fb_count, sizeof(fb_slot_t));
    if (!s_cam.fbs) {
        return PICO_CAMERA_ERR_NO_MEM;
    }
    s_cam.fb_count = config->fb_count;
    for (size_t i = 0; i < s_cam.fb_count; i++) {
        uint8_t *raw = (uint8_t *)malloc(s_cam.fb_cap + 32);
        if (!raw) {
            PC_LOG("[PicoCamera] ERROR: malloc(%u) failed\n", (unsigned)s_cam.fb_cap);
            pico_camera_deinit();
            return PICO_CAMERA_ERR_NO_MEM;
        }
        s_cam.fbs[i].raw = raw;
        s_cam.fbs[i].fb.buf = (uint8_t *)(((uintptr_t)raw + 31) & ~(uintptr_t)0x1F);
        s_cam.fbs[i].in_use = false;
    }

    // Capture engine
    pio_capture_pins_t pins = {
        config->pin_d0, config->pin_vsync, config->pin_href, config->pin_pclk
    };
    if (pio_capture_init(&pins) != 0) {
        pico_camera_deinit();
        return PICO_CAMERA_FAIL;
    }

    // Apply requested format and size
    if (s_cam.sensor.set_pixformat &&
        s_cam.sensor.set_pixformat(&s_cam.sensor, config->pixel_format) != 0) {
        pico_camera_deinit();
        return PICO_CAMERA_ERR_FAILED_TO_SET_OUT_FORMAT;
    }
    sleep_ms(100);
    if (s_cam.sensor.set_framesize &&
        s_cam.sensor.set_framesize(&s_cam.sensor, s_cam.cfg.frame_size) != 0) {
        pico_camera_deinit();
        return PICO_CAMERA_ERR_FAILED_TO_SET_FRAME_SIZE;
    }
    sleep_ms(100);
    if (config->pixel_format == PIXFORMAT_JPEG && s_cam.sensor.set_quality) {
        s_cam.sensor.set_quality(&s_cam.sensor, config->jpeg_quality);
    }

    sleep_ms(300);  // let the signal settle
    s_cam.inited = true;
    PC_LOG("[PicoCamera] Init OK (%ux%u, fb_count=%u)\n",
           (unsigned)resolution[s_cam.cfg.frame_size].width,
           (unsigned)resolution[s_cam.cfg.frame_size].height,
           (unsigned)s_cam.fb_count);
    return PICO_CAMERA_OK;
}

pico_camera_err_t pico_camera_deinit(void) {
    pio_capture_deinit();
    xclk_stop(s_cam.cfg.pin_xclk);
    sccb_deinit();
    if (s_cam.fbs) {
        for (size_t i = 0; i < s_cam.fb_count; i++) {
            free(s_cam.fbs[i].raw);
        }
        free(s_cam.fbs);
    }
    memset(&s_cam, 0, sizeof(s_cam));
    return PICO_CAMERA_OK;
}

camera_fb_t *pico_camera_fb_get(void) {
    if (!s_cam.inited) {
        return NULL;
    }

    // Find a free buffer
    fb_slot_t *slot = NULL;
    for (size_t i = 0; i < s_cam.fb_count; i++) {
        if (!s_cam.fbs[i].in_use) {
            slot = &s_cam.fbs[i];
            break;
        }
    }
    if (!slot) {
        return NULL;
    }

    size_t len = 0;
    if (s_cam.cfg.pixel_format == PIXFORMAT_JPEG) {
        if (pio_capture_frame_variable(slot->fb.buf, s_cam.fb_cap, &len, CAPTURE_TIMEOUT_MS) != 0) {
            PC_LOG("[PicoCamera] JPEG capture timeout\n");
            return NULL;
        }
    } else {
        len = frame_bytes_for(s_cam.cfg.pixel_format, s_cam.cfg.frame_size);
        if (len > s_cam.fb_cap) {
            return NULL;
        }
        if (pio_capture_frame(slot->fb.buf, len, CAPTURE_TIMEOUT_MS) != 0) {
            PC_LOG("[PicoCamera] capture timeout\n");
            return NULL;
        }
    }

    slot->in_use = true;
    slot->fb.len = len;
    slot->fb.width = resolution[s_cam.cfg.frame_size].width;
    slot->fb.height = resolution[s_cam.cfg.frame_size].height;
    slot->fb.format = s_cam.cfg.pixel_format;
    uint64_t us = time_us_64();
    slot->fb.timestamp.tv_sec = (time_t)(us / 1000000);
    slot->fb.timestamp.tv_usec = (suseconds_t)(us % 1000000);
    return &slot->fb;
}

void pico_camera_fb_return(camera_fb_t *fb) {
    if (!fb || !s_cam.fbs) {
        return;
    }
    for (size_t i = 0; i < s_cam.fb_count; i++) {
        if (&s_cam.fbs[i].fb == fb) {
            s_cam.fbs[i].in_use = false;
            return;
        }
    }
}

sensor_t *pico_camera_sensor_get(void) {
    return s_cam.inited ? &s_cam.sensor : NULL;
}

const camera_sensor_info_t *pico_camera_sensor_info_get(void) {
    return s_cam.inited ? s_cam.info : NULL;
}
