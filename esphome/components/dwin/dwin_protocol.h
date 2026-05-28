#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace dwin {

// DGUS/DWIN serial frame format used by T5L/DGUS II panels:
//   5A A5 LEN CMD PAYLOAD...
// LEN is the number of bytes following LEN, including CMD.
// Common commands:
//   0x80 write register
//   0x81 read register
//   0x82 write variable pointer (VP)
//   0x83 read/return variable pointer (VP)
static constexpr uint8_t DWIN_FRAME_HEADER_1 = 0x5A;
static constexpr uint8_t DWIN_FRAME_HEADER_2 = 0xA5;
static constexpr uint8_t DWIN_CMD_WRITE_REGISTER = 0x80;
static constexpr uint8_t DWIN_CMD_READ_REGISTER = 0x81;
static constexpr uint8_t DWIN_CMD_WRITE_VP = 0x82;
static constexpr uint8_t DWIN_CMD_READ_VP = 0x83;
static constexpr size_t DWIN_MIN_FRAME_SIZE = 4;
static constexpr size_t DWIN_MAX_FRAME_SIZE = 255 + 3;

// DGUS system/control VPs. These are firmware/project dependent on some panels,
// so the ESPHome component exposes them as configurable defaults.
static constexpr uint16_t DWIN_DEFAULT_BRIGHTNESS_VP = 0x0082;
static constexpr uint16_t DWIN_DEFAULT_PAGE_VP = 0x0084;

// Common DGUS page-switch payload written to VP 0x0084:
//   5A 01 00 PP
// where PP is the target page id.
static constexpr uint8_t DWIN_PAGE_PREFIX_1 = 0x5A;
static constexpr uint8_t DWIN_PAGE_PREFIX_2 = 0x01;

}  // namespace dwin
}  // namespace esphome
