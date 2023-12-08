#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2405Device : public DallasDevice, public DallasPinComponent {
 public:
  bool setup_sensor() override;
  bool is_supported(uint8_t *address8) override;
  void dump_config() override;

 protected:
  bool current_state_;
  bool current_ctrl_;

  // gpio pin component
  bool digital_read_(uint8_t pin) override;
  void digital_write_(uint8_t pin, bool value) override;
  void pin_mode_(uint8_t pin, gpio::Flags flags) override;

  bool set_state(bool state);

  // device access
  bool toggle_pin();
  bool search(bool active);
};

class DS2405Switch : public DS2405Device, public switch_::Switch {
 public:
  void dump_config() override;
  bool setup_sensor() override;
  void write_state(bool state) override;
};

class DS2405Component : public DS2405Device, public PollingComponent {
 public:
  float get_setup_priority() const override;
  void dump_config() override;
  void update() override;

  void notify_alerting() override;
  void set_alert_activity(bool f);
 protected:
  bool alert_activity_{false};

};

class DS2405GPIOPin : public DallasGPIOPin {};

}
}
