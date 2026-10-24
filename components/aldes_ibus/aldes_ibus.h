#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>
#include <cstdint>

namespace esphome {
namespace aldes_ibus {

class AldesIBus : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void send_poll();
  void send_boost();
  void queue_mode(uint8_t mode);
  void set_frames_ok_sensor(sensor::Sensor *s) { frames_ok_sensor_ = s; }
  void set_checksum_bad_sensor(sensor::Sensor *s) { checksum_bad_sensor_ = s; }
  void set_sync_errors_sensor(sensor::Sensor *s) { sync_errors_sensor_ = s; }
  void set_rx_bytes_sensor(sensor::Sensor *s) { rx_bytes_sensor_ = s; }
  void set_raw_rx_sensor(text_sensor::TextSensor *s) { raw_rx_sensor_ = s; }
  void set_last_frame_sensor(text_sensor::TextSensor *s) { last_frame_sensor_ = s; }
  void dump_config() override;
 protected:
  std::vector<uint8_t> buffer_;
  uint32_t last_byte_ms_{0};
  uint32_t last_tx_ms_{0};
  uint8_t pending_mode_{0};
  bool mode_pending_{false};
  ESPPreferenceObject mode_pref_;
  bool has_sent_mode_{false};
  uint8_t last_sent_mode_{0};
  uint32_t rx_bytes_{0};
  uint32_t last_publish_ms_{0};
  uint32_t last_raw_publish_ms_{0};
  std::vector<uint8_t> raw_sample_;
  sensor::Sensor *frames_ok_sensor_{nullptr};
  sensor::Sensor *checksum_bad_sensor_{nullptr};
  sensor::Sensor *sync_errors_sensor_{nullptr};
  sensor::Sensor *rx_bytes_sensor_{nullptr};
  text_sensor::TextSensor *last_frame_sensor_{nullptr};
  text_sensor::TextSensor *raw_rx_sensor_{nullptr};
  uint32_t frames_ok_{0};
  uint32_t checksum_bad_{0};
  uint32_t sync_errors_{0};
  void parse_();
  void publish_counters_();
  void publish_raw_sample_();
};

}  // namespace aldes_ibus
}  // namespace esphome
