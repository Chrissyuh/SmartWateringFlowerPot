# Firmware

Firmware for the self-watering flower pot controller.

## Current Firmware

The active bring-up firmware is `testcode1/`.

It is a controlled pump-test build:

- Starts `GPIO4 / PUMP_GATE` LOW at boot.
- Allows a web UI pump pulse only after a first-use browser warning.
- Caps pump runtime in firmware at `2000 ms`.
- Hosts a local AP at all times.
- Can optionally join home Wi-Fi from the settings form while keeping the AP online.
- Prints USB serial UI hints so a Windows helper can open the web UI from a data-cable connection.
- Reads the moisture ADC continuously.
- Tracks moisture raw, rolling average, min/max/span, and rough diagnostic band.
- Records a rolling moisture/client history buffer and exposes collapsible charts in the hosted page.
- Downloads a session JSON export with firmware identity, moisture stats/history, AP-client counters, LED-test counters, flow pulses, and pump-run count.
- Exposes a deep diagnostics panel/API for reset reasons, boot count, pump-interrupted reset clues, heap/flash/sketch state, chip temperature, GPIO state, and loop health.
- Shows live optional input short status for `TP6-TP7` and `TP10-TP9`.
- Provides LED test buttons for the red and green board LEDs.

## Build

From `firmware/testcode1/`:

```powershell
python -m platformio run
python -m platformio run --target upload
```

The current configuration uses `COM3`; update `platformio.ini` if the ESP32-S3 appears on another port.

To open the UI from Windows over the USB serial discovery path:

```powershell
cd firmware\testcode1
.\tools\open-flowerpot-ui.ps1
```

For a simple plug-in watcher:

```powershell
cd firmware\testcode1
.\tools\watch-flowerpot-ui.ps1
```

This does not turn USB into Ethernet and does not switch Wi-Fi networks. It opens the ESP32's home-Wi-Fi URL when available, otherwise the AP URL.

## Pump Safety Direction

The current firmware is still only for supervised testing. Future autonomous pump firmware should add watering control only after hardware bring-up proves:

- MOSFET orientation is correct.
- `PUMP_GATE` is LOW during boot/reset.
- Flyback diode orientation is correct.
- Pump current is measured.
- A hard maximum runtime is enforced.
- Manual pump tests are short and explicit.
