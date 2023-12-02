#pragma once

#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2408GPIOPin;

class DS2408Component : public PollingComponent, public dallas::DallasDevice, public dallas::DallasPinComponent {
 public:
  bool is_supported(uint8_t *address8);

  /// Check dallas availability and setup masks
  void setup() override;
  float get_setup_priority() const override;
  void set_alert_activity(bool f);
  
  void update() override;
  void notify_alerting() override;
  void dump_config() override;

  void write_channel(uint8_t value);
  uint8_t read_channel();
 protected:
  friend DS2408GPIOPin;

  void update_conditional();
  void reset_activity();

  uint8_t value_;
  uint8_t read_value_;
  uint8_t pin_mode_value_;
  bool alert_activity_{false};

    /// Helper function to read the value of a pin.
  bool digital_read_(uint8_t pin) override;
  /// Helper function to write the value of a pin.
  void digital_write_(uint8_t pin, bool value) override;
  /// Helper function to set the pin mode of a pin.
  void pin_mode_(uint8_t pin, gpio::Flags flags) override;

};


class DS2408GPIOPin : public DallasGPIOPin {};

}
}
