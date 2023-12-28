#include "ds2423.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2423";

static const uint8_t DALLAS_MODEL_DS2423 = 0x1D;
static const uint8_t DALLAS_COMMAND_READ_MEMORY_AND_COUNTER = 0xA5;

bool DallasCounterComponent::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2423;
}

void DallasCounterComponent::update() {
  ESP_LOGD(TAG, "updating counter");

  this->read_counter_(0, this->counter_a_sensor_);
  this->read_counter_(1, this->counter_b_sensor_);
  this->read_counter_(2, this->counter_c_sensor_);
  this->read_counter_(3, this->counter_d_sensor_);
}

void DallasCounterComponent::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2423");
}

void IRAM_ATTR DallasCounterComponent::read_counter_(uint8_t counter, sensor::Sensor *sensor) {
  if ( sensor != nullptr ) {
    auto *wire = this->get_reset_one_wire_();

    if (wire == nullptr) {
      sensor->publish_state(NAN);
      return;
    }

  counter += 12;

  uint8_t buffer[32+4+4+2];
  uint8_t req[3];
  req[0] = DALLAS_COMMAND_READ_MEMORY_AND_COUNTER;
  req[1] = (counter * 32) & 0xff;
  req[2] = ((counter * 32)>>8) & 0xff;
  {
    InterruptLock lock;

    wire->select(this->address_);
    for( uint8_t &r : req) {
      wire->write8(r);
    }
    for (unsigned char &i : buffer) {
      i = wire->read8();
	  }
	}

  auto crc = crc16(req, 3, 0, 0xA001, false, false);
  crc = crc16(buffer, 40, crc, 0xA001, false, true);

/*
for(int n = 0; n < 40; n+=4)
{
	ESP_LOGD(TAG, "byte[%d]=%02x %02x %02x %02x", n, buffer[n],buffer[n+1],buffer[n+2],buffer[n+3]);
}
  ESP_LOGD(TAG, "crc=%04x bytes:%02x %02x", crc, buffer[40],buffer[41]);
*/

  if ( (crc & 0xff) != buffer[40] || ((crc>>8) & 0xff) != buffer[41]) {
    ESP_LOGW(TAG, "bad crc %04x %02x%02x", crc, buffer[41], buffer[40]);
		sensor->publish_state(NAN);
    return;
  }

  auto value = (reinterpret_cast<uint32_t*>(buffer))[8];
  ESP_LOGD(TAG, "Value=%08x", value);
  sensor->publish_state(value);

  }
}

}
}