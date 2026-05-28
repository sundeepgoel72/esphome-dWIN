# ESPHome DWIN work branch

This fork is being shaped as an upstreamable ESPHome PR for DWIN DGUS/T5L HMI displays.

The implementation lives in the normal ESPHome tree:

- `esphome/components/dwin/`
- `tests/components/dwin/`
- `docs/components/display/dwin.rst`

## Implemented protocol coverage

The component implements the common DGUS serial frame structure:

```text
5A A5 LEN CMD PAYLOAD...
```

Where `LEN` is the number of bytes after the length byte, including `CMD`.

Current command coverage:

| Command | Meaning | Status |
|---|---|---|
| `0x80` | Write register | Low-level C++ helper |
| `0x81` | Read/register response | Parser callback |
| `0x82` | Write VP | Actions + C++ helpers |
| `0x83` | Read VP / VP response | Action + parser callback |

## Implemented ESPHome surface

Configuration:

- `brightness`
- `brightness_address`
- `page_address`
- `command_spacing`
- `max_queue_size`
- `on_vp_data`
- `on_register_data`
- `on_buffer_overflow`
- `lambda`

Actions:

- `display.dwin.write_word`
- `display.dwin.write_text`
- `display.dwin.read_vp`
- `display.dwin.set_page`
- `display.dwin.set_brightness`

## Simulation and test harnesses

- Windows simulator: `tools/dwin_simulator.py`
- Frame generator: `tools/dwin_frame_cli.py`
- Windows simulation notes: `docs/windows-simulation.md`

## Notes for testing

DWIN/DGUS panels are driven by the VP map compiled into the HMI project. Before a PR is opened upstream, the component needs validation against a real DGUS project with known VP addresses for:

- text display
- numeric display
- touch key return
- page switching
- brightness
- VP readback

## Upstream PR checklist

- [x] Keep ESPHome fork layout
- [x] Add component under `esphome/components/dwin`
- [x] Add ESPHome compile test YAML
- [x] Add docs skeleton
- [x] Add queueing and command spacing
- [x] Add parser state machine
- [x] Add register and VP command constants
- [x] Add Windows protocol simulator
- [x] Add manual frame generator
- [ ] Run ESPHome lint/CI locally
- [ ] Validate against real DWIN hardware
- [ ] Add binary_sensor/sensor/text_sensor platforms if maintainers prefer entity abstractions over raw VP callbacks
- [ ] Add documentation images/links if requested
- [ ] Open PR against `esphome/esphome:dev`
