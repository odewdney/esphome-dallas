#pragma once

#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2408GPIOPin;

class DS2408Component : public PollingComponent, public dallas::DallasDevice {
 public:
  DS2408Component();

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
  bool alert_activity_;

    /// Helper function to read the value of a pin.
  bool digital_read_(uint8_t pin);
  /// Helper function to write the value of a pin.
  void digital_write_(uint8_t pin, bool value);
  /// Helper function to set the pin mode of a pin.
  void pin_mode_(uint8_t pin, gpio::Flags flags);

};


class DS2408GPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(DS2408Component *parent) { parent_ = parent; }
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

 protected:
  DS2408Component *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};


}
}
