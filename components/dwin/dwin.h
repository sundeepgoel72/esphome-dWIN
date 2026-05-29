#pragma once

#include "dwin_protocol.h"
#include "esphome/components/display/display.h"
#include "esphome/components/uart/uart.h"

#include <deque>
#include <vector>

namespace esphome {
namespace dwin {

enum class DWINParserState : uint8_t { WAIT_HEADER_1, WAIT_HEADER_2, WAIT_LENGTH, READ_PAYLOAD };

struct DWINCommand {
  uint8_t command;
  uint16_t address;
  std::vector<uint8_t> payload;
};

class DWIN : public display::Display, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

  void set_writer(std::function<void(DWIN &)> &&writer) { this->writer_ = writer; }
  void set_brightness_address(uint16_t address) { this->brightness_address_ = address; }
  void set_page_address(uint16_t address) { this->page_address_ = address; }
  void set_command_spacing(uint32_t spacing_ms) { this->command_spacing_ms_ = spacing_ms; }
  void set_max_queue_size(uint16_t max_queue_size) { this->max_queue_size_ = max_queue_size; }

  void set_word(uint16_t vp, uint16_t value);
  void set_words(uint16_t vp, const std::vector<uint16_t> &values);
  void set_bytes(uint16_t vp, const std::vector<uint8_t> &payload);
  void set_text(uint16_t vp, const std::string &text, uint16_t field_len = 32);
  void set_page(uint16_t page);
  void set_brightness(float brightness);
  void request_words(uint16_t vp, uint8_t word_count);

 protected:
  void draw_pixel_at(int x, int y, Color color) override {}
  int get_width_internal() override { return 0; }
  int get_height_internal() override { return 0; }

  void queue_command_(uint8_t command, uint16_t address, const std::vector<uint8_t> &payload);
  void flush_next_command_();
  void send_frame_(const DWINCommand &command);
  void reset_parser_();
  void parse_byte_(uint8_t byte);
  void handle_frame_(uint8_t command, const std::vector<uint8_t> &payload);

  std::function<void(DWIN &)> writer_;
  DWINParserState parser_state_{DWINParserState::WAIT_HEADER_1};
  uint8_t expected_payload_len_{0};
  std::vector<uint8_t> rx_payload_;
  std::deque<DWINCommand> command_queue_;
  uint32_t last_command_ms_{0};
  uint32_t command_spacing_ms_{0};
  uint16_t max_queue_size_{32};
  uint16_t brightness_address_{DWIN_DEFAULT_BRIGHTNESS_VP};
  uint16_t page_address_{DWIN_DEFAULT_PAGE_VP};
  uint32_t frames_rx_{0};
  uint32_t frames_bad_{0};
  uint32_t frames_tx_{0};
  uint32_t queue_overflows_{0};
};

}  // namespace dwin
}  // namespace esphome
