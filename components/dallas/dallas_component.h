#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esp_one_wire.h"

#include <vector>

namespace esphome {
namespace dallas {

class DallasDevice;
class DallasSensor;

class DallasComponent : public PollingComponent {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void register_sensor(DallasDevice *sensor);

  void setup() override;
  void dump_config() override;
  //float get_setup_priority() const override { return setup_priority::DATA; }
  float get_setup_priority() const override { return setup_priority::BUS; }

  void update() override;

  void set_alert_update_interval(uint32_t update_interval);
  void update_alert();
    virtual uint32_t get_alert_update_interval() const;
  void start_alert_poller();
  void stop_alert_poller();
  void call_setup() override;

 protected:
  friend DallasDevice;

  InternalGPIOPin *pin_;
  ESPOneWire *one_wire_;
  std::vector<DallasDevice *> sensors_;
  std::vector<uint64_t> found_sensors_;
  uint32_t alert_update_interval_;
};


class DallasDevice {
 public:
  void set_parent(DallasComponent *parent) { parent_ = parent; }
  uint64_t get_address() { return this->address_; }
  /// Helper to get a pointer to the address as uint8_t.
  uint8_t *get_address8();
  /// Helper to create (and cache) the name for this sensor. For example "0xfe0000031f1eaf29".
  const std::string &get_address_name();

  /// Set the 64-bit unsigned address for this sensor.
  void set_address(uint64_t address);
  /// Get the index of this sensor. (0 if using address.)
  optional<uint8_t> get_index() const;
  /// Set the index of this sensor. If using index, address will be set after setup.
  void set_index(uint8_t index);

  /// Get the number of milliseconds we have to wait for the conversion phase.
  uint16_t virtual millis_to_wait_for_conversion() const { return 0; };

  bool virtual is_supported(uint8_t *address8) { return false; }
  bool virtual setup_sensor() { return false; };
  void virtual dump_config();
  void virtual read_conversion() {};
  void virtual notify_alerting() {};

 protected:
  DallasComponent *parent_{nullptr};
  uint64_t address_{0U};
  optional<uint8_t> index_;
  std::string address_name_;
  
  ESPOneWire *get_one_wire_() { return this->parent_ ? this->parent_->one_wire_ : nullptr; }
  ESPOneWire *get_reset_one_wire_();
};

class DallasSensor : public sensor::Sensor, public DallasDevice {
 public:
  std::string unique_id() override;
  void dump_config() override;
};

/// Internal class that helps us create multiple sensors for one Dallas hub.
class DallasTemperatureSensor : public DallasSensor {
 public:
  /// Get the set resolution for this sensor.
  uint8_t get_resolution() const;
  /// Set the resolution for this sensor.
  void set_resolution(uint8_t resolution);
  /// Get the number of milliseconds we have to wait for the conversion phase.
  uint16_t millis_to_wait_for_conversion() const override;

  bool is_supported(uint8_t *address8) override;
  bool setup_sensor() override;
  void dump_config() override;
  void read_conversion() override;

  bool read_scratch_pad();

  bool check_scratch_pad();

  float get_temp_c();


 protected:
  uint8_t resolution_;
  uint8_t scratch_pad_[9] = {
      0,
  };
};


uint16_t crc16(const uint8_t *data, uint16_t len, uint16_t crc, uint16_t reverse_poly, bool refin, bool refout);

}  // namespace dallas
}  // namespace esphome
