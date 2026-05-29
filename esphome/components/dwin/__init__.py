import esphome.codegen as cg
from esphome.components import display, uart

CODEOWNERS = ["@sundeepgoel72"]
DEPENDENCIES = ["uart"]

dwin_ns = cg.esphome_ns.namespace("dwin")
DWIN = dwin_ns.class_("DWIN", cg.PollingComponent, uart.UARTDevice, display.DisplayBuffer)
dwin_ref = DWIN.operator("ref")
