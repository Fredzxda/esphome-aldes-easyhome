#include "aldes_ibus.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/application.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include <cstdio>

namespace esphome {
namespace aldes_ibus {

static const char *const TAG = "aldes_ibus";
static constexpr size_t MAX_BUFFER = 1024;
static constexpr uint32_t FRAME_TIMEOUT_MS = 500;

void AldesIBus::dump_config() {
  ESP_LOGCONFIG(TAG, "Aldes iBUS receiver (RX only)");
}

void AldesIBus::loop() {
  uint8_t byte;
  while (this->available()) {
    if (!this->read_byte(&byte)) break;
    this->buffer_.push_back(byte);
    this->last_byte_ms_ = millis();
  }
  if (this->buffer_.size() > MAX_BUFFER) {
    this->buffer_.clear();
    this->sync_errors_++;
    ESP_LOGW(TAG, "Receive buffer overflow");
  }
  this->parse_();
}

void AldesIBus::parse_() {
  while (!this->buffer_.empty()) {
    if (this->buffer_[0] != 0x87) {
      this->buffer_.erase(this->buffer_.begin());
      this->sync_errors_++;
      continue;
    }
    if (this->buffer_.size() < 2) break;
    if (this->buffer_[1] != 0x00) {
      this->buffer_.erase(this->buffer_.begin());
      this->sync_errors_++;
      continue;
    }
    if (this->buffer_.size() < 3) break;
    const size_t size = static_cast<size_t>(this->buffer_[2]) + 1;
    if (size < 5 || size > 256) {
      this->buffer_.erase(this->buffer_.begin());
      this->sync_errors_++;
      continue;
    }
    if (this->buffer_.size() < size) {
      if (millis() - this->last_byte_ms_ > FRAME_TIMEOUT_MS) {
        this->buffer_.erase(this->buffer_.begin());
        this->sync_errors_++;
        continue;
      }
      break;
    }
    uint8_t sum = 0;
    for (size_t i = 0; i < size; i++) sum += this->buffer_[i];
    if (sum != 0) {
      this->checksum_bad_++;
      this->buffer_.erase(this->buffer_.begin());
      continue;
    }
    std::string hex;
    char tmp[4];
    for (size_t i = 0; i < size; i++) {
      if (i) hex += ' ';
      snprintf(tmp, sizeof(tmp), "%02X", this->buffer_[i]);
      hex += tmp;
    }
    this->frames_ok_++;
    ESP_LOGI(TAG, "VMC frame (%u bytes): %s", (unsigned) size, hex.c_str());
    // Sensor publication will be wired in the next development commit.
    this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + size);
  }
}

}  // namespace aldes_ibus
}  // namespace esphome
