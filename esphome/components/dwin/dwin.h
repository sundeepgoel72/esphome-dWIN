#pragma once

#include "esphome/components/display/display.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include <vector>

namespace esphome {
namespace dwin {

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

  void set_word(uint16_t vp, uint16_t value);
  void set_words(uint16_t vp, const std::vector<uint16_t> &values);
  void set_text(uint16_t vp, const std::string &text, uint16_t field_len = 32);
  void set_page(uint16_t page);
  void set_brightness(float brightness);
  void request_words(uint16_t vp, uint8_t word_count);

  void add_on_vp_data_callback(std::function<void(uint16_t, std::vector<uint16_t>)> &&callback) {
    this->vp_data_callback_.add(std::move(callback));
  }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override {}
  void send_frame_(uint8_t command, uint16_t vp, const std::vector<uint8_t> &payload);
  void parse_byte_(uint8_t byte);
  void handle_frame_(uint8_t command, const std::vector<uint8_t> &payload);

  std::function<void(DWIN &)> writer_;
  CallbackManager<void(uint16_t, std::vector<uint16_t>)> vp_data_callback_;
  std::vector<uint8_t> rx_buffer_;
  uint16_t brightness_address_{0x0082};
  uint16_t page_address_{0x0084};
  uint32_t frames_rx_{0};
  uint32_t frames_bad_{0};
};

class DWINVPDataTrigger : public Trigger<uint16_t, std::vector<uint16_t>> {
 public:
  explicit DWINVPDataTrigger(DWIN *parent) {
    parent->add_on_vp_data_callback([this](uint16_t vp, std::vector<uint16_t> data) { this->trigger(vp, data); });
  }
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

}  // namespace dwin
}  // namespace esphome
