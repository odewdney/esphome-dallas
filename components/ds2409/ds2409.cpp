#include "ds2409.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2409";

static const uint8_t DALLAS_MODEL_DS2409 = 0x1f;
static const uint8_t DALLAS_STATUS_CMD = 0x5a;
static const uint8_t DALLAS_ALL_OFF_CMD = 0x66;
static const uint8_t DALLAS_DISCHARGE_LINES = 0x99;
static const uint8_t DALLAS_DIRECT_ON_MAIN = 0xa5;
static const uint8_t DALLAS_SMART_ON_MAIN = 0xcc;
static const uint8_t DALLAS_SMART_ON_AUX = 0x33;

ESPOneWire *DS2409Network::get_reset_one_wire_() { return this->parent_->get_reset_one_wire_child_(this->main_); }
Component *DS2409Network::get_component() { return this->parent_; }
void DS2409Network::component_set_timeout(const std::string &name, uint32_t timeout, std::function<void()> &&f)
{
    parent_->set_timeout(name, timeout, std::move(f));
}

ESPOneWire *DS2409Component::get_reset_one_wire_child_(bool main) {
    if (main) {
        bool direct = this->direct_onmain();
        if ( !direct )
            return nullptr;
        return this->get_reset_one_wire_();
    }
    return this->smart_on(main);
}

bool DS2409Component::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2409;
}

float DS2409Component::get_setup_priority() const { return setup_priority::IO + 1.f; }

bool DS2409Component::setup_sensor() {
    auto restore_state = this->get_initial_state_with_restore_mode();
    bool state;
    if (!restore_state) {
        auto info = this->status_update(0x18);
        state = info & 0x40;
    } else {
        state = restore_state.value();
    }
    uint8_t state_reg = 0x20 | ( state ? 0x80 : 0);
    auto info = this->status_update(state_reg);
    this->current_state_ = info & 0x40;

    for(auto *net : {&this->main,&this->aux})
      net->setup_sensor();

    return true;
}

void DS2409Component::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2409");
  LOG_UPDATE_INTERVAL(this);
  switch_::log_switch(TAG, "  ", "ctrl", this);
}

void DS2409Component::update() {
//    this->current_state_ = search(false);
//    this->publish_state(this->current_state_);
    for(auto *net : {&this->main,&this->aux}) {
        net->update_conversions();
    }
    this->all_off();
}

void DS2409Component::set_alert_activity(bool f) { this->alert_activity_ = f; }

void DS2409Component::notify_alerting() {
    auto info = this->status_update(0x18);
    ESP_LOGD(TAG, "alert %02x", info);
    this->all_off();
}

void DS2409Component::write_state(bool state) {
    if (this->current_ctrl_ != state) {
        uint8_t info = this->status_update( 0x20 | (state ? 0x80 : 0));
        state = !!(info & 0x40);
        this->current_ctrl_ = state;
        this->publish_state(state);
    }
}

bool DS2409Component::digital_read_(uint8_t pin) {
    return current_state_;
}

void DS2409Component::digital_write_(uint8_t pin, bool value) {
    this->write_state(value);
}

void DS2409Component::pin_mode_(uint8_t pin, gpio::Flags flags) {

}

uint8_t DS2409Component::status_update(uint8_t config) {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return 0;
    }

    uint8_t info, info_chk;
    {
        InterruptLock lock;

        wire->select(this->address_);
        wire->write8(DALLAS_STATUS_CMD);
        wire->write8(config);
        info = wire->read8();
        info_chk = wire->read8();
    }
    if (info != info_chk)
        ESP_LOGW(TAG, "error config=%02x status=%02x %02x", config, info, info_chk);

    // main not selected
    if ((info & 5) != 4 && this->direct_main) {
        ESP_LOGW(TAG,"unexpected status %02x", info);
        this->direct_main = false;
    }

    return info;
}

bool DS2409Component::direct_onmain() {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return false;
    }

    uint8_t presence;
    {
        InterruptLock lock;
        wire->select(this->address_);
        wire->write8(DALLAS_DIRECT_ON_MAIN);
        auto confirm = wire->read8();
        if ( confirm == DALLAS_DIRECT_ON_MAIN) {
            this->direct_main = true;
            return true;
        }
    }
    return false;
}

ESPOneWire *DS2409Component::smart_on(bool main) {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return nullptr;
    }

    this->direct_main = false;

    uint8_t cmd = main ? DALLAS_SMART_ON_MAIN : DALLAS_SMART_ON_AUX;

    uint8_t presence;
    {
        InterruptLock lock;
        wire->select(this->address_);
        wire->write8(cmd);
        wire->write8(0xff); // reset
        presence = wire->read8();
        auto confirm = wire->read8();
        if( confirm != cmd ) {
            ESP_LOGD(TAG, "failed to get confirm");
          return nullptr;
        }
    }
    if ( presence == 0xff ) {
        ESP_LOGD(TAG, "no devices on %s", main ?"main":"aux");
        return nullptr;
    }
    return wire;
}

void DS2409Component::all_off() {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return;
    }

    this->direct_main = false;

    {
        InterruptLock lock;

        wire->select(this->address_);
        wire->write8(DALLAS_ALL_OFF_CMD);
        auto confirm = wire->read8();
        if (confirm != DALLAS_ALL_OFF_CMD)
            ESP_LOGW(TAG, "all off error %02x", confirm);
    }
}

}
}
