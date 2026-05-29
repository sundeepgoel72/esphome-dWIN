import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_BRIGHTNESS, CONF_ID, CONF_LAMBDA
from esphome.core import TimePeriod

from . import DWIN, dwin_ref

CONF_BRIGHTNESS_ADDRESS = "brightness_address"
CONF_COMMAND_SPACING = "command_spacing"
CONF_MAX_QUEUE_SIZE = "max_queue_size"
CONF_PAGE_ADDRESS = "page_address"

CONFIG_SCHEMA = (
    display.BASIC_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DWIN),
            cv.Optional(CONF_BRIGHTNESS): cv.percentage,
            cv.Optional(CONF_BRIGHTNESS_ADDRESS, default=0x0082): cv.hex_uint16_t,
            cv.Optional(CONF_COMMAND_SPACING, default="0ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(max=TimePeriod(milliseconds=255)),
            ),
            cv.Optional(CONF_MAX_QUEUE_SIZE, default=32): cv.positive_int,
            cv.Optional(CONF_PAGE_ADDRESS, default=0x0084): cv.hex_uint16_t,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await uart.register_uart_device(var, config)
    await display.register_display(var, config)

    cg.add(var.set_brightness_address(config[CONF_BRIGHTNESS_ADDRESS]))
    cg.add(var.set_command_spacing(config[CONF_COMMAND_SPACING].total_milliseconds))
    cg.add(var.set_max_queue_size(config[CONF_MAX_QUEUE_SIZE]))
    cg.add(var.set_page_address(config[CONF_PAGE_ADDRESS]))

    if CONF_BRIGHTNESS in config:
        cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(dwin_ref, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
