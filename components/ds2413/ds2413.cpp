#include "ds2413.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2413";

static const uint8_t DALLAS_MODEL_DS2413 = 0x3a;
static const uint8_t DALLAS_PIO_ACCESS_READ_CMD = 0xF5;
static const uint8_t DALLAS_PIO_ACCESS_WRITE_CMD = 0x5A;

bool DS2413Device::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2413;
}

bool DS2413Device::setup_sensor() {
    auto state = this->pio_access_read();
    if (!state) {
        return false;
    }
    auto reg = state.value();
    this->current_state_ = reg;
    this->current_latch_ = ((reg & 2)>>1) | ((reg & 8)>>2);
    return true;
}


void DS2413Device::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2413");
}

bool DS2413Device::digital_read_(uint8_t pin) {
    return this->current_latch_ & ( 1 << pin);
}

void DS2413Device::digital_write_(uint8_t pin, bool value) {
    uint8_t bit = 1<<pin;
    this->current_latch_ &= ~bit;
    if (value)
        this->current_latch_ |= bit;
ESP_LOGD(TAG,"setting to %02x",this->current_latch_);
    auto state = this->pio_access_write(this->current_latch_);
ESP_LOGD(TAG,"state = %02x", state ? state.value() : -1);
    if (state)
        this->current_state_ = state.value();
}

void DS2413Device::pin_mode_(uint8_t pin, gpio::Flags flags) {

}

optional<uint8_t> DS2413Device::pio_access_read() {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return {};
    }

    uint8_t data;
    {
        InterruptLock lock;

        wire->select(this->address_);
        wire->write8(DALLAS_PIO_ACCESS_READ_CMD);
        data = wire->read8();
    }

    if ((((data & 0xf0) ^ 0xf0)>>4) != (data & 0x0f) ) {
        ESP_LOGW(TAG, "Bad data %02x", data);
        return {};
    }
    return data & 0x0f;
}

optional<uint8_t> DS2413Device::pio_access_write(uint8_t data) {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return {};
    }

    uint8_t confirm;
    {
        InterruptLock lock;

        wire->select(this->address_);
        wire->write8(DALLAS_PIO_ACCESS_WRITE_CMD);
        data |= 0xfc;
        wire->write8(data);
        data ^= 0xff;
        wire->write8(data);
        confirm = wire->read8();
        data = wire->read8();
    }

    if (confirm != 0xaa) {
        ESP_LOGW(TAG, "Confirm error %02x", confirm);
        return {};
    }

    if ((((data & 0xf0) ^ 0xf0)>>4) != (data & 0x0f) ) {
        ESP_LOGW(TAG, "Bad data %02x", data);
        return {};
    }
    return data & 0x0f;
}

void DS2413Switch::write_state(bool state) {
    this->parent_->digital_write_(this->pin_, state);
    this->publish_state(state);
}

/*
bool DS2413Switch::setup_sensor() {
    auto restore_state = this->get_initial_state_with_restore_mode();
    if (restore_state && this->parent_->digital_read_(this->pin_) != restore_state.value()) {
        this->parent_->digital_write_(this->pin_, restore_state.value());
    }
    return true;
}

void DS2413Switch::dump_config() {
  DS2413Device::dump_config();
  switch_::log_switch(TAG, "   ","ds2413", this);
}
*/
float DS2413Component::get_setup_priority() const { return setup_priority::IO; }

void DS2413Component::dump_config() {
  DS2413Device::dump_config();
  LOG_UPDATE_INTERVAL(this);
}

void DS2413Component::update() {
//    this->current_state_ = search(false);
//    this->publish_state(this->current_state_);
}

}
}
