# ESPHome DWIN external component

This repo is now intentionally shaped as a clean ESPHome `external_components` repository for DWIN DGUS/T5L displays.

The active component lives only here:

```text
components/dwin/
```

The earlier upstream-fork style `esphome/components/dwin/` tree was removed from `dev` because ESPHome external component resolution was picking that path and behaving differently from normal external component repos.

## Usage

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/sundeepgoel72/esphome-dWIN
      ref: dev
    components: [dwin]
    refresh: 0s

uart:
  id: dwin_uart
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200

display:
  - platform: dwin
    id: panel
    update_interval: 5s
```

## Current goal

First milestone: compile cleanly as an external component on ESPHome 2025.8.x.

After that:

1. Add back actions one by one.
2. Add simulator-driven tests.
3. Add hardware tests.
4. Add VP-backed `sensor`, `binary_sensor`, and `text_sensor` helpers.
5. Once mature, retrofit into an ESPHome fork for upstream PR.
