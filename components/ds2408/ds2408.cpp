#include "ds2408.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.ds2408";

static const uint8_t DALLAS_MODEL_DS2408 = 0x29;
static const uint8_t DALLAS_READ_PIO_REGISTERS = 0xF0;
static const uint8_t DALLAS_CHANNEL_ACCESS_WRITE = 0x5A;
static const uint8_t DALLAS_CHANNEL_ACCESS_READ = 0xF5;
static const uint8_t DALLAS_WRITE_CHANNEL_SEARCH_REGISTER = 0xCC;
static const uint8_t DALLAS_RESET_ACTIVITY_LATCHES = 0xC3;

bool DS2408Component::is_supported(uint8_t *address8) {
	return address8[0] == DALLAS_MODEL_DS2408;
}

void DS2408Component::setup() {
  auto *wire = this->get_reset_one_wire_();
  if(wire == nullptr){
    return;
  }
  ESP_LOGD(TAG,"reading");
  uint16_t addr = 0x88;
  uint8_t buffer[10];
  uint8_t req[3] = { DALLAS_READ_PIO_REGISTERS,
    (uint8_t)(addr & 0xff),
    (uint8_t)((addr>>8) & 0xff )};

  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8( req[0] );
    wire->write8( req[1] );
    wire->write8( req[2] );
    for (unsigned char &i : buffer) {
      i = wire->read8();
	  }
  }
  uint16_t crc_res = buffer[8] | (buffer[9]<<8);

  uint16_t crc = dallas::crc16(req, 3, 0, 0xa001, false, false);
  crc = dallas::crc16(buffer, 8, crc, 0xa001, false, true);

  if ( crc != crc_res ) {
    ESP_LOGW(TAG, "Failed to read reg - bad crc");
    return;
  }
  this->read_value_ = buffer[0];
  this->value_ = buffer[1];
  this->pin_mode_value_ = 0;

for(int n=0; n < 10; n+=2){
    ESP_LOGD(TAG, "%02x: %02x %02x", n, buffer[n], buffer[n+1]);
}

}

void DS2408Component::notify_alerting() {
  update();
  if(this->alert_activity_) {
    reset_activity();
  } else {
    update_conditional();
  }
}

void DS2408Component::dump_config() {
  DallasDevice::dump_config();
  ESP_LOGCONFIG(TAG, "    Device: ds2408");
}

/// Helper function to read the value of a pin.
bool DS2408Component::digital_read_(uint8_t pin) {
    return (this->read_value_ & (1<<pin)) != 0;
}
void DS2408Component::update() {
  this->read_value_ = read_channel();
}
/// Helper function to write the value of a pin.
void DS2408Component::digital_write_(uint8_t pin, bool value) {
  if( value ) 
    this->value_ |= 1<<pin;
  else
    this->value_ &= ~(1<<pin);
  
  write_channel(this->value_);
}

uint8_t DS2408Component::read_channel() {
    auto *wire = this->get_reset_one_wire_();
  if(wire == nullptr){
    return 0;
  }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_CHANNEL_ACCESS_READ);
    return wire->read8();
  }
}

void DS2408Component::write_channel(uint8_t value) {
  auto *wire = this->get_reset_one_wire_();
  if(wire == nullptr){
    return;
  }
  uint8_t chk, val;
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_CHANNEL_ACCESS_WRITE);
    wire->write8(value);
    wire->write8(~value);
    chk = wire->read8();
    val = wire->read8();
  }

  if(chk != 0xAA && val != value) {
    ESP_LOGD(TAG,"Write chk:%02x val:%02x", chk, val);
  }
}

/// Helper function to set the pin mode of a pin.
void DS2408Component::pin_mode_(uint8_t pin, gpio::Flags flags) {
  uint8_t mask = 1 << pin;
  if ( flags & gpio::FLAG_INPUT )
    this->pin_mode_value_ |= mask;
  else
    this->pin_mode_value_ &= ~mask;
  update_conditional();
}

void DS2408Component::reset_activity() {
  auto *wire = this->get_reset_one_wire_();
  if(wire == nullptr){
    return;
  }
  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_RESET_ACTIVITY_LATCHES);
  }
}

void DS2408Component::update_conditional() {
ESP_LOGW(TAG,"Updating cond");

  auto *wire = this->get_reset_one_wire_();
  if(wire == nullptr){
    return;
  }

  uint8_t pol, config;
  if(this->alert_activity_) {
    pol = this->pin_mode_value_;
    config = 1;
  }
  else {
    pol = (~this->read_value_) & this->pin_mode_value_;
    config = 0;
  }

  {
    InterruptLock lock;

    wire->select(this->address_);
    wire->write8(DALLAS_WRITE_CHANNEL_SEARCH_REGISTER);
    wire->write8(0x8b);
    wire->write8(0);
    // cond search mask
    wire->write8(this->pin_mode_value_);
    // cond polarity
    wire->write8(pol);
    // config
    wire->write8(config);
  }
ESP_LOGD(TAG,"Set pin mode mask=%02x pol=%02x config=%02x read=%02x", this->pin_mode_value_, pol, config, this->read_value_);
}

float DS2408Component::get_setup_priority() const { return setup_priority::IO; }

void DS2408Component::set_alert_activity(bool f) {
  this->alert_activity_ = f;
  update_conditional();
}

}
}
