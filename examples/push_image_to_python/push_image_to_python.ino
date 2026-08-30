/*
 * This file is part of the PicoCamera project.
 * https://github.com/umeiko/PicoCamera
 *
 * Author: umeko <umeko@stu.xmu.edu.cn>
 * License: MIT
 */

/**
 * push_image_to_python - stream camera frames over USB serial.
 *
 * Protocol (binary):
 *   RGB565 frame:  "SRGB" + width*height*2 raw bytes + "ERGB"
 *   JPEG frame:    "SJPG" + raw JPEG bytes (FFD8...FFD9) + "EJPG"
 *   YUV422 frame:  "SYUV" + width*height*2 packed YUYV bytes + "EYUV"
 *   GRAYSCALE:     "SGRY" + width*height raw Y bytes + "EGRY"
 *
 * Pick the format with the PUSH_FORMAT macro below, then run
 * push_image_to_python.py on the PC to view the stream.
 *
 * Note on frame rate: capture and USB transmission run back-to-back in
 * loop(), so the viewer fps is bounded by the link throughput. This USB
 * CDC link (Arduino-Pico Serial + pyserial) measures ~320 KB/s, i.e.
 * viewer fps ≈ 320 / frame size in KB:
 *
 *   format     | QVGA 320x240      | QQVGA 160x120
 *   ---------  | ----------------  | --------------
 *   RGB565     | ~150KB,  ~2 fps   | ~38KB,   ~8 fps
 *   YUV422     | ~150KB,  ~2 fps   | ~38KB,   ~8 fps
 *   GRAYSCALE  |  ~75KB,  ~4 fps   | ~19KB,  ~16 fps
 *   JPEG       | ~10-20KB, no longer transport-bound
 *
 * Lower the frame_size (or use JPEG) if you want a smoother stream.
 *
 * Wiring: see CameraSerialInfo.
 * The python receiver script: https://github.com/umeiko/PicoCamera/blob/main/examples/push_image_to_python/push_image_to_python.py
 */

#include <PicoCamera.h>

// 0 = push RGB565 (SRGB...ERGB)
// 1 = push JPEG   (SJPG...EJPG)
// 2 = push YUV422 (SYUV...EYUV)
// 3 = push GRAYSCALE (SGRY...EGRY)
#define PUSH_FORMAT 1

static camera_config_t config = {
  .pin_pwdn     = -1,
  .pin_reset    = -1,   // no hardware reset line; the driver soft-resets via SCCB
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
#if PUSH_FORMAT == 1
  .pixel_format  = PIXFORMAT_JPEG,
#elif PUSH_FORMAT == 2
  .pixel_format  = PIXFORMAT_YUV422,
#elif PUSH_FORMAT == 3
  .pixel_format  = PIXFORMAT_GRAYSCALE,
#else
  .pixel_format  = PIXFORMAT_RGB565,
#endif
  .frame_size    = FRAMESIZE_QVGA,   // 320x240
  .jpeg_quality  = 10,
  .fb_count      = 1,
  .fb_location   = PICO_CAMERA_FB_AUTO,
};

void setup() {
  Serial.begin(115200);
  pico_camera_err_t err = pico_camera_init(&config);
  if (err != PICO_CAMERA_OK) {
    while (true) {
      Serial.printf("pico_camera_init failed: %s (%d)\n", pico_camera_err_str(err), err);
      delay(1000);
    }
  }
}

void loop() {
  camera_fb_t *fb = pico_camera_fb_get();
  if (fb) {
#if PUSH_FORMAT == 1
    Serial.write("SJPG", 4);
#elif PUSH_FORMAT == 2
    Serial.write("SYUV", 4);
#elif PUSH_FORMAT == 3
    Serial.write("SGRY", 4);
#else
    Serial.write("SRGB", 4);
#endif
    Serial.write(fb->buf, fb->len);
#if PUSH_FORMAT == 1
    Serial.write("EJPG", 4);
#elif PUSH_FORMAT == 2
    Serial.write("EYUV", 4);
#elif PUSH_FORMAT == 3
    Serial.write("EGRY", 4);
#else
    Serial.write("ERGB", 4);
#endif
    Serial.flush();  // wait until the whole frame left the FIFO
    pico_camera_fb_return(fb);
  }
  delay(10);
}
