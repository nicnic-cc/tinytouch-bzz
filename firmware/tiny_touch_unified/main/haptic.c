#include "haptic.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Drives the vibration module's IN pin. Free on both the ESP32-S3 Super Mini
// and the Seeed XIAO ESP32-S3 (D3).
static const int HAPTIC_PIN = 4;

void haptic_buzz(uint32_t on_ms) {
  gpio_set_level(HAPTIC_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(on_ms));
  gpio_set_level(HAPTIC_PIN, 0);
}

void haptic_init(void) {
  gpio_config_t io = {
    .pin_bit_mask = 1ULL << HAPTIC_PIN,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io));
  gpio_set_level(HAPTIC_PIN, 0);
}
