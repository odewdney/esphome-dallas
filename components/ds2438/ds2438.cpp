#include "ds2438.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2438";

static const uint8_t DALLAS_MODEL_DS2438 = 0x26;
static const uint8_t DALLAS_READ_SCATCH_PAD_CMD = 0xBE;
static const uint8_t DALLAS_WRITE_SCATCH_PAD_CMD = 0x4E;
static const uint8_t DALLAS_COPY_SCATCH_PAD_CMD = 0x48;
static const uint8_t DALLAS_RECALL_MEMORY_CMD = 0xB8;
static const uint8_t DALLAS_CONVERT_TEMP_CMD = 0x44;
static const uint8_t DALLAS_CONVERT_VOLT_CMD = 0xB4;

bool DS2438Component::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2438;
}

void DS2438Component::setup() {
    uint8_t buffer[8] = {0};
    // ctrl
    buffer[0] = 0x0f;
    buffer[7] = this->threshold_;
    this->write_mem(0, buffer);
}

void DS2438Component::update() {
  ESP_LOGD(TAG, "updating battery");

    this->start_volt_conversion(false);
    this->set_timeout("volt", 10, [this]() {this->read_volt(false);});
}

void DS2438Component::read_volt(bool vcc) {
    uint8_t buffer[8];
    this->read_mem(0, buffer);

    float volt = (buffer[4]<<8) + buffer[3];
    volt /= 100.0f;

    auto sensor = vcc ? this->vcc_sensor_ : this->vad_sensor_;
    if (sensor != nullptr) {
        sensor->publish_state(volt);
    }

    if (!vcc) {
        this->start_volt_conversion(true);
        this->set_timeout("volt", 10, [this]() {this->read_volt(true);});
        return;
    }

    float current = (((int8_t)buffer[6]) << 8) + buffer[5];
    current /= 4096;
    if ( this->shunt_resistance_ != 0.0f)
      current /= this->shunt_resistance_;

    if (this->current_sensor_ != nullptr)
        this->current_sensor_->publish_state(current);

    if(this->ica_sensor_ != nullptr) {
        if ( !this->read_mem(1,buffer))
            return;
        float ica = buffer[4];
        ica /= 2048;
        if ( this->shunt_resistance_ != 0.0f)
            ica /= this->shunt_resistance_;
        this->ica_sensor_->publish_state(ica);
    }
}

void DS2438Component::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2438");
}

void DS2438Component::read_conversion() {
    uint8_t buffer[8];
    bool res = this->read_mem(0, buffer);

    float temp = (((int8_t)buffer[2]) << 8) + buffer[1];
    temp /= 256.0f;
    ESP_LOGD(TAG, "Temp = %f", temp);
    if ( this->temp_sensor_ != nullptr )
        this->temp_sensor_->publish_state(temp);
}

bool DS2438Component::start_volt_conversion(bool vcc) {
    uint8_t buffer[8];
    buffer[0] = 7 | (vcc ? 8 : 0);
    buffer[7] = this->threshold_;
    if (!this->write_mem(0, buffer))
        return false;

    auto *wire = this->get_reset_one_wire_();

    if (wire == nullptr) {
        return false;
    }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_CONVERT_VOLT_CMD);
  }
    return true;    
}

bool DS2438Component::read_mem(uint8_t page, uint8_t *buffer) {
    auto *wire = this->get_reset_one_wire_();

    if (wire == nullptr) {
        return false;
    }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_RECALL_MEMORY_CMD);
    wire->write8(page);
  }
  wire = this->get_reset_one_wire_();

  if (wire == nullptr) {
    return false;
  }

  uint8_t crc;
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_READ_SCATCH_PAD_CMD);
    wire->write8(page);
    for(uint8_t n = 0; n < 8; n++)
        buffer[n] = wire->read8();
    crc = wire->read8();
  }

  uint8_t crcdata = crc8(buffer, 8);
  if ( crc != crcdata ) {
    ESP_LOGW(TAG, "bad crc %02x %02x", crc, crcdata);
    return false;
  }
  return true;
}

bool DS2438Component::write_mem(uint8_t page, uint8_t *buffer) {
    auto *wire = this->get_reset_one_wire_();

    if (wire == nullptr) {
        return false;
    }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_WRITE_SCATCH_PAD_CMD);
    wire->write8(page);
    for(uint8_t n = 0; n < 8; n++)
        wire->write8(buffer[n]);
  }
  wire = this->get_reset_one_wire_();

  if (wire == nullptr) {
    return false;
  }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_COPY_SCATCH_PAD_CMD);
    wire->write8(page);
  }

  return true;
}

}
}
