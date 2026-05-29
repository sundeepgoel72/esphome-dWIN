# External component testing workflow

This repo currently supports two layouts:

1. `components/dwin/` for easy ESPHome `external_components` testing.
2. `esphome/components/dwin/` as an upstream reference layout for a future ESPHome PR.

During active development, test through the external component layout. Once the component is stable on hardware, copy/sync the final component back into `esphome/components/dwin/` and prepare a clean upstream PR.

## Example ESPHome config

Use:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/sundeepgoel72/esphome-dWIN
      ref: dev
    components: [dwin]
```

Then use the normal display platform:

```yaml
display:
  - platform: dwin
    id: panel
    update_interval: 2s
    command_spacing: 10ms
    lambda: |-
      it.set_text(0x1000, "ESPHome DWIN", 24);
      it.set_word(0x1100, 42);
      it.request_words(0x2000, 2);
```

A complete example is available at:

```text
examples/external_component_esp32.yaml
```

## Recommended approach

Use external component mode while:

- testing against the Python simulator
- testing on ESP32 hardware
- discovering DWIN/DGUS VP quirks
- adding touch/sensor/text abstractions
- changing YAML schema often

Move back to native ESPHome PR mode only after:

- hardware behavior is stable
- logs and VP maps are understood
- CI passes
- docs are mostly final
- the commit history can be cleaned up
