#include "esp_one_wire.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.one_wire";

const uint8_t ONE_WIRE_ROM_SELECT = 0x55;
const uint8_t ONE_WIRE_ROM_SEARCH = 0xF0;
const uint8_t ONE_WIRE_ROM_ACTIVE_SEARCH = 0xEC;

ESPOneWire::ESPOneWire(InternalGPIOPin *pin) { pin_ = pin->to_isr(); }

bool HOT IRAM_ATTR ESPOneWire::reset() {
  // See reset here:
  // https://www.maximintegrated.com/en/design/technical-documents/app-notes/1/126.html
  // Wait for communication to clear (delay G)
  pin_.pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  uint8_t retries = 125;
  do {
    if (--retries == 0)
      return false;
    delayMicroseconds(2);
  } while (!pin_.digital_read());

  // Send 480µs LOW TX reset pulse (drive bus low, delay H)
  pin_.pin_mode(gpio::FLAG_OUTPUT);
  pin_.digital_write(false);
  delayMicroseconds(480);

  // Release the bus, delay I
  pin_.pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  delayMicroseconds(70);

  // sample bus, 0=device(s) present, 1=no device present
  bool r = !pin_.digital_read();
  // delay J
  delayMicroseconds(410);
  return r;
}

uint32_t lastbit{0};

void HOT IRAM_ATTR ESPOneWire::write_bit(bool bit) {
  while((micros()-lastbit)<60)
    ;
  lastbit = micros();

  // drive bus low
  pin_.pin_mode(gpio::FLAG_OUTPUT);
  pin_.digital_write(false);

  // from datasheet:
  // write 0 low time: t_low0: min=60µs, max=120µs
  // write 1 low time: t_low1: min=1µs, max=15µs
  // time slot: t_slot: min=60µs, max=120µs
  // recovery time: t_rec: min=1µs
  // ds18b20 appears to read the bus after roughly 14µs
//  uint32_t delay0 = bit ? 6 : 60;
//  uint32_t delay1 = bit ? 54 : 5;
  uint32_t delay0 = bit ? 6 : 55;
 uint32_t delay1 = bit ? 1 : 1;

  // delay A/C
  delayMicroseconds(delay0);
  // release bus
  pin_.digital_write(true);
  // delay B/D
  delayMicroseconds(delay1);
}

bool HOT IRAM_ATTR ESPOneWire::read_bit() {
  while((micros()-lastbit)<60)
    ;
  lastbit = micros();

  // drive bus low
  pin_.pin_mode(gpio::FLAG_OUTPUT);
  pin_.digital_write(false);

  // note: for reading we'll need very accurate timing, as the
  // timing for the digital_read() is tight; according to the datasheet,
  // we should read at the end of 16µs starting from the bus low
  // typically, the ds18b20 pulls the line high after 11µs for a logical 1
  // and 29µs for a logical 0

  uint32_t start = micros();
  // datasheet says >1µs
  delayMicroseconds(3);

  // release bus, delay E
  pin_.pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);

  // Unfortunately some frameworks have different characteristics than others
  // esp32 arduino appears to pull the bus low only after the digital_write(false),
  // whereas on esp-idf it already happens during the pin_mode(OUTPUT)
  // manually correct for this with these constants.

#ifdef USE_ESP32
  uint32_t timing_constant = 12;
#else
  uint32_t timing_constant = 14;
#endif

  // measure from start value directly, to get best accurate timing no matter
  // how long pin_mode/delayMicroseconds took
  while (micros() - start < timing_constant)
    ;

  // sample bus to read bit from peer
  bool r = pin_.digital_read();

/*
  // wait for 0 to end
  if (!r){
    while ( (micros() - start) < 60){
      if ( pin_.digital_read() ) {
        delayMicroseconds(2);
        break;
      }
    }
  }
  */
/*
  // read slot is at least 60µs; get as close to 60µs to spend less time with interrupts locked
  uint32_t now = micros();
  if (now - start < 60)
    delayMicroseconds(60 - (now - start));
*/
  return r;
}

void IRAM_ATTR ESPOneWire::write8(uint8_t val) {
  for (uint8_t i = 0; i < 8; i++) {
    this->write_bit(bool((1u << i) & val));
  }
}

void IRAM_ATTR ESPOneWire::write64(uint64_t val) {
  for (uint8_t i = 0; i < 64; i++) {
    this->write_bit(bool((1ULL << i) & val));
  }
}

uint8_t IRAM_ATTR ESPOneWire::read8() {
  uint8_t ret = 0;
  for (uint8_t i = 0; i < 8; i++) {
    ret |= (uint8_t(this->read_bit()) << i);
  }
  return ret;
}
uint64_t IRAM_ATTR ESPOneWire::read64() {
  uint64_t ret = 0;
  for (uint8_t i = 0; i < 8; i++) {
    ret |= (uint64_t(this->read_bit()) << i);
  }
  return ret;
}
void IRAM_ATTR ESPOneWire::select(uint64_t address) {
  this->write8(ONE_WIRE_ROM_SELECT);
  this->write64(address);
}
static int search_count = 0;
void IRAM_ATTR ESPOneWire::reset_search() {
  search_count = 0;
  this->last_discrepancy_ = 0;
  this->last_device_flag_ = false;
  this->rom_number_ = 0;
}
uint64_t ESPOneWire::search() {
  return perform_search(ONE_WIRE_ROM_SEARCH);
}
uint64_t ESPOneWire::active_search() {
  return perform_search(ONE_WIRE_ROM_ACTIVE_SEARCH);
}

uint64_t ESPOneWire::search_select(uint64_t addr, uint8_t cmd) {
  this->last_discrepancy_ = 64;
  this->last_device_flag_ = false;
  this->rom_number_ = addr;
  return this->perform_search(cmd);
}

const uint8_t TRIBIT_SINGLE_BIT = 0x20;
const uint8_t TRIBIT_SECOND_BIT = 0x40;
const uint8_t TRIBIT_BRANCH_BIT = 0x80;

uint8_t IRAM_ATTR ESPOneWire::tribit(bool dir) {
  // read bit
  bool id_bit = this->read_bit();
  // read its complement
  bool cmp_id_bit = this->read_bit();

  if ( id_bit != cmp_id_bit ) {
    dir = id_bit;
  } else {
    // error - no one there
    if (id_bit && cmp_id_bit)
      dir = 1;
  }
  this->write_bit(dir);

  uint8_t res = 0;
  if(id_bit) res |= TRIBIT_SINGLE_BIT;
  if(cmp_id_bit) res |= TRIBIT_SECOND_BIT;
  if(dir) res |= TRIBIT_BRANCH_BIT;

  return res;
}

uint64_t IRAM_ATTR ESPOneWire::perform_search(uint8_t cmd) {
  if (this->last_device_flag_) {
    return 0u;
  }

if(search_count++ > 20) return 0u;

  uint8_t id_bit_number = 1;
  uint8_t last_zero = 0;
  uint8_t rom_byte_number = 0;
  bool search_result = false;
  uint8_t rom_byte_mask = 1;

  {
    InterruptLock lock;
    // Initiate search
    this->write8(cmd);
    do {
      bool branch;

      if (id_bit_number < this->last_discrepancy_) {
        branch = (this->rom_number8_()[rom_byte_number] & rom_byte_mask) > 0;
      } else {
        branch = id_bit_number == this->last_discrepancy_;
      }

      auto tbit = this->tribit(branch);

      // read bit
      bool id_bit = tbit & TRIBIT_SINGLE_BIT;
      // read its complement
      bool cmp_id_bit = tbit & TRIBIT_SECOND_BIT;

      if (id_bit && cmp_id_bit) {
        // No devices participating in search
        break;
      }

      // actual branch taken
      branch =  tbit & TRIBIT_BRANCH_BIT;

      if (!branch & !id_bit & !cmp_id_bit) {
        last_zero = id_bit_number;
      }

      if (branch) {
        // set bit
        this->rom_number8_()[rom_byte_number] |= rom_byte_mask;
      } else {
        // clear bit
        this->rom_number8_()[rom_byte_number] &= ~rom_byte_mask;
      }

      id_bit_number++;
      rom_byte_mask <<= 1;
      if (rom_byte_mask == 0u) {
        // go to next byte
        rom_byte_number++;
        rom_byte_mask = 1;
      }
    } while (rom_byte_number < 8);  // loop through all bytes
  }

  if (id_bit_number >= 65) {
    this->last_discrepancy_ = last_zero;
    if (this->last_discrepancy_ == 0) {
      // we're at root and have no choices left, so this was the last one.
      this->last_device_flag_ = true;
    }
    search_result = true;
  }

  search_result = search_result && (this->rom_number8_()[0] != 0);
  if (!search_result) {
    this->reset_search();
    return 0u;
  }

  return this->rom_number_;
}
std::vector<uint64_t> ESPOneWire::search_vec() {
  std::vector<uint64_t> res;

  this->reset_search();
  while ( true ) {
    {
      InterruptLock lock;
      if (!this->reset()) {
        // Reset failed or no devices present
        this->reset_search();
        break;
      }
    }
    uint64_t address = this->search();
    if ( address == 0u)
      break;
    res.push_back(address);
  }
  return res;
}

void IRAM_ATTR ESPOneWire::skip() {
  this->write8(0xCC);  // skip ROM
}

uint8_t IRAM_ATTR *ESPOneWire::rom_number8_() { return reinterpret_cast<uint8_t *>(&this->rom_number_); }

}  // namespace dallas
}  // namespace esphome
