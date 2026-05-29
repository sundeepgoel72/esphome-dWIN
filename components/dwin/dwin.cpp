#include "dwin.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dwin {

static const char *const TAG = "dwin";

void DWIN::setup() { this->rx_payload_.reserve(64); }

void DWIN::dump_config() {
  ESP_LOGCONFIG(TAG, "DWIN DGUS display:");
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(115200);
  ESP_LOGCONFIG(TAG, "  Brightness VP: 0x%04X", this->brightness_address_);
  ESP_LOGCONFIG(TAG, "  Page VP: 0x%04X", this->page_address_);
  ESP_LOGCONFIG(TAG, "  Command spacing: %ums", this->command_spacing_ms_);
  ESP_LOGCONFIG(TAG, "  Max queue size: %u", this->max_queue_size_);
  ESP_LOGCONFIG(TAG, "  Frames TX/RX: %u/%u", this->frames_tx_, this->frames_rx_);
  ESP_LOGCONFIG(TAG, "  Bad frames: %u", this->frames_bad_);
  ESP_LOGCONFIG(TAG, "  Queue overflows: %u", this->queue_overflows_);
}

void DWIN::loop() { while (this->available()) { uint8_t byte; if (this->read_byte(&byte)) this->parse_byte_(byte); } this->flush_next_command_(); }
void DWIN::update() { if (this->writer_) this->writer_(*this); }
void DWIN::write_register(uint8_t address, const std::vector<uint8_t> &payload) { this->queue_command_(DWIN_CMD_WRITE_REGISTER, address, payload); }
void DWIN::read_register(uint8_t address, uint8_t byte_count) { this->queue_command_(DWIN_CMD_READ_REGISTER, address, std::vector<uint8_t>{byte_count}); }
void DWIN::set_word(uint16_t vp, uint16_t value) { this->set_words(vp, std::vector<uint16_t>{value}); }
void DWIN::set_words(uint16_t vp, const std::vector<uint16_t> &values) { std::vector<uint8_t> payload; payload.reserve(values.size() * 2); for (auto value : values) { payload.push_back(uint8_t(value >> 8)); payload.push_back(uint8_t(value & 0xFF)); } this->set_bytes(vp, payload); }
void DWIN::set_bytes(uint16_t vp, const std::vector<uint8_t> &payload) { this->queue_command_(DWIN_CMD_WRITE_VP, vp, payload); }
void DWIN::set_text(uint16_t vp, const std::string &text, uint16_t field_len) { std::vector<uint8_t> payload; payload.reserve(field_len); for (uint16_t i = 0; i < field_len; i++) payload.push_back(i < text.size() ? text[i] : 0x00); this->set_bytes(vp, payload); }
void DWIN::set_page(uint16_t page) { this->set_bytes(this->page_address_, std::vector<uint8_t>{DWIN_PAGE_PREFIX_1, DWIN_PAGE_PREFIX_2, 0x00, uint8_t(page & 0xFF)}); }
void DWIN::set_brightness(float brightness) { if (brightness < 0.0f) brightness = 0.0f; if (brightness > 1.0f) brightness = 1.0f; this->set_word(this->brightness_address_, uint16_t(brightness * 100.0f)); }
void DWIN::request_words(uint16_t vp, uint8_t word_count) { this->queue_command_(DWIN_CMD_READ_VP, vp, std::vector<uint8_t>{word_count}); }

void DWIN::queue_command_(uint8_t command, uint16_t address, const std::vector<uint8_t> &payload) { if (this->command_queue_.size() >= this->max_queue_size_) { this->queue_overflows_++; this->buffer_overflow_callback_.call(); ESP_LOGW(TAG, "Command queue overflow; dropping command 0x%02X address 0x%04X", command, address); return; } this->command_queue_.push_back(DWINCommand{command, address, payload}); }
void DWIN::flush_next_command_() { if (this->command_queue_.empty()) return; const uint32_t now = millis(); if (this->command_spacing_ms_ != 0 && now - this->last_command_ms_ < this->command_spacing_ms_) return; this->send_frame_(this->command_queue_.front()); this->command_queue_.pop_front(); this->last_command_ms_ = now; }
void DWIN::send_frame_(const DWINCommand &command) { const bool is_register_command = command.command == DWIN_CMD_WRITE_REGISTER || command.command == DWIN_CMD_READ_REGISTER; const uint8_t address_len = is_register_command ? 1 : 2; const uint8_t len = 1 + address_len + command.payload.size(); this->write_byte(DWIN_FRAME_HEADER_1); this->write_byte(DWIN_FRAME_HEADER_2); this->write_byte(len); this->write_byte(command.command); if (is_register_command) { this->write_byte(uint8_t(command.address & 0xFF)); } else { this->write_byte(uint8_t(command.address >> 8)); this->write_byte(uint8_t(command.address & 0xFF)); } for (auto b : command.payload) this->write_byte(b); this->flush(); this->frames_tx_++; }
void DWIN::reset_parser_() { this->parser_state_ = DWINParserState::WAIT_HEADER_1; this->expected_payload_len_ = 0; this->rx_payload_.clear(); }
void DWIN::parse_byte_(uint8_t byte) { switch (this->parser_state_) { case DWINParserState::WAIT_HEADER_1: if (byte == DWIN_FRAME_HEADER_1) this->parser_state_ = DWINParserState::WAIT_HEADER_2; break; case DWINParserState::WAIT_HEADER_2: if (byte == DWIN_FRAME_HEADER_2) this->parser_state_ = DWINParserState::WAIT_LENGTH; else if (byte != DWIN_FRAME_HEADER_1) this->reset_parser_(); break; case DWINParserState::WAIT_LENGTH: if (byte == 0 || byte > DWIN_MAX_FRAME_SIZE - 3) { this->frames_bad_++; this->reset_parser_(); return; } this->expected_payload_len_ = byte; this->rx_payload_.clear(); this->parser_state_ = DWINParserState::READ_PAYLOAD; break; case DWINParserState::READ_PAYLOAD: this->rx_payload_.push_back(byte); if (this->rx_payload_.size() >= this->expected_payload_len_) { const uint8_t command = this->rx_payload_[0]; std::vector<uint8_t> payload(this->rx_payload_.begin() + 1, this->rx_payload_.end()); this->reset_parser_(); this->frames_rx_++; this->handle_frame_(command, payload); } break; } }
void DWIN::handle_frame_(uint8_t command, const std::vector<uint8_t> &payload) { switch (command) { case DWIN_CMD_READ_VP: this->handle_vp_frame_(payload); break; case DWIN_CMD_READ_REGISTER: this->handle_register_frame_(payload); break; default: ESP_LOGV(TAG, "Unhandled frame command=0x%02X len=%u", command, payload.size()); break; } }
void DWIN::handle_vp_frame_(const std::vector<uint8_t> &payload) { if (payload.size() < 3) { this->frames_bad_++; return; } const uint16_t vp = (uint16_t(payload[0]) << 8) | payload[1]; const uint8_t words = payload[2]; std::vector<uint16_t> data; data.reserve(words); for (size_t i = 3; i + 1 < payload.size() && data.size() < words; i += 2) data.push_back((uint16_t(payload[i]) << 8) | payload[i + 1]); this->vp_data_callback_.call(vp, data); }
void DWIN::handle_register_frame_(const std::vector<uint8_t> &payload) { if (payload.size() < 2) { this->frames_bad_++; return; } const uint8_t address = payload[0]; std::vector<uint8_t> data(payload.begin() + 1, payload.end()); this->register_data_callback_.call(address, data); }

}  // namespace dwin
}  // namespace esphome
