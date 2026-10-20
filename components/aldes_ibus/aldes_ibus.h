#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <vector>
#include <cstdint>

namespace esphome {
namespace aldes_ibus {

class AldesIBus : public Component, public uart::UARTDevice {
 public:
  void loop() override;
  void dump_config() override;
 protected:
  std::vector<uint8_t> buffer_;
  uint32_t last_byte_ms_{0};
  uint32_t frames_ok_{0};
  uint32_t checksum_bad_{0};
  uint32_t sync_errors_{0};
  void parse_();
};

}  // namespace aldes_ibus
}  // namespace esphome
