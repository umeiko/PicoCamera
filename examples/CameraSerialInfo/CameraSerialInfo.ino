/**
 * CameraSerialInfo - detect the connected sensor and print its info.
 *
 * Wiring (adjust to your board):
 *   XCLK=GP5  SIOD=GP12  SIOC=GP13  RESET=GP29  PWDN=NC
 *   VSYNC=GP2  HREF=GP3  PCLK=GP4
 *   D0..D7 = GP14..GP21 (sensor Y2..Y9, must be consecutive)
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
    .frame_size    = FRAMESIZE_QVGA,
    .jpeg_quality  = 10,
    .fb_count      = 1,
};

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("PicoCamera - CameraSerialInfo");

    pico_camera_err_t err = pico_camera_init(&config);
    if (err != PICO_CAMERA_OK) {
        Serial.printf("pico_camera_init failed: %d\n", err);
        return;
    }

    const camera_sensor_info_t *info = pico_camera_sensor_info_get();
    Serial.printf("Sensor: %s (PID 0x%04X)\n", info->name, info->pid);
    Serial.printf("Resolution: %ux%u\n",
                  (unsigned)resolution[config.frame_size].width,
                  (unsigned)resolution[config.frame_size].height);
    Serial.printf("JPEG support: %s\n", info->support_jpeg ? "yes" : "no");
}

void loop() {
}
