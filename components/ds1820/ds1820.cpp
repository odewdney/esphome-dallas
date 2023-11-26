#include "ds1820.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds1820";

static const uint8_t DALLAS_MODEL_DS18S20 = 0x10;
static const uint8_t DALLAS_MODEL_DS1822 = 0x22;
static const uint8_t DALLAS_MODEL_DS18B20 = 0x28;
static const uint8_t DALLAS_MODEL_DS1825 = 0x3B;
static const uint8_t DALLAS_MODEL_DS28EA00 = 0x42;
static const uint8_t DALLAS_COMMAND_START_CONVERSION = 0x44;
static const uint8_t DALLAS_COMMAND_READ_SCRATCH_PAD = 0xBE;
static const uint8_t DALLAS_COMMAND_WRITE_SCRATCH_PAD = 0x4E;

uint16_t DallasTemperatureSensor::millis_to_wait_for_conversion() const {
  switch (this->resolution_) {
    case 9:
      return 94;
    case 10:
      return 188;
    case 11:
      return 375;
    default:
      return 750;
  }
}

bool DallasTemperatureSensor::is_supported(uint8_t *address8) {
  return (address8[0] == DALLAS_MODEL_DS18S20 || address8[0] == DALLAS_MODEL_DS1822 ||
        address8[0] == DALLAS_MODEL_DS18B20 || address8[0] == DALLAS_MODEL_DS1825 ||
        address8[0] == DALLAS_MODEL_DS28EA00);
}

uint8_t DallasTemperatureSensor::get_resolution() const { return this->resolution_; }
void DallasTemperatureSensor::set_resolution(uint8_t resolution) { this->resolution_ = resolution; }

void DallasTemperatureSensor::read_conversion() {
  bool res = this->read_scratch_pad();

  if (!res) {
    ESP_LOGW(TAG, "'%s' - Resetting bus for read failed!", this->get_name().c_str());
    this->publish_state(NAN);
    this->parent_->status_set_warning();
    return;
  }
  if (!this->check_scratch_pad()) {
    this->publish_state(NAN);
    this->parent_->status_set_warning();
    return;
  }

  float tempc = this->get_temp_c();
  ESP_LOGD(TAG, "'%s': Got Temperature=%.1f°C", this->get_name().c_str(), tempc);
  this->publish_state(tempc);
}

void DallasTemperatureSensor::dump_config() {
  DallasSensor::dump_config();
  ESP_LOGCONFIG(TAG, "    Resolution: %u", this->get_resolution());
}

bool IRAM_ATTR DallasTemperatureSensor::read_scratch_pad() {
  auto *wire = this->get_reset_one_wire_();

  if (wire == nullptr) {
    return false;
  }

  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_COMMAND_READ_SCRATCH_PAD);

    for (unsigned char &i : this->scratch_pad_) {
      i = wire->read8();
    }
  }

  return true;
}
bool DallasTemperatureSensor::setup_sensor() {
  if ( this->get_address8()[0] == 0 )
    return false;

  bool r = this->read_scratch_pad();

  if (!r) {
    ESP_LOGE(TAG, "Reading scratchpad failed: reset");
    return false;
  }
  if (!this->check_scratch_pad())
    return false;

  if (this->scratch_pad_[4] == this->resolution_)
    return false;

  if (this->get_address8()[0] == DALLAS_MODEL_DS18S20) {
    // DS18S20 doesn't support resolution.
    ESP_LOGW(TAG, "DS18S20 doesn't support setting resolution.");
    return false;
  }

  switch (this->resolution_) {
    case 12:
      this->scratch_pad_[4] = 0x7F;
      break;
    case 11:
      this->scratch_pad_[4] = 0x5F;
      break;
    case 10:
      this->scratch_pad_[4] = 0x3F;
      break;
    case 9:
    default:
      this->scratch_pad_[4] = 0x1F;
      break;
  }

  this->scratch_pad_[2] = 125;
  this->scratch_pad_[3] = -55;

  auto *wire = this->get_one_wire_();
  {
    InterruptLock lock;
    if (wire->reset()) {
      wire->select(this->address_);
      wire->write8(DALLAS_COMMAND_WRITE_SCRATCH_PAD);
      wire->write8(this->scratch_pad_[2]);  // high alarm temp
      wire->write8(this->scratch_pad_[3]);  // low alarm temp
      wire->write8(this->scratch_pad_[4]);  // resolution
      wire->reset();

      // write value to EEPROM
      wire->select(this->address_);
      wire->write8(0x48);
    }
  }

  delay(20);  // allow it to finish operation
  wire->reset();
  return true;
}
bool DallasTemperatureSensor::check_scratch_pad() {
  bool chksum_validity = (crc8(this->scratch_pad_, 8) == this->scratch_pad_[8]);
  bool config_validity = false;

  switch (this->get_address8()[0]) {
    case DALLAS_MODEL_DS18B20:
      config_validity = ((this->scratch_pad_[4] & 0x9F) == 0x1F);
      break;
    default:
      config_validity = ((this->scratch_pad_[4] & 0x10) == 0x10);
  }

#ifdef ESPHOME_LOG_LEVEL_VERY_VERBOSE
  ESP_LOGVV(TAG, "Scratch pad: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X (%02X)", this->scratch_pad_[0],
            this->scratch_pad_[1], this->scratch_pad_[2], this->scratch_pad_[3], this->scratch_pad_[4],
            this->scratch_pad_[5], this->scratch_pad_[6], this->scratch_pad_[7], this->scratch_pad_[8],
            crc8(this->scratch_pad_, 8));
#endif
  if (!chksum_validity) {
    ESP_LOGW(TAG, "'%s' - Scratch pad checksum invalid!", this->get_name().c_str());
  } else if (!config_validity) {
    ESP_LOGW(TAG, "'%s' - Scratch pad config register invalid!", this->get_name().c_str());
  }
  return chksum_validity && config_validity;
}
float DallasTemperatureSensor::get_temp_c() {
  int16_t temp = (int16_t(this->scratch_pad_[1]) << 11) | (int16_t(this->scratch_pad_[0]) << 3);
  if (this->get_address8()[0] == DALLAS_MODEL_DS18S20) {
    int diff = (this->scratch_pad_[7] - this->scratch_pad_[6]) << 7;
    temp = ((temp & 0xFFF0) << 3) - 16 + (diff / this->scratch_pad_[7]);
  }

  return temp / 128.0f;
}

}
}
