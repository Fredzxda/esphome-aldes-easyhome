#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace esphome {
namespace aldes_ibus {

class AldesIBus : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void send_poll();
  void send_boost();
  void queue_mode(uint8_t mode);
  void set_corrected_humidity_enabled(bool enabled);
  void publish_room_humidities_();
  uint8_t get_requested_mode() const { return pending_mode_; }
  void set_frames_ok_sensor(sensor::Sensor *s) { frames_ok_sensor_ = s; }
  void set_checksum_bad_sensor(sensor::Sensor *s) { checksum_bad_sensor_ = s; }
  void set_sync_errors_sensor(sensor::Sensor *s) { sync_errors_sensor_ = s; }
  void set_rx_bytes_sensor(sensor::Sensor *s) { rx_bytes_sensor_ = s; }
  void set_raw_rx_sensor(text_sensor::TextSensor *s) { raw_rx_sensor_ = s; }
  void set_last_frame_sensor(text_sensor::TextSensor *s) { last_frame_sensor_ = s; }
  void set_last_long_frame_sensor(text_sensor::TextSensor *s) { last_long_frame_sensor_ = s; }
  void set_co2_sensor(sensor::Sensor *s) { co2_sensor_ = s; }
  void set_varhr_sensor(sensor::Sensor *s) { varhr_sensor_ = s; }
  void set_room_sensor(size_t index, sensor::Sensor *s) { if (index < 6) room_sensors_[index] = s; }
  void set_pwm1_sensor(sensor::Sensor *s) { pwm1_sensor_ = s; }
  void set_pwm2_sensor(sensor::Sensor *s) { pwm2_sensor_ = s; }
  void set_self_controlled_sensor(binary_sensor::BinarySensor *s) { self_controlled_sensor_ = s; }
  void set_actual_mode_sensor(text_sensor::TextSensor *s) { actual_mode_sensor_ = s; }
  void set_connected_sensor(binary_sensor::BinarySensor *s) { connected_sensor_ = s; }
  void set_rx_led(output::BinaryOutput *o) { rx_led_ = o; }
  bool ibus_connected() const { return has_valid_frame_ && (millis() - last_valid_frame_ms_ < MODE_POLL_INTERVAL_MS + 2000); }
  // Empirical reconstruction of the historical Aldes QAI; no smoothing.
  // Wait for all probes to have published before calculating humidity QAI.
  float qai_humidity_wet() const {
    if (!room_sensors_[1] || !room_sensors_[3] || !room_sensors_[5] ||
        ! !raw_humidity_valid_[0] || !raw_humidity_valid_[1] ||
        !raw_humidity_valid_[2]) return NAN;
    const float maximum = std::max(raw_humidity_[0],
                          std::max(raw_humidity_[1], raw_humidity_[2]));
    const int h = static_cast<int>(std::floor(maximum));
    if (h <= 50) return 0;
    if (h <= 60) return (5 * (h - 50)) / 2;
    if (h == 61) return 25;
    if (h <= 70) return 25 + (25 * (h - 61)) / 9;
    if (h == 71) return 50;
    if (h <= 75) return 50 + (25 * (h - 71)) / 4;
    return std::min(100, h - 1);
  }
  float qai_humidity_dry() const {
    if (!room_sensors_[1] || !room_sensors_[3] || !room_sensors_[5] ||
        !room_sensors_[1]->has_state() || !room_sensors_[3]->has_state() ||
        !room_sensors_[5]->has_state()) return NAN;
    const float minimum = std::min(raw_humidity_[0],
                          std::min(raw_humidity_[1], raw_humidity_[2]));
    return std::min(100.0f, std::max(0.0f, std::floor(2.5f * (50.0f - minimum))));
  }
  float qai_co2() const {
    if (!co2_sensor_ || !co2_sensor_->has_state()) return NAN;
    const float co2 = co2_sensor_->state;
    float value = 0;
    if (co2 <= 400) value = 0;
    else if (co2 <= 1200) value = (co2 - 400) / 16.0f;
    else if (co2 <= 1400) value = 50.0f + (co2 - 1200) / 8.0f;
    else value = 75.0f + (co2 - 1400) / 24.0f;
    return std::min(100.0f, std::max(0.0f, std::ceil(value) - 1.0f));
  }
  float qai_humidity() const {
    const float wet = qai_humidity_wet(), dry = qai_humidity_dry();
    if (std::isnan(wet) || std::isnan(dry)) return NAN;
    return std::max(wet, dry);
  }
  float qai_index() const {
    const float humidity = qai_humidity(), co2 = qai_co2();
    if (std::isnan(humidity) || std::isnan(co2)) return NAN;
    return std::min(100.0f, std::max(humidity, co2));
  }
  void dump_config() override;
 protected:
  static constexpr uint32_t MODE_POLL_INTERVAL_MS = 10000;
  std::vector<uint8_t> buffer_;
  binary_sensor::BinarySensor *connected_sensor_{nullptr};
  output::BinaryOutput *rx_led_{nullptr};
  uint32_t last_valid_frame_ms_{0};
  uint32_t rx_led_on_ms_{0};
  bool has_valid_frame_{false};
  bool connected_published_{false};
  bool rx_led_active_{false};
  uint32_t last_byte_ms_{0};
  uint32_t last_tx_ms_{0};
  uint8_t pending_mode_{0};
  bool mode_pending_{false};
  ESPPreferenceObject mode_pref_;
  bool has_sent_mode_{false};
  uint8_t last_sent_mode_{0};
  uint32_t rx_bytes_{0};
  uint32_t last_publish_ms_{0};
  sensor::Sensor *frames_ok_sensor_{nullptr};
  sensor::Sensor *checksum_bad_sensor_{nullptr};
  sensor::Sensor *sync_errors_sensor_{nullptr};
  sensor::Sensor *rx_bytes_sensor_{nullptr};
  text_sensor::TextSensor *last_frame_sensor_{nullptr};
  text_sensor::TextSensor *last_long_frame_sensor_{nullptr};
  text_sensor::TextSensor *raw_rx_sensor_{nullptr};
  sensor::Sensor *co2_sensor_{nullptr};
  sensor::Sensor *varhr_sensor_{nullptr};
  sensor::Sensor *room_sensors_[6]{};
  float raw_humidity_[3]{};
  float room_temperature_[3]{};
  bool raw_humidity_valid_[3]{};
  bool room_temperature_valid_[3]{};
  bool corrected_humidity_enabled_{false};
  sensor::Sensor *pwm1_sensor_{nullptr};
  sensor::Sensor *pwm2_sensor_{nullptr};
  binary_sensor::BinarySensor *self_controlled_sensor_{nullptr};
  text_sensor::TextSensor *actual_mode_sensor_{nullptr};
  uint32_t frames_ok_{0};
  uint32_t checksum_bad_{0};
  uint32_t sync_errors_{0};
  void parse_();
  void publish_counters_();
};

}  // namespace aldes_ibus
}  // namespace esphome
