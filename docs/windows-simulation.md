# Simulating a DWIN/DGUS panel from Windows

You can test the ESPHome DWIN component without a physical DWIN panel by using a Windows virtual null-modem pair and the Python simulator in this repo.

## Recommended setup

Install a virtual null-modem driver such as `com0com`, then create a pair, for example:

```text
COM10 <-> COM11
```

Run the simulator on one side:

```powershell
python -m pip install pyserial
python tools\dwin_simulator.py --port COM11 --baud 115200 --inject-touch-vp 0x2000
```

Point your test client, UART bridge, or ESPHome-side serial process at the other side (`COM10`).

## What the simulator does

It accepts common DGUS frames:

```text
5A A5 LEN 82 VP_H VP_L DATA...        write VP
5A A5 04 83 VP_H VP_L WORD_COUNT      read VP
```

It stores VP words in memory and replies to VP reads with:

```text
5A A5 LEN 83 VP_H VP_L WORD_COUNT DATA_H DATA_L ...
```

It also has basic register command support:

```text
0x80 write register
0x81 read register / register response
```

## Generate test frames manually

Use:

```powershell
python tools\dwin_frame_cli.py write-word 0x1100 1234
python tools\dwin_frame_cli.py write-text 0x1000 "Hello" 16
python tools\dwin_frame_cli.py read-vp 0x2000 2
```

Expected output examples:

```text
5A A5 05 82 11 00 04 D2
5A A5 04 83 20 00 02
```

## Practical test path

1. Validate frame generation with `dwin_frame_cli.py`.
2. Run `dwin_simulator.py` and confirm it logs frames.
3. Use virtual COM ports for protocol-only tests.
4. Move to ESP32 + real DWIN panel once framing is confirmed.

## Limitations

This simulator validates protocol framing and VP read/write behavior. It does not emulate DGUS rendering, fonts, icons, touch widgets, project files, curves, RTC, or page memory. Hardware testing is still required before an upstream PR.
