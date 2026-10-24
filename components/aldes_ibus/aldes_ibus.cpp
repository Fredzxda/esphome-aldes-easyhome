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
  ESP_LOGCONFIG(TAG, "Aldes iBUS receiver (manual poll)");
}

void AldesIBus::send_poll() {
  // Legacy diagnostic poll retained for reference. Not used by the Boost button.
  static const uint8_t poll[] = {0xFD,0x87,0x11,0x4B,0x02,0x02,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x4A,0x19};
  this->write_array(poll, sizeof(poll));
  this->flush();
  ESP_LOGI(TAG, "Legacy manual poll sent (unvalidated)");
}

void AldesIBus::send_boost() {
  // Captured from the original AldesBox: mode 0x65 (Boost).
  // Sum of all 18 bytes, including checksum 0x72, is 0 modulo 256.
  static const uint8_t boost[] = {0xFD,0x87,0x11,0x4B,0x02,0x02,0x00,0x00,0x65,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x4A,0x72};
  this->write_array(boost, sizeof(boost));
  this->flush();
  ESP_LOGI(TAG, "AldesBox Boost command sent (18 bytes, checksum 0x72)");
}

void AldesIBus::setup() {
  this->publish_counters_();
}

void AldesIBus::publish_counters_() {
  if (this->frames_ok_sensor_) this->frames_ok_sensor_->publish_state(this->frames_ok_);
  if (this->checksum_bad_sensor_) this->checksum_bad_sensor_->publish_state(this->checksum_bad_);
  if (this->sync_errors_sensor_) this->sync_errors_sensor_->publish_state(this->sync_errors_);
  if (this->rx_bytes_sensor_) this->rx_bytes_sensor_->publish_state(this->rx_bytes_);
}

void AldesIBus::publish_raw_sample_() {
  if (this->raw_sample_.empty()) return;
  std::string hex;
  char tmp[4];
  for (uint8_t byte : this->raw_sample_) {
    if (!hex.empty()) hex += ' ';
    snprintf(tmp, sizeof(tmp), "%02X", byte);
    hex += tmp;
  }
  ESP_LOGI(TAG, "Raw RX sample (%u bytes): %s", (unsigned) this->raw_sample_.size(), hex.c_str());
  if (this->raw_rx_sensor_) this->raw_rx_sensor_->publish_state(hex);
  this->raw_sample_.clear();
}

void AldesIBus::loop() {
  uint8_t byte;
  while (this->available()) {
    if (!this->read_byte(&byte)) break;
    this->buffer_.push_back(byte);
    this->rx_bytes_++;
    if (this->raw_sample_.size() < 64) this->raw_sample_.push_back(byte);
    this->last_byte_ms_ = millis();
  }
  if (this->buffer_.size() > MAX_BUFFER) {
    this->buffer_.clear();
    this->sync_errors_++;
    ESP_LOGW(TAG, "Receive buffer overflow");
  }
  this->parse_();
  if (millis() - this->last_publish_ms_ >= 5000) {
    this->last_publish_ms_ = millis();
    this->publish_counters_();
    this->publish_raw_sample_();
  }
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
    if (this->last_frame_sensor_) {
      // HA states have a 255-character limit. Publish a short prefix;
      // the complete frame remains available in ESPHome logs.
      this->last_frame_sensor_->publish_state(hex.substr(0, 240));
    }
    this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + size);
  }
}

}  // namespace aldes_ibus
}  // namespace esphome
