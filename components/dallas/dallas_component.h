#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esp_one_wire.h"

#include <vector>

namespace esphome {
namespace dallas {

 #define LOG_UPDATE_ALERT_INTERVAL(this) \
   if (this->get_alert_update_interval() == SCHEDULER_DONT_RUN) { \
     ESP_LOGCONFIG(TAG, "  Alert Interval: never"); \
   } else if (this->get_alert_update_interval() < 100) { \
     ESP_LOGCONFIG(TAG, "  Alert Interval: %.3fs", this->get_alert_update_interval() / 1000.0f); \
   } else { \
     ESP_LOGCONFIG(TAG, "  Alert Interval: %.1fs", this->get_alert_update_interval() / 1000.0f); \
   }
 

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

class DallasGPIOPin;
class DallasPinComponent {
 protected:
  friend DallasGPIOPin;
    /// Helper function to read the value of a pin.
  bool virtual digital_read_(uint8_t pin) = 0;
  /// Helper function to write the value of a pin.
  void virtual digital_write_(uint8_t pin, bool value) = 0;
  /// Helper function to set the pin mode of a pin.
  void virtual pin_mode_(uint8_t pin, gpio::Flags flags) = 0;
};

class DallasGPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(DallasPinComponent *parent) { parent_ = parent; }
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

 protected:
  DallasPinComponent *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

uint16_t crc16(const uint8_t *data, uint16_t len, uint16_t crc, uint16_t reverse_poly, bool refin, bool refout);

}  // namespace dallas
}  // namespace esphome
