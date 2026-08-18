/*
 * This file is part of the PicoCamera project.
 * https://github.com/umeiko/PicoCamera
 *
 * Author: umeko <umeko@stu.xmu.edu.cn>
 * License: MIT
 */

/**
 * CameraCaptureRGB565 - capture RGB565 frames and print frame stats.
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
  .pixel_format  = PIXFORMAT_RGB565,
  .frame_size    = FRAMESIZE_QVGA,   // 320x240
  .jpeg_quality  = 10,
  .fb_count      = 1,
  .fb_location   = PICO_CAMERA_FB_AUTO,
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("PicoCamera - CameraCaptureRGB565");

  pico_camera_err_t err = pico_camera_init(&config);
  if (err != PICO_CAMERA_OK) {
    Serial.printf("pico_camera_init failed: %d\n", err);
    while (true) {
      delay(1000);
    }
  }

  // Example: runtime sensor control, esp32-camera style
  sensor_t *s = pico_camera_sensor_get();
  if (s) {
    if (s->set_vflip) {
      s->set_vflip(s, 0);
    }
    if (s->set_hmirror) {
      s->set_hmirror(s, 0);
    }
  }
}

void loop() {
  uint32_t t0 = millis();
  camera_fb_t *fb = pico_camera_fb_get();
  uint32_t t1 = millis();

  if (!fb) {
    Serial.println("capture failed");
    delay(500);
    return;
  }

  // Simple checksum so you can see the frame content changing
  uint32_t sum = 0;
  for (size_t i = 0; i < fb->len; i += 64) {
    sum += fb->buf[i];
  }

  Serial.printf("frame %ux%u len=%u captured in %ums checksum=%u\n",
                (unsigned)fb->width, (unsigned)fb->height,
                (unsigned)fb->len, (unsigned)(t1 - t0), (unsigned)sum);

  pico_camera_fb_return(fb);
  delay(500);
}
