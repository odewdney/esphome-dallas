#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

/// Internal class that helps us create multiple sensors for one Dallas hub.
class DallasTemperatureSensor : public DallasSensor {
 public:
  /// Get the set resolution for this sensor.
  uint8_t get_resolution() const;
  /// Set the resolution for this sensor.
  void set_resolution(uint8_t resolution);
  /// Get the number of milliseconds we have to wait for the conversion phase.
  uint16_t millis_to_wait_for_conversion() const override;

  bool is_supported(uint8_t *address8) override;
  bool setup_sensor() override;
  void dump_config() override;
  void read_conversion() override;

  bool read_scratch_pad();

  bool check_scratch_pad();

  float get_temp_c();


 protected:
  uint8_t resolution_;
  uint8_t scratch_pad_[9] = {
      0,
  };
};
}  // namespace dallas
}  // namespace esphome
