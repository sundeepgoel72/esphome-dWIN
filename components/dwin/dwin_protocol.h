#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace dwin {

static constexpr uint8_t DWIN_FRAME_HEADER_1 = 0x5A;
static constexpr uint8_t DWIN_FRAME_HEADER_2 = 0xA5;
static constexpr uint8_t DWIN_CMD_WRITE_REGISTER = 0x80;
static constexpr uint8_t DWIN_CMD_READ_REGISTER = 0x81;
static constexpr uint8_t DWIN_CMD_WRITE_VP = 0x82;
static constexpr uint8_t DWIN_CMD_READ_VP = 0x83;
static constexpr size_t DWIN_MAX_FRAME_SIZE = 255 + 3;
static constexpr uint16_t DWIN_DEFAULT_BRIGHTNESS_VP = 0x0082;
static constexpr uint16_t DWIN_DEFAULT_PAGE_VP = 0x0084;
static constexpr uint8_t DWIN_PAGE_PREFIX_1 = 0x5A;
static constexpr uint8_t DWIN_PAGE_PREFIX_2 = 0x01;

}  // namespace dwin
}  // namespace esphome
