# Self-Watering Flower Pot V2 Bring-Up Checklist

Use this checklist after boards arrive. Do not connect the pump until power rails, MCU behavior, and MOSFET gate behavior have been checked.

## Required Bench Setup

- Use a current-limited 5 V bench supply for first power-up if available.
- Use a dedicated 5 V, >=2 A USB adapter or USB power bank for pump operation.
- Do not run the pump while powered from a laptop USB port.
- Use a real USB-C data cable for flashing; charge-only cables will not work.
- Keep a 3.3 V USB-UART adapter available for J7 fallback programming. Never use 5 V UART logic.

## Before Soldering

- Inspect PCB for fab defects, scratches, shorts, and missing mask.
- Confirm board revision text says `Self-Watering Flower Pot V2 Rev A`.
- Confirm USB-C footprint orientation.
- Confirm ESP32 module orientation and antenna keepout.
- Confirm diode polarity markings.
- Confirm MOSFET footprint orientation against the AO3400A datasheet.
- Confirm connector labels and pin 1 markers.
- Confirm mounting holes and board outline fit the enclosure plan.

## Power Section Only

- Solder USB-C input, buck regulator, inductor, required resistors/capacitors, and rail test points first if assembly order allows.
- Do not install ESP32 until `+3V3` has been verified if practical.
- Apply current-limited 5V power.
- Measure `+5V` to `GND`.
- Measure `+3V3` to `GND`.
- Check regulator temperature.
- Check for shorts between `+5V/GND` and `+3V3/GND`.
- Confirm power LED behavior if populated.

## MCU Bring-Up

- Install ESP32-S3 module and required support parts.
- Verify ESP32 power pins receive `+3V3`.
- Verify `EN` and `BOOT` behavior.
- Check USB detection with a known data-capable USB-C cable.
- Program a minimal blink/status firmware through native USB-C first.
- If native USB flashing fails, use J7 fallback: hold `IO0/BOOT` low, pulse `EN` low, release `EN`, then upload through a 3.3 V USB-UART adapter.
- Confirm `PUMP_GATE` remains LOW during reset and boot.
- Confirm error/status LED GPIO polarity.

## Sensor and UI Tests

- Run an I2C scan and confirm OLED address.
- Display a test status page on OLED.
- Read encoder A/B transitions.
- Read encoder push button.
- Read moisture ADC in air.
- Read moisture ADC with sensor in damp soil or water only if the selected sensor supports it.
- Toggle reservoir switch and verify firmware blocks watering when empty.
- If flow sensor is installed, verify it outputs 3.3 V-safe pulses before connecting it to Board A.
- If flow sensor is installed, verify pulse counting with pump disconnected or dry-safe test setup.

## Pump Driver Tests

- Keep pump disconnected at first.
- Toggle pump GPIO and measure `PUMP_GATE`.
- Confirm gate pulldown drives MOSFET off when MCU is reset or unpowered.
- Test with a dummy load or LED/resistor if useful.
- Connect pump with current-limited 5V supply.
- Run a very short pump pulse.
- Measure pump startup and running current.
- Stop testing if pump startup causes ESP32 reset, USB dropout, or excessive voltage sag.
- Check MOSFET temperature.
- Check flyback diode orientation and temperature.
- Verify firmware max runtime stops the pump.
- Verify pump cannot turn on during reset/boot.

## Firmware Safety Checks

- Hysteresis prevents rapid pump toggling.
- Cooldown prevents repeated watering.
- Maximum runtime or maximum volume limit is enforced.
- Empty reservoir locks out pump operation.
- No-flow fault locks out pump if flow sensor is populated and no pulses arrive.
- Calibration values persist after reset.
- OLED and error LED show fault states clearly.
