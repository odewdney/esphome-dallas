#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2409Component;

class DS2409Network : public DallasNetwork {
 public:
    DS2409Network(DS2409Component *parent, bool main) {this->parent_=parent;this->main_=main;}

 protected:
    DS2409Component *parent_;
    bool main_;

  ESPOneWire *get_reset_one_wire_() override;
  Component *get_component() override;
  void component_set_timeout(const std::string &name, uint32_t timeout, std::function<void()> &&f) override;

};

class DS2409Component : public DallasDevice, public PollingComponent, public switch_::Switch, public DallasPinComponent {
 public:
  bool setup_sensor() override;
  bool is_supported(uint8_t *address8) override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;
  
  void notify_alerting() override;
  void set_alert_activity(bool f);

  DS2409Network *get_network(bool mainnetwork) { return mainnetwork ? &main : &aux; }

 protected:
    DS2409Network main{this,true};
    DS2409Network aux{this,false};
  bool current_state_;
  bool current_ctrl_;
  bool alert_activity_{false};

  void write_state(bool state) override;

    friend DS2409Network;
  ESPOneWire *get_reset_one_wire_child_(bool main);

  // gpio pin component
  bool digital_read_(uint8_t pin) override;
  void digital_write_(uint8_t pin, bool value) override;
  void pin_mode_(uint8_t pin, gpio::Flags flags) override;

  // device access
  uint8_t status_update(uint8_t config);
  ESPOneWire *smart_on(bool main);
  void all_off();
};

class DS2409GPIOPin : public DallasGPIOPin {};

}
}
