#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DS2405Component : public DallasDevice, public Component, public switch_::Switch {
 public:
  bool is_supported(uint8_t *address8) override;
  void dump_config() override;
  void notify_alerting() override;
  
 protected:
  bool current_state;
  void write_state(bool state) override;
};

}
}
