#include "dallas_component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.sensor";
static const uint8_t DALLAS_COMMAND_START_CONVERSION = 0x44;

void DallasComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DallasComponent 2...");

  pin_->setup();

  // clear bus with 480µs high, otherwise initial reset in search_vec() fails
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  delayMicroseconds(480);

  one_wire_ = new ESPOneWire(pin_);  // NOLINT(cppcoreguidelines-owning-memory)

  std::vector<uint64_t> raw_sensors;
  raw_sensors = this->one_wire_->search_vec();

  for (auto &address : raw_sensors) {
    auto *address8 = reinterpret_cast<uint8_t *>(&address);
    if (crc8(address8, 7) != address8[7]) {
      ESP_LOGW(TAG, "Dallas device 0x%s has invalid CRC.", format_hex(address).c_str());
      continue;
    }
	bool is_supported = false;
    for (auto *sensor : this->sensors_) {
	  if ( sensor->is_supported(address8) ) {
	    is_supported = true;
	    break;
	  }
	}
	if ( !is_supported ) {
      ESP_LOGW(TAG, "Unknown device type 0x%02X.", address8[0]);
      continue;
	}
    this->found_sensors_.push_back(address);
  }

  for (auto *sensor : this->sensors_) {
    if (sensor->get_index().has_value()) {
      if (*sensor->get_index() >= this->found_sensors_.size()
        || !sensor->is_supported(reinterpret_cast<uint8_t *>(&this->found_sensors_[*sensor->get_index()]))) {
        this->status_set_error();
        continue;
      }
      sensor->set_address(this->found_sensors_[*sensor->get_index()]);
    }

    if (!sensor->setup_sensor()) {
      this->status_set_error();
    }
  }
}

void DallasComponent::start_alert_poller() {
    this->set_interval("alert_update", this->get_alert_update_interval(), [this]() { this->update_alert(); });
}

void DallasComponent::stop_alert_poller() {
  this->cancel_interval("alert_update");
}

uint32_t DallasComponent::get_alert_update_interval() const { return this->alert_update_interval_; }
void DallasComponent::set_alert_update_interval(uint32_t alert_update_interval) { this->alert_update_interval_ = alert_update_interval; }

void DallasComponent::call_setup() {
  PollingComponent::call_setup();
  start_alert_poller();
}

void DallasComponent::update_alert() {
  //ESP_LOGD(TAG,"Scanning alerting devices");
  this->one_wire_->reset_search();
  uint64_t addr;
  while((addr = this->one_wire_->active_search()) != 0u){
      if ( (addr & 0xff) != 0x05 ) // suppress ds2405
      	ESP_LOGD(TAG,"Alert %016llx",addr);
      for (auto *sensor : this->sensors_) {
        if (sensor->get_address() == addr){
          sensor->notify_alerting();
        }
      }
  }
}


void DallasComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DallasComponent:");
  LOG_PIN("  Pin: ", this->pin_);
  LOG_UPDATE_INTERVAL(this);

  if (this->found_sensors_.empty()) {
    ESP_LOGW(TAG, "  Found no sensors! X");
  } else {
    ESP_LOGD(TAG, "  Found sensors:");
    for (auto &address : this->found_sensors_) {
      ESP_LOGD(TAG, "    0x%s", format_hex(address).c_str());
    }
  }

  for (auto *sensor : this->sensors_) {
    sensor->dump_config();
    if (sensor->get_index().has_value()) {
      ESP_LOGCONFIG(TAG, "    Index %u", *sensor->get_index());
      if (*sensor->get_index() >= this->found_sensors_.size()) {
        ESP_LOGE(TAG, "Couldn't find sensor by index - not connected. Proceeding without it.");
        continue;
      }
    }
  }
}

void DallasComponent::register_sensor(DallasDevice *sensor) { this->sensors_.push_back(sensor); }
void DallasComponent::update() {
  this->status_clear_warning();

  bool result;
  {
    InterruptLock lock;
    result = this->one_wire_->reset();
  }
  if (!result) {
    ESP_LOGE(TAG, "Requesting conversion failed");
    this->status_set_warning();
//    for (auto *sensior : this->sensors_) {
//      sensor->publish_state(NAN);
//    }
    return;
  }

  {
    InterruptLock lock;
    this->one_wire_->skip();
    this->one_wire_->write8(DALLAS_COMMAND_START_CONVERSION);
  }

  for (auto *sensor : this->sensors_) {
	auto conversion_millis = sensor->millis_to_wait_for_conversion();
	if (conversion_millis > 0) {
      this->set_timeout(sensor->get_address_name(), conversion_millis, [this, sensor] {
	    sensor->read_conversion();
      });
	}
  }
}

void DallasDevice::set_address(uint64_t address) { this->address_ = address; }
optional<uint8_t> DallasDevice::get_index() const { return this->index_; }
void DallasDevice::set_index(uint8_t index) { this->index_ = index; }
uint8_t *DallasDevice::get_address8() { return reinterpret_cast<uint8_t *>(&this->address_); }
const std::string &DallasDevice::get_address_name() {
  if (this->address_name_.empty()) {
    this->address_name_ = std::string("0x") + format_hex(this->address_);
  }

  return this->address_name_;
}

void DallasDevice::dump_config() {
  //LOG_SENSOR("  ", "Device", this);
	ESP_LOGCONFIG(TAG, "  Device Address: %s", this->get_address_name().c_str());
}

ESPOneWire *DallasDevice::get_reset_one_wire_() {
  auto parent = this->parent_;
  if (parent == nullptr) {
    ESP_LOGD(TAG, "parent is null");
    return nullptr;
  }
  auto wire = parent->one_wire_;
  if ( wire == nullptr ) {
    ESP_LOGD(TAG, "one wire is null");
    return nullptr;
  }
  {
    InterruptLock lock;

    if (!wire->reset()) {
      ESP_LOGD(TAG, "reset failed");
      return nullptr;
    }
  }
  return wire;
}


std::string DallasSensor::unique_id() { return "dallas-" + str_lower_case(format_hex(this->address_)); }

void DallasSensor::dump_config() {
  LOG_SENSOR("  ", "Device", this);
  DallasDevice::dump_config();
}




uint16_t crc16(const uint8_t *data, uint16_t len, uint16_t crc, uint16_t reverse_poly, bool refin, bool refout) {
#ifdef USE_ESP32
  if (reverse_poly == 0x8408) {
    crc = crc16_le(refin ? crc : (crc ^ 0xffff), data, len);
    return refout ? crc : (crc ^ 0xffff);
  }
#endif
  if (refin) {
    crc ^= 0xffff;
  }
  /*
#ifndef USE_ESP32
  if (reverse_poly == 0x8408) {
    while (len--) {
      uint8_t combo = crc ^ (uint8_t) *data++;
      crc = (crc >> 8) ^ CRC16_8408_LE_LUT_L[combo & 0x0F] ^ CRC16_8408_LE_LUT_H[combo >> 4];
    }
  } else
#endif
      if (reverse_poly == 0xa001) {
    while (len--) {
      uint8_t combo = crc ^ (uint8_t) *data++;
      crc = (crc >> 8) ^ CRC16_A001_LE_LUT_L[combo & 0x0F] ^ CRC16_A001_LE_LUT_H[combo >> 4];
    }
  } else */ {
    while (len--) {
      crc ^= *data++;
      for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x0001) {
          crc = (crc >> 1) ^ reverse_poly;
        } else {
          crc >>= 1;
        }
      }
    }
  }
  return refout ? (crc ^ 0xffff) : crc;
}


}  // namespace dallas
}  // namespace esphome

