from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LAMBDA

from . import DWIN, dwin_ns, dwin_ref

CONF_ON_VP_DATA = "on_vp_data"
CONF_ADDRESS = "address"
CONF_VALUE = "value"
CONF_TEXT = "text"
CONF_LENGTH = "length"
CONF_BRIGHTNESS_ADDRESS = "brightness_address"
CONF_PAGE_ADDRESS = "page_address"

DWINWriteWordAction = dwin_ns.class_("DWINWriteWordAction", automation.Action)
DWINWriteTextAction = dwin_ns.class_("DWINWriteTextAction", automation.Action)
DWINVPDataTrigger = dwin_ns.class_(
    "DWINVPDataTrigger",
    automation.Trigger.template(cg.uint16, cg.std_vector.template(cg.uint16)),
)

CONFIG_SCHEMA = (
    display.BASIC_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DWIN),
            cv.Optional(CONF_BRIGHTNESS_ADDRESS, default=0x0082): cv.hex_uint16_t,
            cv.Optional(CONF_PAGE_ADDRESS, default=0x0084): cv.hex_uint16_t,
            cv.Optional(CONF_ON_VP_DATA): automation.validate_automation(
                {cv.GenerateID(): cv.declare_id(DWINVPDataTrigger)}
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


@automation.register_action(
    "display.dwin.write_word",
    DWINWriteWordAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DWIN),
            cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
            cv.Required(CONF_VALUE): cv.templatable(cv.uint16_t),
        }
    ),
)
async def write_word_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    address = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    value = await cg.templatable(config[CONF_VALUE], args, cg.uint16)
    cg.add(var.set_address(address))
    cg.add(var.set_value(value))
    return var


@automation.register_action(
    "display.dwin.write_text",
    DWINWriteTextAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DWIN),
            cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
            cv.Required(CONF_TEXT): cv.templatable(cv.string),
            cv.Optional(CONF_LENGTH, default=32): cv.uint16_t,
        }
    ),
)
async def write_text_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    address = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    text = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_address(address))
    cg.add(var.set_text(text))
    cg.add(var.set_length(config[CONF_LENGTH]))
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await display.register_display(var, config)

    cg.add(var.set_brightness_address(config[CONF_BRIGHTNESS_ADDRESS]))
    cg.add(var.set_page_address(config[CONF_PAGE_ADDRESS]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(dwin_ref, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))

    for conf in config.get(CONF_ON_VP_DATA, []):
        trigger = cg.new_Pvariable(conf[CONF_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.uint16, "vp"), (cg.std_vector.template(cg.uint16), "data")],
            conf,
        )
