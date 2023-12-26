#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2413Switch;

class DS2413Device : public DallasDevice, public DallasPinComponent {
 public:
  bool setup_sensor() override;
  bool is_supported(uint8_t *address8) override;
  void dump_config() override;

 protected:
  friend DS2413Switch;

  uint8_t current_state_;
  uint8_t current_latch_;

  // gpio pin component
  bool digital_read_(uint8_t pin) override;
  void digital_write_(uint8_t pin, bool value) override;
  void pin_mode_(uint8_t pin, gpio::Flags flags) override;

  // device access
  optional<uint8_t> pio_access_read();
  optional<uint8_t> pio_access_write(uint8_t data);
};

class DS2413Component : public DS2413Device, public PollingComponent {
 public:
  float get_setup_priority() const override;
  void dump_config() override;
  void update() override;
 protected:
};

class DS2413Switch : public switch_::Switch {
 public:
//  void dump_config() override;
//  bool setup_sensor() override;
  void write_state(bool state) override;

  void set_parent(DS2413Device *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
 protected:
  DS2413Device *parent_;
  uint8_t pin_;
};

class DS2413GPIOPin : public DallasGPIOPin {};

}
}
