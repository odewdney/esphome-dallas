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

  void set_counter_a(sensor::Sensor *counter) { counter_a_ = counter; }
  void set_counter_b(sensor::Sensor *counter) { counter_b_ = counter; }
  void set_counter_c(sensor::Sensor *counter) { counter_c_ = counter; }
  void set_counter_d(sensor::Sensor *counter) { counter_d_ = counter; }

 protected:
  sensor::Sensor *counter_a_{nullptr};
  sensor::Sensor *counter_b_{nullptr};
  sensor::Sensor *counter_c_{nullptr};
  sensor::Sensor *counter_d_{nullptr};
  
  void read_counter_(uint8_t counter, sensor::Sensor *sensor);

};

}
}