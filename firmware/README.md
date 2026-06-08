# Firmware

Firmware for the self-watering flower pot controller.

## Current Firmware

The active bring-up firmware is `testcode1/`.

It is intentionally conservative:

- Forces `GPIO4 / PUMP_GATE` LOW at boot and every loop.
- Provides no pump-control endpoint.
- Hosts a local AP and diagnostic web UI.
- Reads the moisture ADC continuously.
- Tracks moisture raw, rolling average, min/max/span, and rough diagnostic band.
- Shows live optional input short status for `TP6-TP7` and `TP10-TP9`.
- Provides LED test buttons for the red and green board LEDs.

## Build

From `firmware/testcode1/`:

```powershell
python -m platformio run
python -m platformio run --target upload
```

The current configuration uses `COM3`; update `platformio.ini` if the ESP32-S3 appears on another port.

## Safety Direction

Future pump firmware should add pump control only after hardware bring-up proves:

- MOSFET orientation is correct.
- `PUMP_GATE` is LOW during boot/reset.
- Flyback diode orientation is correct.
- Pump current is measured.
- A hard maximum runtime is enforced.
- Manual pump tests are short and explicit.
