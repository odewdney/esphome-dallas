#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2409Component : public DallasDevice, public PollingComponent, public switch_::Switch, public DallasPinComponent {
 public:
  bool setup_sensor() override;
  bool is_supported(uint8_t *address8) override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;
  
  void notify_alerting() override;
  void set_alert_activity(bool f);

 protected:
  bool current_state_;
  bool current_ctrl_;
  bool alert_activity_{false};

  void write_state(bool state) override;

  // gpio pin component
  bool digital_read_(uint8_t pin) override;
  void digital_write_(uint8_t pin, bool value) override;
  void pin_mode_(uint8_t pin, gpio::Flags flags) override;

  // device access
  uint8_t status_update(uint8_t config);
  bool smart_on(bool main);
  void all_off();
};

class DS2409GPIOPin : public DallasGPIOPin {};

}
}
