#include "ds2405.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2405";

static const uint8_t DALLAS_MODEL_DS2405 = 0x05;

bool DS2405Component::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2405;
}

float DS2405Component::get_setup_priority() const { return setup_priority::IO; }

bool DS2405Component::setup_sensor() {
    this->current_ctrl_ = this->search(true);
    return true;
}

void DS2405Component::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2405");
  LOG_UPDATE_INTERVAL(this);
}

void DS2405Component::update() {
//    this->current_state_ = search(false);
//    this->publish_state(this->current_state_);
}

void DS2405Component::set_alert_activity(bool f) { this->alert_activity_ = f; }

void DS2405Component::notify_alerting() {
    if( this->alert_activity_ )
      this->current_state_ = true;
}

void DS2405Component::write_state(bool state) {
    if (this->current_ctrl_ != state) {
        bool cur = this->toggle_pin();
        if(cur == state) {
            ESP_LOGD(TAG, "State error state=%d cur=%d", state, cur);
        }
        this->current_ctrl_ = state;
        this->publish_state(state);
    }
}

bool DS2405Component::digital_read_(uint8_t pin) {
    return current_state_;
}

void DS2405Component::digital_write_(uint8_t pin, bool value) {
    this->write_state(value);
}

void DS2405Component::pin_mode_(uint8_t pin, gpio::Flags flags) {

}

bool DS2405Component::toggle_pin() {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return false;
    }

    bool bit1,bit2;
    {
        InterruptLock lock;

        wire->select(this->address_);
        bit1 = wire->read_bit();
        bit2 = wire->read_bit();
    }
    ESP_LOGD(TAG, "Bit=%d %d", bit1, bit2);
    return bit1;
}

bool DS2405Component::search(bool active) {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return false;
    }

    uint8_t cmd = active ? ONE_WIRE_ROM_ACTIVE_SEARCH : ONE_WIRE_ROM_SEARCH;

    bool bit = false;

    uint64_t sel;
    {
        InterruptLock lock;
        sel = wire->search_select(this->address_, cmd);
        if ( sel == this->address_ )
            bit = wire->read_bit();
    }
ESP_LOGD(TAG, "search %016llx %d", sel, bit);

    if (active) {
        return (sel == this->address_ );
    }
    return bit;
}

}
}
