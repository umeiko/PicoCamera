/**
 * push_image_to_python - stream camera frames over USB serial.
 *
 * Protocol (binary):
 *   RGB565 frame:  "SRGB" + width*height*2 raw bytes + "ERGB"
 *   JPEG frame:    "SJPG" + raw JPEG bytes (FFD8...FFD9) + "EJPG"
 *
 * Pick the format with the PUSH_FORMAT macro below, then run
 * push_image_to_python.py on the PC to view the stream.
 *
 * Wiring: see CameraSerialInfo.
 */

#include <PicoCamera.h>

// 0 = push RGB565 (SRGB...ERGB)
// 1 = push JPEG   (SJPG...EJPG)
#define PUSH_FORMAT 1

static camera_config_t config = {
    .pin_pwdn     = -1,
    .pin_reset    = 29,
    .pin_xclk     = 5,
    .pin_sccb_sda = 12,
    .pin_sccb_scl = 13,
    .pin_d0 = 14, .pin_d1 = 15, .pin_d2 = 16, .pin_d3 = 17,
    .pin_d4 = 18, .pin_d5 = 19, .pin_d6 = 20, .pin_d7 = 21,
    .pin_vsync = 2,
    .pin_href  = 3,
    .pin_pclk  = 4,
    .xclk_freq_hz  = 10000000,
    .sccb_i2c_port = 0,
#if PUSH_FORMAT
    .pixel_format  = PIXFORMAT_JPEG,
#else
    .pixel_format  = PIXFORMAT_RGB565,
#endif
    .frame_size    = FRAMESIZE_QVGA,   // 320x240
    .jpeg_quality  = 10,
    .fb_count      = 1,
};

void setup() {
    Serial.begin(115200);
    pico_camera_err_t err = pico_camera_init(&config);
    if (err != PICO_CAMERA_OK) {
        while (true) {
            Serial.printf("pico_camera_init failed: %d\n", err);
            delay(1000);
        }
    }
}

void loop() {
    camera_fb_t *fb = pico_camera_fb_get();
    if (fb) {
#if PUSH_FORMAT
        Serial.write("SJPG", 4);
#else
        Serial.write("SRGB", 4);
#endif
        Serial.write(fb->buf, fb->len);
#if PUSH_FORMAT
        Serial.write("EJPG", 4);
#else
        Serial.write("ERGB", 4);
#endif
        Serial.flush();  // wait until the whole frame left the FIFO
        pico_camera_fb_return(fb);
    }
    delay(10);
}
