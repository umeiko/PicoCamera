/**
 * PicoCamera minimal Pico SDK example — no Arduino core involved.
 *
 * Initializes the camera (OV2640/OV3660/OV7670/GC2145), captures frames in a
 * loop and prints frame info over USB serial. With PIXFORMAT_JPEG the
 * buffer is a complete JPEG file and its SOI/EOI markers are checked.
 *
 * Wiring: same as the readme quick-start table — adjust pin numbers in
 * config below to match your board.
 */
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "PicoCamera.h"

int main(void) {
    stdio_init_all();

    camera_config_t config = {
        .pin_pwdn     = -1,
        .pin_reset    = 29,
        .pin_xclk     = 5,
        .pin_sccb_sda = 12,
        .pin_sccb_scl = 13,
        .pin_d0 = 14, .pin_d1 = 15, .pin_d2 = 16, .pin_d3 = 17,
        .pin_d4 = 18, .pin_d5 = 19, .pin_d6 = 20, .pin_d7 = 21,
        .pin_vsync = 2, .pin_href = 3, .pin_pclk = 4,
        .xclk_freq_hz  = 10000000,
        .sccb_i2c_port = 0,
        .pixel_format  = PIXFORMAT_JPEG,   // or PIXFORMAT_RGB565
        .frame_size    = FRAMESIZE_QVGA,
        .jpeg_quality  = 10,
        .fb_count      = 1,
        .fb_location   = PICO_CAMERA_FB_AUTO,
    };

    pico_camera_err_t err = pico_camera_init(&config);
    if (err != PICO_CAMERA_OK) {
        printf("pico_camera_init failed: %d\n", err);
        return 1;
    }

    const camera_sensor_info_t *info = pico_camera_sensor_info_get();
    printf("sensor: %s (pid 0x%04x, jpeg %s)\n", info->name, info->pid,
           info->support_jpeg ? "yes" : "no");

    while (true) {
        camera_fb_t *fb = pico_camera_fb_get();
        if (!fb) {
            printf("capture failed\n");
            continue;
        }

        printf("frame %zux%zu, %zu bytes", fb->width, fb->height, fb->len);
        if (fb->format == PIXFORMAT_JPEG) {
            bool ok = fb->buf[0] == 0xFF && fb->buf[1] == 0xD8 &&
                      fb->buf[fb->len - 2] == 0xFF && fb->buf[fb->len - 1] == 0xD9;
            printf(", jpeg soi/eoi %s", ok ? "ok" : "BROKEN");
        }
        printf("\n");

        pico_camera_fb_return(fb);
    }
}
