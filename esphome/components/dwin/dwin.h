#pragma once

#include "dwin_protocol.h"
#include "esphome/components/display/display.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include <deque>
#include <vector>

namespace esphome {
namespace dwin {

enum class DWINParserState : uint8_t {
  WAIT_HEADER_1,
  WAIT_HEADER_2,
  WAIT_LENGTH,
  READ_PAYLOAD,
};

struct DWINCommand {
  uint8_t command;
  uint16_t address;
  std::vector<uint8_t> payload;
};

class DWIN : public PollingComponent, public uart::UARTDevice, public display::DisplayBuffer {
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

  void write_register(uint8_t address, const std::vector<uint8_t> &payload);
  void read_register(uint8_t address, uint8_t byte_count);
  void set_word(uint16_t vp, uint16_t value);
  void set_words(uint16_t vp, const std::vector<uint16_t> &values);
  void set_bytes(uint16_t vp, const std::vector<uint8_t> &payload);
  void set_text(uint16_t vp, const std::string &text, uint16_t field_len = 32);
  void set_page(uint16_t page);
  void set_brightness(float brightness);
  void request_words(uint16_t vp, uint8_t word_count);

  void add_on_vp_data_callback(std::function<void(uint16_t, std::vector<uint16_t>)> &&callback) {
    this->vp_data_callback_.add(std::move(callback));
  }
  void add_on_register_data_callback(std::function<void(uint8_t, std::vector<uint8_t>)> &&callback) {
    this->register_data_callback_.add(std::move(callback));
  }
  void add_buffer_overflow_callback(std::function<void()> &&callback) { this->buffer_overflow_callback_.add(std::move(callback)); }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override {}
  void queue_command_(uint8_t command, uint16_t address, const std::vector<uint8_t> &payload);
  void flush_next_command_();
  void send_frame_(const DWINCommand &command);
  void reset_parser_();
  void parse_byte_(uint8_t byte);
  void handle_frame_(uint8_t command, const std::vector<uint8_t> &payload);
  void handle_vp_frame_(const std::vector<uint8_t> &payload);
  void handle_register_frame_(const std::vector<uint8_t> &payload);

  std::function<void(DWIN &)> writer_;
  CallbackManager<void(uint16_t, std::vector<uint16_t>)> vp_data_callback_;
  CallbackManager<void(uint8_t, std::vector<uint8_t>)> register_data_callback_;
  CallbackManager<void()> buffer_overflow_callback_;

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

class DWINVPDataTrigger : public Trigger<uint16_t, std::vector<uint16_t>> {
 public:
  explicit DWINVPDataTrigger(DWIN *parent) {
    parent->add_on_vp_data_callback([this](uint16_t vp, std::vector<uint16_t> data) { this->trigger(vp, data); });
  }
};

class DWINRegisterDataTrigger : public Trigger<uint8_t, std::vector<uint8_t>> {
 public:
  explicit DWINRegisterDataTrigger(DWIN *parent) {
    parent->add_on_register_data_callback([this](uint8_t address, std::vector<uint8_t> data) { this->trigger(address, data); });
  }
};

class DWINBufferOverflowTrigger : public Trigger<> {
 public:
  explicit DWINBufferOverflowTrigger(DWIN *parent) { parent->add_buffer_overflow_callback([this]() { this->trigger(); }); }
};

template<typename... Ts> class DWINWriteWordAction : public Action<Ts...> {
 public:
  explicit DWINWriteWordAction(DWIN *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(uint16_t, value)
  void play(Ts... x) override { this->parent_->set_word(this->address_.value(x...), this->value_.value(x...)); }

 protected:
  DWIN *parent_;
};

template<typename... Ts> class DWINWriteTextAction : public Action<Ts...> {
 public:
  explicit DWINWriteTextAction(DWIN *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(std::string, text)
  void set_length(uint16_t length) { this->length_ = length; }
  void play(Ts... x) override { this->parent_->set_text(this->address_.value(x...), this->text_.value(x...), this->length_); }

 protected:
  DWIN *parent_;
  uint16_t length_{32};
};

template<typename... Ts> class DWINReadVPAction : public Action<Ts...> {
 public:
  explicit DWINReadVPAction(DWIN *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(uint8_t, words)
  void play(Ts... x) override { this->parent_->request_words(this->address_.value(x...), this->words_.value(x...)); }

 protected:
  DWIN *parent_;
};

template<typename... Ts> class DWINSetPageAction : public Action<Ts...> {
 public:
  explicit DWINSetPageAction(DWIN *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, page)
  void play(Ts... x) override { this->parent_->set_page(this->page_.value(x...)); }

 protected:
  DWIN *parent_;
};

template<typename... Ts> class DWINSetBrightnessAction : public Action<Ts...> {
 public:
  explicit DWINSetBrightnessAction(DWIN *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(float, brightness)
  void play(Ts... x) override { this->parent_->set_brightness(this->brightness_.value(x...)); }

 protected:
  DWIN *parent_;
};

}  // namespace dwin
}  // namespace esphome
