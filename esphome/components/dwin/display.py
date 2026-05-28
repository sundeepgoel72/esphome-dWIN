from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_BRIGHTNESS, CONF_ID, CONF_LAMBDA
from esphome.core import TimePeriod

from . import DWIN, dwin_ns, dwin_ref

CONF_ADDRESS = "address"
CONF_BRIGHTNESS_ADDRESS = "brightness_address"
CONF_COMMAND_SPACING = "command_spacing"
CONF_LENGTH = "length"
CONF_MAX_QUEUE_SIZE = "max_queue_size"
CONF_ON_BUFFER_OVERFLOW = "on_buffer_overflow"
CONF_ON_REGISTER_DATA = "on_register_data"
CONF_ON_VP_DATA = "on_vp_data"
CONF_PAGE = "page"
CONF_PAGE_ADDRESS = "page_address"
CONF_TEXT = "text"
CONF_VALUE = "value"
CONF_WORDS = "words"

DWINWriteWordAction = dwin_ns.class_("DWINWriteWordAction", automation.Action)
DWINWriteTextAction = dwin_ns.class_("DWINWriteTextAction", automation.Action)
DWINReadVPAction = dwin_ns.class_("DWINReadVPAction", automation.Action)
DWINSetPageAction = dwin_ns.class_("DWINSetPageAction", automation.Action)
DWINSetBrightnessAction = dwin_ns.class_("DWINSetBrightnessAction", automation.Action)
DWINVPDataTrigger = dwin_ns.class_(
    "DWINVPDataTrigger",
    automation.Trigger.template(cg.uint16, cg.std_vector.template(cg.uint16)),
)
DWINRegisterDataTrigger = dwin_ns.class_(
    "DWINRegisterDataTrigger",
    automation.Trigger.template(cg.uint8, cg.std_vector.template(cg.uint8)),
)
DWINBufferOverflowTrigger = dwin_ns.class_("DWINBufferOverflowTrigger", automation.Trigger.template())

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
            cv.Optional(CONF_ON_BUFFER_OVERFLOW): automation.validate_automation(
                {cv.GenerateID(): cv.declare_id(DWINBufferOverflowTrigger)}
            ),
            cv.Optional(CONF_ON_REGISTER_DATA): automation.validate_automation(
                {cv.GenerateID(): cv.declare_id(DWINRegisterDataTrigger)}
            ),
            cv.Optional(CONF_ON_VP_DATA): automation.validate_automation(
                {cv.GenerateID(): cv.declare_id(DWINVPDataTrigger)}
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


def _automation_schema(extra):
    return cv.Schema({cv.GenerateID(): cv.use_id(DWIN), **extra})


@automation.register_action(
    "display.dwin.write_word",
    DWINWriteWordAction,
    _automation_schema(
        {
            cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
            cv.Required(CONF_VALUE): cv.templatable(cv.uint16_t),
        }
    ),
)
async def write_word_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_address(await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)))
    cg.add(var.set_value(await cg.templatable(config[CONF_VALUE], args, cg.uint16)))
    return var


@automation.register_action(
    "display.dwin.write_text",
    DWINWriteTextAction,
    _automation_schema(
        {
            cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
            cv.Required(CONF_TEXT): cv.templatable(cv.string),
            cv.Optional(CONF_LENGTH, default=32): cv.uint16_t,
        }
    ),
)
async def write_text_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_address(await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)))
    cg.add(var.set_text(await cg.templatable(config[CONF_TEXT], args, cg.std_string)))
    cg.add(var.set_length(config[CONF_LENGTH]))
    return var


@automation.register_action(
    "display.dwin.read_vp",
    DWINReadVPAction,
    _automation_schema(
        {
            cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
            cv.Required(CONF_WORDS): cv.templatable(cv.uint8_t),
        }
    ),
)
async def read_vp_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_address(await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)))
    cg.add(var.set_words(await cg.templatable(config[CONF_WORDS], args, cg.uint8)))
    return var


@automation.register_action(
    "display.dwin.set_page",
    DWINSetPageAction,
    cv.maybe_simple_value(
        _automation_schema({cv.Required(CONF_PAGE): cv.templatable(cv.uint16_t)}),
        key=CONF_PAGE,
    ),
)
async def set_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_page(await cg.templatable(config[CONF_PAGE], args, cg.uint16)))
    return var


@automation.register_action(
    "display.dwin.set_brightness",
    DWINSetBrightnessAction,
    cv.maybe_simple_value(
        _automation_schema({cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.percentage)}),
        key=CONF_BRIGHTNESS,
    ),
)
async def set_brightness_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_brightness(await cg.templatable(config[CONF_BRIGHTNESS], args, cg.float_)))
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
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

    for conf in config.get(CONF_ON_VP_DATA, []):
        trigger = cg.new_Pvariable(conf[CONF_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.uint16, "vp"), (cg.std_vector.template(cg.uint16), "data")],
            conf,
        )

    for conf in config.get(CONF_ON_REGISTER_DATA, []):
        trigger = cg.new_Pvariable(conf[CONF_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.uint8, "address"), (cg.std_vector.template(cg.uint8), "data")],
            conf,
        )

    for conf in config.get(CONF_ON_BUFFER_OVERFLOW, []):
        trigger = cg.new_Pvariable(conf[CONF_ID], var)
        await automation.build_automation(trigger, [], conf)
