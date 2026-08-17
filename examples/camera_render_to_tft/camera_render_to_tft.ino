/*
 * This file is part of the PicoCamera project.
 * https://github.com/umeiko/PicoCamera
 *
 * Author: umeko <umeko@stu.xmu.edu.cn>
 * License: MIT
 */

/**
 * camera_render_to_tft - live camera preview on a TFT display.
 *
 * Captures RGB565 frames with PicoCamera and pushes them to a TFT
 * screen via TFT_eSPI.
 *
 * Before using this sketch you must configure TFT_eSPI for your
 * display (driver chip, pins, SPI frequency). See readme.md in this
 * folder.
 *
 * Camera wiring (adjust to your board):
 *   XCLK=GP5  SIOD=GP12  SIOC=GP13  RESET=GP29  PWDN=NC
 *   VSYNC=GP2  HREF=GP3  PCLK=GP4
 *   D0..D7 = GP14..GP21 (sensor Y2..Y9, must be consecutive)
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <PicoCamera.h>

TFT_eSPI tft = TFT_eSPI();

static camera_config_t cam_config = {
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
};

// Display window: center-crop the 320x240 frame for a 280x240 screen.
// Adjust to your display.
static const int DISP_WIDTH  = 280;
static const int DISP_HEIGHT = 240;
static const int CROP_X      = (320 - 280) / 2;

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setSwapBytes(true);   // OV2640 outputs big-endian RGB565
  tft.fillScreen(TFT_BLACK);

  pico_camera_err_t err = pico_camera_init(&cam_config);
  if (err != PICO_CAMERA_OK) {
    Serial.printf("pico_camera_init failed: %d\n", err);
    tft.setTextSize(2);
    tft.setCursor(10, DISP_HEIGHT / 2 - 10);
    tft.println("NO Camera");
  }
}

void loop() {
  camera_fb_t *fb = pico_camera_fb_get();
  if (fb) {
    tft.startWrite();
    for (int y = 0; y < DISP_HEIGHT; y++) {
      const uint16_t *row = (const uint16_t *)fb->buf + y * fb->width + CROP_X;
      tft.pushImage(0, y, DISP_WIDTH, 1, (uint16_t *)row);
    }
    tft.endWrite();
    pico_camera_fb_return(fb);
  }
  delay(10);
}
