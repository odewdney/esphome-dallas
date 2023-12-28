#pragma once

#include "../dallas/dallas_component.h"

namespace esphome {
namespace dallas {


enum DS2438Threshold {ZeroBits = 0, TwoBits = 0x40, FourBits = 0x80, EightBits = 0xC0};

class DS2438Component : public PollingComponent, public DallasDevice {
 public:
  DS2438Component() : PollingComponent(15000) {}
  
  bool is_supported(uint8_t *address8) override;
  void setup() override;
  void update() override;
  void dump_config() override;
  void set_current_resistor(float resistance) { this->shunt_resistance_ = resistance; };
  void set_current_threshold(uint8_t threshold) { this->threshold_ = threshold; };
  uint16_t millis_to_wait_for_conversion() const override { return 10; };
  void read_conversion() override;

  SUB_SENSOR(temp);
  SUB_SENSOR(vcc);
  SUB_SENSOR(vad);
  SUB_SENSOR(current);
  SUB_SENSOR(ica);

 protected:
  float shunt_resistance_;
  uint8_t threshold_;

  void read_volt(bool vcc);

  bool read_mem(uint8_t page, uint8_t *buffer);
  bool write_mem(uint8_t page, uint8_t *buffer);
  bool start_volt_conversion(bool vcc);
};

}
}