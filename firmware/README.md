# Smart Pot V2 Firmware Notes

Firmware is not implemented yet. The hardware should support this behavior:

- Initialize display, sensor inputs, encoder, fault LED, and pump output OFF at boot.
- Read calibration/settings from nonvolatile memory.
- Average moisture ADC readings and convert to calibrated moisture percentage.
- Use hysteresis around the watering threshold.
- Enforce cooldown after watering.
- Enforce maximum pump runtime or maximum volume per watering event.
- Lock out pump operation when reservoir is empty.
- If a flow sensor is populated, count pulses with an interrupt-capable GPIO and fault on no-flow.
- Show moisture, pump state, reservoir/fault state, calibration mode, and settings on OLED.
- Allow manual pump test only with a safety timeout.
