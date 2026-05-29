import esphome.codegen as cg

CODEOWNERS = ["@sundeepgoel72"]
DEPENDENCIES = ["uart"]

dwin_ns = cg.esphome_ns.namespace("dwin")
DWIN = dwin_ns.class_("DWIN")
dwin_ref = DWIN.operator("ref")
