import esphome.codegen as cg
from esphome.components import display, uart

CODEOWNERS = ["@sundeepgoel72"]
DEPENDENCIES = ["uart"]

dwin_ns = cg.esphome_ns.namespace("dwin")
DWIN = dwin_ns.class_("DWIN", display.Display, uart.UARTDevice)
dwin_ref = DWIN.operator("ref")
