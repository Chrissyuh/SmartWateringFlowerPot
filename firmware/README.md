# Self-Watering Flower Pot V2 Firmware Notes

Firmware is not implemented yet. The hardware should support this behavior:

- Initialize moisture input, optional sensor inputs, error/status LEDs, and pump output OFF at boot.
- Read calibration/settings from nonvolatile memory.
- Average moisture ADC readings and convert to calibrated moisture percentage.
- Use hysteresis around the watering threshold.
- Enforce cooldown after watering.
- Enforce maximum pump runtime or maximum volume per watering event.
- Default `RESERVOIR_SENSOR_ENABLED=false`.
- Default `FLOW_SENSOR_ENABLED=false`.
- Missing reservoir/flow hardware must not block watering unless explicitly enabled.
- If a reservoir switch is enabled later, use the ESP32 internal pullup and switch to GND.
- If a flow sensor is populated later, count pulses with an interrupt-capable GPIO and fault on no-flow.
- Use green/status LED for normal state and pump activity.
- Use red/error LED for fault state.
- Use USB serial or compile-time constants for calibration/settings in the cost-down version.
- Allow manual pump test only with a safety timeout.
