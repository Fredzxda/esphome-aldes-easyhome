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
static constexpr uint32_t MODE_PREF_KEY = 0xA1DE5E01;

void AldesIBus::set_corrected_humidity_enabled(bool enabled) {
  if (corrected_humidity_enabled_ == enabled) return;
  corrected_humidity_enabled_ = enabled;
  publish_room_humidities_();
}

void AldesIBus::publish_room_humidities_() {
  for (size_t room = 0; room < 3; ++room) {
    auto *sensor = room_sensors_[room * 2 + 1];
    if (!sensor || !raw_humidity_valid_[room]) continue;
    float humidity = raw_humidity_[room];
    if (corrected_humidity_enabled_ && room_temperature_valid_[room] &&
        room_temperature_[room] > 20.0f) {
      const double t = room_temperature_[room];
      const double factor = std::exp(32.770308 * (20.0 / 501.1436 - t / (481.1436 + t)));
      humidity = static_cast<float>(humidity * factor);
    }
    sensor->publish_state(humidity);
  }
}

void AldesIBus::dump_config() {
  ESP_LOGCONFIG(TAG, "Aldes iBUS transceiver (periodic mode polling, 10s)");
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

void AldesIBus::queue_mode(uint8_t mode) {
  switch (mode) {
    case 0x10: case 0x21: case 0x43: case 0x65: case 0x87:
      this->pending_mode_ = mode;
      this->mode_pending_ = true;  // latest selection wins
      if (!this->mode_pref_.save(&mode)) {
        ESP_LOGW(TAG, "Failed to persist requested VMC mode 0x%02X", mode);
      }
      ESP_LOGI(TAG, "Queued VMC mode 0x%02X", mode);
      break;
    default:
      ESP_LOGW(TAG, "Unsupported VMC mode 0x%02X", mode);
  }
}

void AldesIBus::setup() {
  this->mode_pref_ = global_preferences->make_preference<uint8_t>(MODE_PREF_KEY);
  uint8_t saved_mode = 0x43;  // Daily on first boot or invalid preference
  const bool restored = this->mode_pref_.load(&saved_mode);
  const bool valid = saved_mode == 0x10 || saved_mode == 0x21 ||
                     saved_mode == 0x43 || saved_mode == 0x65 ||
                     saved_mode == 0x87;
  this->pending_mode_ = (restored && valid) ? saved_mode : 0x43;
  this->mode_pending_ = true;
  ESP_LOGI(TAG, "Startup: %s VMC mode 0x%02X queued",
           (restored && valid) ? "restored" : "default Daily",
           this->pending_mode_);
  this->publish_counters_();
  if (this->connected_sensor_) this->connected_sensor_->publish_state(false);
  if (this->rx_led_) this->rx_led_->turn_off();
}

void AldesIBus::publish_counters_() {
  if (this->frames_ok_sensor_) this->frames_ok_sensor_->publish_state(this->frames_ok_);
  if (this->checksum_bad_sensor_) this->checksum_bad_sensor_->publish_state(this->checksum_bad_);
  if (this->sync_errors_sensor_) this->sync_errors_sensor_->publish_state(this->sync_errors_);
  if (this->rx_bytes_sensor_) this->rx_bytes_sensor_->publish_state(this->rx_bytes_);
}

void AldesIBus::loop() {
  uint8_t byte;
  while (this->available()) {
    if (!this->read_byte(&byte)) break;
    this->buffer_.push_back(byte);
    this->rx_bytes_++;
    this->last_byte_ms_ = millis();
  }
  if (this->buffer_.size() > MAX_BUFFER) {
    this->buffer_.clear();
    this->sync_errors_++;
    ESP_LOGW(TAG, "Receive buffer overflow");
  }
  this->parse_();
  if (this->rx_led_active_ && millis() - this->rx_led_on_ms_ >= 1000) {
    this->rx_led_active_ = false;
    if (this->rx_led_) this->rx_led_->turn_off();
  }
  const bool connected = this->ibus_connected();
  if (this->connected_sensor_ && connected != this->connected_published_) {
    this->connected_published_ = connected;
    this->connected_sensor_->publish_state(connected);
  }
  // Queue a refresh of the last successfully transmitted mode every 10 s.
  // The initial restored (or default Daily) command is queued in setup().
  if (!this->mode_pending_ && this->has_sent_mode_ &&
      millis() - this->last_tx_ms_ >= MODE_POLL_INTERVAL_MS) {
    this->pending_mode_ = this->last_sent_mode_;
    this->mode_pending_ = true;
  }
  // Transmit only when the UART has no unread data and the bus has been
  // quiet for at least 250 ms. A pending partial frame blocks TX.
  // This is an idle-gap heuristic, not physical bus arbitration.
  if (this->mode_pending_ && !this->available() && this->buffer_.empty() &&
      millis() - this->last_byte_ms_ >= 250 &&
      millis() - this->last_tx_ms_ >= 500) {
    uint8_t frame[] = {0xFD,0x87,0x11,0x4B,0x02,0x02,0x00,0x00,0x00,
                       0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x4A,0x00};
    frame[8] = this->pending_mode_;
    uint8_t sum = 0;
    for (size_t i = 0; i < 17; ++i) sum += frame[i];
    frame[17] = static_cast<uint8_t>(-sum);
    this->mode_pending_ = false;
    this->write_array(frame, sizeof(frame));
    this->flush();
    this->last_tx_ms_ = millis();
    this->last_sent_mode_ = frame[8];
    this->has_sent_mode_ = true;
    ESP_LOGD(TAG, "VMC mode command sent: 0x%02X, checksum 0x%02X",
             frame[8], frame[17]);
  }
  if (millis() - this->last_publish_ms_ >= 5000) {
    this->last_publish_ms_ = millis();
    this->publish_counters_();
  }
}

void AldesIBus::parse_() {
  while (!this->buffer_.empty()) {
    // Accept both iBUS directions: AldesBox/ESP32 echo FD 87,
    // and VMC response 87 00. Length is byte 2 + 1 in both.
    if (this->buffer_.size() < 2) break;
    const bool tx_echo = this->buffer_[0] == 0xFD && this->buffer_[1] == 0x87;
    const bool vmc_frame = this->buffer_[0] == 0x87 && this->buffer_[1] == 0x00;
    if (!tx_echo && !vmc_frame) {
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
    if (tx_echo) {
      // Valid outgoing frame received back through the transceiver.
      // Do not count it as a VMC reply.
      ESP_LOGD(TAG, "iBUS TX echo (%u bytes): %s", (unsigned) size, hex.c_str());
    } else {
      this->frames_ok_++;
      this->has_valid_frame_ = true;
      this->last_valid_frame_ms_ = millis();
      this->rx_led_on_ms_ = millis();
      this->rx_led_active_ = true;
      if (this->rx_led_) this->rx_led_->turn_on();
      // Standard 28-byte VMC status: CO2 at offsets 20-21 (LE),
      // humidity variation (VarHR) at offset 23.
      if (size == 28 && this->buffer_[3] == 0x4B &&
          this->buffer_[4] == 0x03 && this->buffer_[5] == 0x01) {
        const uint16_t co2 = uint16_t(this->buffer_[20]) |
                             (uint16_t(this->buffer_[21]) << 8);
        const uint8_t varhr = this->buffer_[23];
        if (this->co2_sensor_) this->co2_sensor_->publish_state(co2);
        if (this->varhr_sensor_) this->varhr_sensor_->publish_state(varhr);
        const uint16_t pwm1 = uint16_t(this->buffer_[8]) |
                              (uint16_t(this->buffer_[9]) << 8);
        const uint16_t pwm2 = uint16_t(this->buffer_[24]) |
                              (uint16_t(this->buffer_[25]) << 8);
        if (this->pwm1_sensor_) this->pwm1_sensor_->publish_state(pwm1);
        if (this->pwm2_sensor_) this->pwm2_sensor_->publish_state(pwm2);
        if (this->self_controlled_sensor_) this->self_controlled_sensor_->publish_state(pwm2 >= 750);
        // Byte 11 (zero-based offset 10) is the reported VMC mode.
        // Byte 10 (offset 9) is PWM1's high byte in this frame.
        const uint8_t actual_mode = this->buffer_[10];
        const char *mode_label = nullptr;
        switch (actual_mode) {
          case 0x10: mode_label = "Programmation"; break;
          case 0x21: mode_label = "Vacances"; break;
          case 0x43: mode_label = "Quotidien"; break;
          case 0x65: mode_label = "Boost"; break;
          case 0x87: mode_label = "Invites"; break;
          default: break;
        }
        if (this->actual_mode_sensor_) {
          if (mode_label) {
            this->actual_mode_sensor_->publish_state(mode_label);
          } else {
            char unknown[16];
            snprintf(unknown, sizeof(unknown), "Inconnu 0x%02X", actual_mode);
            this->actual_mode_sensor_->publish_state(unknown);
          }
        }
        // Standard frame byte positions 15-20 (one-based):
        // temperature encoded with -6 C offset plus quarter degrees, humidity as integer percent.
        for (size_t room = 0; room < 3; ++room) {
          const size_t offset = 14 + room * 2;
          // Preserve raw iBUS values independently of the HA display selection.
          // QAI and all internal humidity calculations use raw_humidity_.
          const uint8_t encoded = this->buffer_[offset];
          const float temperature = float(encoded >> 2) - 6.0f +
                                    float(encoded & 0x03) * 0.25f;
          this->room_temperature_[room] = temperature;
          this->room_temperature_valid_[room] = true;
          this->raw_humidity_[room] = this->buffer_[offset + 1];
          this->raw_humidity_valid_[room] = true;
          if (this->room_sensors_[room * 2])
            this->room_sensors_[room * 2]->publish_state(temperature);
        }
        this->publish_room_humidities_();
      }
      ESP_LOGD(TAG, "VMC frame (%u bytes): %s", (unsigned) size, hex.c_str());
      // Expose only the VMC response, never the locally echoed TX command.
      if (this->raw_rx_sensor_) this->raw_rx_sensor_->publish_state(hex);
      if (size == 54 && this->last_long_frame_sensor_) {
        // Retain only complete, checksum-valid 54-byte VMC frames.
        this->last_long_frame_sensor_->publish_state(hex);
      }
      if (this->last_frame_sensor_) {
        // HA states have a 255-character limit.
        this->last_frame_sensor_->publish_state(hex.substr(0, 240));
      }
    }
    this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + size);
  }
}

}  // namespace aldes_ibus
}  // namespace esphome
