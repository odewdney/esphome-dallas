#include "ds2405.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.sensor";

static const uint8_t DALLAS_MODEL_DS2405 = 0x05;

bool DS2405Component::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2405;
}

void DS2405Component::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2405");
}

void DS2405Component::notify_alerting() {
    current_state = true;
}

void DS2405Component::write_state(bool state) {
    auto *wire = this->get_reset_one_wire_();
    if(wire == nullptr){
        return;
    }

    bool bit1,bit2;
    {
        InterruptLock lock;

        wire->select(this->address_);
        bit1 = wire->read_bit();
        bit2 = wire->read_bit();
    }
    ESP_LOGD(TAG, "Bit=%d %d", bit1, bit2);
    uint64_t sel;
    {
        InterruptLock lock;
        sel = wire->search_select(this->address_,ONE_WIRE_ROM_ACTIVE_SEARCH);
    }
ESP_LOGD(TAG, "search %016llx", sel);

    this->publish_state(sel == this->address_);
}


}
}
