#pragma once

#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {

class DallasCounterComponent : public PollingComponent, public DallasDevice {
 public:
  DallasCounterComponent() : PollingComponent(15000) {}
  
  bool is_supported(uint8_t *address8) override;
  void update() override;
  void dump_config() override;

  SUB_SENSOR(counter_a);
  SUB_SENSOR(counter_b);
  SUB_SENSOR(counter_c);
  SUB_SENSOR(counter_d);

 protected:
  
  void read_counter_(uint8_t counter, sensor::Sensor *sensor);

};

}
}