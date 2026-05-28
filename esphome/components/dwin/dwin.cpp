#include "dwin.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dwin {

static const char *const TAG = "dwin";

void DWIN::setup() { this->rx_buffer_.reserve(128); }

void DWIN::dump_config() {
  ESP_LOGCONFIG(TAG, "DWIN DGUS display:");
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(115200);
  ESP_LOGCONFIG(TAG, "  Brightness VP: 0x%04X", this->brightness_address_);
  ESP_LOGCONFIG(TAG, "  Page VP: 0x%04X", this->page_address_);
  ESP_LOGCONFIG(TAG, "  Frames RX: %u", this->frames_rx_);
  ESP_LOGCONFIG(TAG, "  Bad frames: %u", this->frames_bad_);
}

void DWIN::loop() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    this->parse_byte_(byte);
  }
}

void DWIN::update() {
  if (this->writer_) {
    this->writer_(*this);
  }
}

void DWIN::set_word(uint16_t vp, uint16_t value) { this->set_words(vp, std::vector<uint16_t>{value}); }

void DWIN::set_words(uint16_t vp, const std::vector<uint16_t> &values) {
  std::vector<uint8_t> payload;
  payload.reserve(values.size() * 2);
  for (auto value : values) {
    payload.push_back(uint8_t(value >> 8));
    payload.push_back(uint8_t(value & 0xFF));
  }
  this->send_frame_(0x82, vp, payload);
}

void DWIN::set_text(uint16_t vp, const std::string &text, uint16_t field_len) {
  std::vector<uint8_t> payload;
  payload.reserve(field_len);
  for (uint16_t i = 0; i < field_len; i++) {
    payload.push_back(i < text.size() ? text[i] : 0x00);
  }
  this->send_frame_(0x82, vp, payload);
}

void DWIN::set_page(uint16_t page) { this->set_word(this->page_address_, page); }

void DWIN::set_brightness(float brightness) {
  if (brightness < 0.0f) brightness = 0.0f;
  if (brightness > 1.0f) brightness = 1.0f;
  this->set_word(this->brightness_address_, uint16_t(brightness * 100.0f));
}

void DWIN::request_words(uint16_t vp, uint8_t word_count) { this->send_frame_(0x83, vp, std::vector<uint8_t>{word_count}); }

void DWIN::send_frame_(uint8_t command, uint16_t vp, const std::vector<uint8_t> &payload) {
  const uint8_t len = payload.size() + 3;  // command + VP high + VP low + payload
  this->write_byte(0x5A);
  this->write_byte(0xA5);
  this->write_byte(len);
  this->write_byte(command);
  this->write_byte(uint8_t(vp >> 8));
  this->write_byte(uint8_t(vp & 0xFF));
  for (auto b : payload) this->write_byte(b);
  this->flush();
}

void DWIN::parse_byte_(uint8_t byte) {
  if (this->rx_buffer_.empty() && byte != 0x5A) return;
  if (this->rx_buffer_.size() == 1 && byte != 0xA5) {
    this->rx_buffer_.clear();
    return;
  }

  this->rx_buffer_.push_back(byte);
  if (this->rx_buffer_.size() < 3) return;

  const uint8_t len = this->rx_buffer_[2];
  const size_t expected = size_t(len) + 3;
  if (expected < 6 || expected > 128) {
    this->frames_bad_++;
    this->rx_buffer_.clear();
    return;
  }
  if (this->rx_buffer_.size() < expected) return;

  uint8_t command = this->rx_buffer_[3];
  std::vector<uint8_t> payload(this->rx_buffer_.begin() + 4, this->rx_buffer_.begin() + expected);
  this->rx_buffer_.clear();
  this->frames_rx_++;
  this->handle_frame_(command, payload);
}

void DWIN::handle_frame_(uint8_t command, const std::vector<uint8_t> &payload) {
  if (command != 0x83 || payload.size() < 3) {
    ESP_LOGV(TAG, "Unhandled frame command=0x%02X len=%u", command, payload.size());
    return;
  }

  uint16_t vp = (uint16_t(payload[0]) << 8) | payload[1];
  uint8_t words = payload[2];
  std::vector<uint16_t> data;
  data.reserve(words);
  for (size_t i = 3; i + 1 < payload.size(); i += 2) {
    data.push_back((uint16_t(payload[i]) << 8) | payload[i + 1]);
  }
  this->vp_data_callback_.call(vp, data);
}

}  // namespace dwin
}  // namespace esphome
