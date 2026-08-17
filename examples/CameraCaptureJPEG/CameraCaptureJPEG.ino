/*
 * This file is part of the PicoCamera project.
 * https://github.com/umeiko/PicoCamera
 *
 * Author: umeko <umeko@stu.xmu.edu.cn>
 * License: MIT
 */

/**
 * CameraCaptureJPEG - capture JPEG frames and print size / marker info.
 *
 * The captured buffer is a complete JPEG file (starts with 0xFFD8,
 * ends with 0xFFD9) - e.g. stream it over serial or save to SD.
 *
 * Wiring: see CameraSerialInfo.
 */

#include <PicoCamera.h>

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
  .pixel_format  = PIXFORMAT_JPEG,
  .frame_size    = FRAMESIZE_QVGA,
  .jpeg_quality  = 10,
  .fb_count      = 1,
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("PicoCamera - CameraCaptureJPEG");

  pico_camera_err_t err = pico_camera_init(&config);
  if (err != PICO_CAMERA_OK) {
    Serial.printf("pico_camera_init failed: %d\n", err);
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  camera_fb_t *fb = pico_camera_fb_get();
  if (!fb) {
    Serial.println("capture failed");
    delay(500);
    return;
  }

  bool soi = fb->len >= 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
  bool eoi = fb->len >= 2 && fb->buf[fb->len - 2] == 0xFF && fb->buf[fb->len - 1] == 0xD9;

  Serial.printf("jpeg %ux%u len=%u SOI=%d EOI=%d\n",
                (unsigned)fb->width, (unsigned)fb->height,
                (unsigned)fb->len, (int)soi, (int)eoi);

  pico_camera_fb_return(fb);
  delay(500);
}
