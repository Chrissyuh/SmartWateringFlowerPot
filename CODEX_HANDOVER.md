# Self-Watering Flower Pot V2 — Codex + KiCad Handover

## Purpose of this file

This file is the main handover document for Codex while working inside the KiCad project folder for the Self-Watering Flower Pot V2.

Codex should treat this as project context, design intent, safety rules, and a task guide. KiCad GUI remains the source of truth for visual schematic/PCB work. Codex should mostly help with organization, checking, documentation, reports, symbol/footprint sanity review, small controlled edits, scripts, and manufacturing-output automation.

This project is not just a quick prototype. The goal is a product-ish engineering prototype: a custom PCB, clean wiring, serviceable enclosure, calibrated control logic, clear failure modes, and a design that could be explained to mentors, judges, or engineers.

---

## Project summary

Project name: **Self-Watering Flower Pot V2**

Core idea: An ESP32-based plant pot controller that reads a capacitive soil moisture sensor, decides when watering is needed, runs a small 5V pump through a MOSFET driver, optionally verifies water flow, displays status on a small OLED, and lets the user adjust/calibrate settings through a rotary encoder/button UI.

Primary goals:

- Create a custom KiCad PCB instead of a breadboard prototype.
- Make the wiring clean using JST connectors.
- Use a safe low-voltage pump driver with flyback protection.
- Separate noisy motor power from sensitive analog sensor readings as much as practical.
- Support calibration and reliable watering behavior in firmware.
- Design around a 3D-printed or fabricated enclosure in SolidWorks.
- Keep the design realistic for a novice KiCad user while still being serious and well-engineered.

Current status:

- Board A main-controller schematic and PCB are complete enough for manufacturer preview.
- Board B UI daughterboard schematic and PCB are complete enough for manufacturer preview.
- Both boards have current clean ERC, DRC, and schematic-parity reports.
- Fabrication packages, STEP exports, schematic PDFs, routing review outputs, and 3D review images have been generated.
- `docs/pre-order-hardening-review.md` documents the remaining real-world prototype tradeoffs: Rev A has no USB input fuse/current limiter, no USB data ESD array, and no extra local ESP32 bulk capacitor directly beside U1.
- The next major job is not more routing. It is final Gerber preview, final live BOM refresh, explicit acceptance of the Rev A protection tradeoff, and then careful bring-up after boards arrive.

---

## Overall architecture

High-level block diagram:

```text
USB-C 5V input
   |
   +--> 5V rail for pump / maybe external modules
   |
   +--> 3.3V buck regulator
            |
            +--> ESP32-S3 module
            +--> soil moisture sensor logic/reference, if sensor output is 3.3V-safe
            +--> OLED / encoder / LEDs / optional sensor logic

ESP32 ADC1 pin <--- capacitive soil moisture sensor analog output
ESP32 GPIO  ----> MOSFET gate ---> pump low-side switch
ESP32 I2C   ----> OLED display / optional I2C expansion
ESP32 GPIO  <---- rotary encoder A/B + push button
ESP32 GPIO  <---- reservoir float switch or low-water sensor
ESP32 GPIO  <---- optional flow sensor pulse output
ESP32 GPIO  ----> red error LED / status LEDs
```

Preferred Rev-A architecture:

- 5V USB-C input.
- 3.3V buck regulator for ESP32 and logic.
- ESP32-S3-WROOM-1-N8 module.
- 5V mini submersible pump or similar small DC pump.
- Low-side N-MOSFET pump driver.
- Flyback diode across pump.
- Capacitive soil moisture sensor through connector.
- 0.96 inch SSD1306 I2C OLED on a separate UI board or cable.
- EC11-style rotary encoder with push button.
- Red error LED.
- JST-XH connectors where practical.
- 0805 passives.
- Test pads and labeled connectors.

---

## Design philosophy

This should feel like a serious embedded/mechatronics product prototype, not a toy breadboard circuit.

Important priorities:

1. **Safety and robustness first**
   - Pump must not be able to run forever in normal firmware failure cases.
   - MOSFET gate needs a pulldown.
   - Pump needs flyback protection.
   - Inputs need to be 3.3V-safe for ESP32 pins.
   - Moisture readings should not be destroyed by motor noise.

2. **Serviceability**
   - Use connectors for pump, sensor, UI, reservoir sensor, and power where practical.
   - Label connectors clearly on silkscreen.
   - Include test pads for rails and key signals.
   - Make it possible to debug without guessing.

3. **Good learning workflow**
   - This is a novice KiCad project, so reminders and checklists matter.
   - Do not assume the schematic or PCB is correct because it “looks okay.”
   - Use ERC/DRC often.
   - Keep footprints, pinouts, connectors, and power rails explicit.

4. **SolidWorks integration**
   - PCB shape, connector placement, button/display locations, and mounting holes should eventually line up with the enclosure.
   - The expected workflow is:

```text
KiCad schematic + rough footprints
-> KiCad PCB rough placement
-> SolidWorks enclosure / fit check
-> KiCad PCB revision
-> SolidWorks revision
-> repeat
```

---

## Hard constraints / do-not-break rules

Codex must follow these rules unless the user explicitly says otherwise.

### KiCad file safety

- Do not blindly rewrite `.kicad_sch` or `.kicad_pcb` files.
- Before editing schematic or PCB files, explain the intended change.
- Prefer checklists, reports, Python scripts, and small controlled patches over huge direct edits.
- KiCad GUI remains the source of truth for visual PCB edits.
- Do not change footprints, net names, connector pinouts, power paths, board shape, or mounting constraints without explicit approval.
- Run ERC after schematic edits when possible.
- Run DRC after PCB edits when possible.
- Keep reports in `/reports`.
- Keep outputs in `/outputs` or `/fabrication`.

### Electronics safety

- ESP32 GPIOs are 3.3V logic. Do not feed 5V into ESP32 pins.
- Moisture sensor must use ADC1 pins when possible because ESP32 ADC2 conflicts with Wi-Fi on many ESP32 variants.
- Pump must be switched with a proper MOSFET driver, not directly from a GPIO.
- MOSFET gate must have a pulldown resistor.
- Pump must have a flyback diode or equivalent protection.
- Keep motor current path short and away from sensitive analog input traces.
- Add bulk capacitance near pump/5V input and local decoupling near ICs/modules.
- Use enough trace width for pump current.
- Include test points for 5V, 3.3V, GND, pump gate, moisture ADC, and I2C if space allows.

### Firmware behavior constraints that influence hardware

The hardware should support firmware safety features:

- Hysteresis around moisture threshold.
- Cooldown after watering.
- Maximum pump runtime or maximum mL per watering event.
- Pump lockout/fault if no water is detected.
- Optional flow pulse counting using interrupt-capable GPIO.
- Calibration stored in nonvolatile memory.
- User-visible fault state through OLED and red error LED.

---

## Preferred components / Rev-A defaults

These are the current preferred defaults, not final purchasing guarantees. Codex should use these as working assumptions unless the user provides updated part numbers.

| Function | Preferred part / style | Notes |
|---|---|---|
| MCU | ESP32-S3-WROOM-1-N8 | Module preferred over dev board for product-ish PCB. Native USB possible. |
| Main input | USB-C 5V | Use as power input. Rev-A can avoid USB-PD complexity. |
| 3.3V regulator | AP63203WU-7 buck | 5V to 3.3V logic rail. Check exact footprint and recommended layout. |
| Pump switch | AO3400A N-MOSFET | Low-side switching for small DC pump. Confirm current/thermal margin. |
| Flyback diode | SS34 or similar Schottky | Across pump/load path. Orientation matters. |
| Pump | Small 5V DC mini submersible pump | Exact pump may vary. Connector should allow replacement. |
| Soil sensor | Capacitive soil moisture sensor | Analog output. Must be 3.3V-safe before ESP32 ADC. |
| Display | 0.96 inch SSD1306 I2C OLED | Could be on UI daughterboard or cabled. |
| Encoder | EC11 rotary encoder w/ push | Main local UI control. Needs debouncing in firmware or hardware. |
| Error LED | Red LED | User-visible fault indicator. |
| Connectors | JST-XH preferred | Good for pump/sensors/power/internal wiring. |
| Passives | 0805 | Easier hand soldering than smaller packages. |
| Programming/debug | Native USB + UART header/test pads | Keep recovery/debug options. |

Potential optional components:

- Reservoir float switch connector.
- Flow sensor connector.
- Extra status LED.
- Buzzer footprint, only if useful.
- Expansion header, only if not bloating the board.
- Power switch footprint.

Avoid adding random features unless they directly support watering reliability, calibration, debugging, or enclosure usability.

---

## Board / schematic organization

The design may be one physical breakaway panel or separate PCBs. The preferred concept is a main control board plus optional breakaway UI/daughterboard sections if it reduces wiring or improves enclosure fit.

### Option A — Single main PCB only

Simpler KiCad project.

Pros:

- Less board-to-board wiring.
- Easier first PCB.
- Fewer connector mistakes.

Cons:

- OLED/button placement may be awkward.
- Enclosure front panel alignment may be harder.
- The board shape may become constrained by UI placement.

### Option B — Main board + UI daughterboard

Preferred if enclosure fit needs a front-facing display/control panel.

Main board contains:

- USB-C power input.
- 3.3V buck regulator.
- ESP32-S3 module.
- Pump MOSFET driver.
- Sensor connectors.
- Reservoir/flow connectors.
- Programming/debug headers or test pads.
- Main power/status LEDs.

UI board contains:

- OLED footprint/header.
- Rotary encoder.
- User button if separate from encoder push.
- Red error LED if front-facing.
- Board-to-board or cable connector back to main board.

Potential UI board connector signals:

```text
3V3
GND
SDA
SCL
ENC_A
ENC_B
ENC_SW
ERROR_LED or STATUS_LED
optional spare GPIO
```

### Option C — Main board + UI board + sensor/environment board

Only use if there is a real mechanical reason. More boards means more complexity, more connector pinouts, and more chances for mistakes.

---

## Suggested schematic sheets

A clean schematic structure matters. Suggested sheets:

1. `Power_Input_Regulation`
2. `MCU_ESP32`
3. `Pump_Driver`
4. `Sensors_Connectors`
5. `User_Interface`
6. `Debug_Testpoints`

If the project stays small, these can also be sections on one schematic page. The important part is readability and intentional net naming.

---

## Schematic block details

### 1. Power input and regulation

Purpose:

- Accept 5V input through USB-C.
- Generate stable 3.3V rail for ESP32 and logic.
- Provide enough bulk capacitance for pump startup transients.

Likely footprints/items:

- USB-C receptacle, USB2/power-only style if possible.
- CC resistors for USB-C sink behavior if using bare USB-C connector.
- Input fuse/polyfuse optional but useful.
- Reverse-polarity protection optional depending on input style.
- AP63203WU-7 buck regulator and required inductor/capacitors/resistors.
- 5V test pad.
- 3.3V test pad.
- Power LED optional.
- Bulk electrolytic/tantalum/polymer capacitor near 5V pump/input.
- 0.1 uF decoupling capacitors near logic IC/module pins as needed.

KiCad reminders:

- Use the exact regulator datasheet recommended footprint and layout.
- Switching regulator layout matters: keep input cap, diode/switching node/inductor feedback loop compact per datasheet.
- Keep noisy switch node copper small.
- Use a ground pour, but do not assume the pour fixes bad placement.
- Label rails clearly: `+5V`, `+3V3`, `GND`.

### 2. ESP32-S3 module

Purpose:

- Main controller.
- Reads moisture sensor.
- Controls pump MOSFET.
- Drives OLED over I2C.
- Reads encoder/buttons/sensors.
- Handles calibration and fault logic.

Likely footprints/items:

- ESP32-S3-WROOM-1-N8 module footprint.
- EN/reset circuit.
- Boot/IO0 button or programming support if needed.
- Native USB D+/D- routing if used.
- UART header/test pads as backup.
- Decoupling capacitors.
- Keepout under antenna.
- Mounting/placement with antenna at board edge where possible.

Important pin planning:

- Moisture sensor should use ADC1-capable pin.
- Flow sensor, if used, should use interrupt-capable GPIO.
- I2C needs SDA/SCL pins with pullups to 3.3V.
- Pump gate control GPIO should be safe at boot and should not accidentally turn pump on during reset.
- Encoder pins should be convenient for firmware interrupt/polling.
- Avoid strapping pins for outputs that may affect boot behavior unless intentionally designed.

Codex task:

- Help create a pin allocation table before routing.
- Check that selected pins do not conflict with bootstrapping, USB, flash, or ADC limitations.

### 3. Pump driver

Purpose:

- Switch the 5V pump safely using ESP32 logic.
- Protect against inductive kick.
- Avoid accidental pump activation at boot.

Likely footprints/items:

- Pump JST-XH connector: `PUMP+`, `PUMP-`.
- AO3400A N-MOSFET or equivalent logic-level N-MOSFET.
- Gate resistor, maybe ~100 ohm to 330 ohm.
- Gate pulldown, likely ~100k to GND.
- Flyback diode across pump, such as SS34.
- Optional TVS or snubber if noise is bad.
- Optional pump status LED, only if useful.
- Pump current path with suitable trace width.

Expected electrical topology:

```text
+5V -> Pump+ connector pin
Pump- connector pin -> MOSFET drain
MOSFET source -> GND
ESP32 GPIO -> gate resistor -> MOSFET gate
MOSFET gate -> pulldown -> GND
Flyback diode across pump, reverse-biased during normal operation
```

Safety reminders:

- The GPIO must not directly power the pump.
- The pump connector should be clearly labeled for polarity.
- The flyback diode orientation must be checked carefully.
- The MOSFET footprint pin order must be verified against the actual part and KiCad footprint.

### 4. Soil moisture sensor

Purpose:

- Read soil moisture through an analog voltage.
- Allow calibration in firmware.
- Keep analog signal reasonably clean.

Likely footprints/items:

- JST-XH or JST-PH connector for sensor.
- Pins likely: `3V3`, `GND`, `MOISTURE_ADC`.
- Optional RC low-pass filter on ADC input.
- Optional series resistor into ADC pin.
- Optional ESD protection if connector exits enclosure.
- Test pad for ADC signal.

Important constraints:

- Sensor output must not exceed 3.3V at ESP32 ADC.
- Use ADC1 pin where possible.
- Keep sensor trace away from pump trace and switching regulator noise.
- Firmware should average readings and use calibration, not raw magic numbers.

### 5. Reservoir sensor / float switch

Purpose:

- Detect low water or reservoir presence.
- Prevent pump from running dry.

Likely footprints/items:

- 2-pin JST connector.
- Pullup or pulldown resistor depending on switch wiring.
- Optional RC debounce.
- GPIO input.
- Silkscreen label showing expected switch behavior if known.

Firmware behavior:

- If reservoir empty or missing, block watering.
- Display fault message and turn on error LED.

### 6. Optional flow sensor

Purpose:

- Verify that water is actually moving.
- Count pulses to estimate amount of water dispensed.

Likely footprints/items:

- 3-pin connector: `5V or 3V3`, `GND`, `FLOW_PULSE`.
- Pullup to correct voltage.
- Level shifting if sensor output is 5V.
- Optional input protection.
- Interrupt-capable ESP32 GPIO.

Constraints:

- Do not connect a 5V pulse output directly to ESP32 GPIO.
- Flow counting should be interrupt-based in firmware.
- If no pulses arrive after pump starts, enter no-flow fault.

### 7. User interface

Purpose:

- Show moisture/status/faults/settings.
- Let user calibrate dry/wet values and adjust thresholds.

Likely footprints/items:

- 0.96 inch SSD1306 I2C OLED header or direct-solder footprint.
- EC11 rotary encoder.
- Encoder push button.
- Red error LED with resistor.
- Optional status LED.
- UI board connector if separate.

Signals:

```text
3V3
GND
SDA
SCL
ENC_A
ENC_B
ENC_SW
ERROR_LED
optional spare
```

Reminders:

- I2C pullups should go to 3.3V, not 5V.
- OLED module voltage compatibility must be checked.
- Encoder needs debouncing in firmware; small RC filters are optional.
- Error LED should be visible from outside the enclosure.

### 8. Debug/test points

Purpose:

- Make first bring-up less painful.
- Make it easier to diagnose board mistakes.

Recommended test points:

- `GND`
- `+5V`
- `+3V3`
- `EN`
- `BOOT/IO0` if relevant
- `TXD`
- `RXD`
- `USB_D+`
- `USB_D-`
- `PUMP_GATE`
- `MOISTURE_ADC`
- `SDA`
- `SCL`
- `FLOW_PULSE` if used

---

## Net naming conventions

Use clear net names. Examples:

Power:

```text
+5V
+3V3
GND
VBUS
```

Pump:

```text
PUMP_GATE
PUMP_LOW
PUMP_PLUS
```

Sensors:

```text
MOISTURE_ADC
RESERVOIR_SW
FLOW_PULSE
```

I2C:

```text
I2C_SDA
I2C_SCL
```

UI:

```text
ENC_A
ENC_B
ENC_SW
ERROR_LED
STATUS_LED
```

Do not use vague net names like `SIGNAL`, `SENSOR`, `OUT`, `GPIO1`, unless they are temporary and documented.

---

## Footprint checklist

Before routing, Codex should help the user verify this list.

### Power

- USB-C connector footprint matches actual part.
- CC resistors are included if using USB-C power input directly.
- Buck regulator footprint matches exact AP63203 package.
- Inductor footprint matches actual selected inductor.
- Input/output capacitors have correct package and voltage rating.
- Test points for 5V/3.3V/GND.

### MCU

- ESP32-S3-WROOM footprint exactly matches module variant.
- Antenna keepout is present and respected.
- EN/BOOT/programming circuit is correct.
- USB data pins, if used, are routed intentionally.
- UART backup header/test pads exist.

### Pump driver

- AO3400A footprint pinout checked against datasheet.
- SS34 diode footprint checked.
- Pump connector footprint matches actual JST part.
- Gate resistor/pulldown included.
- Pump current traces sized appropriately.

### Sensors

- Moisture sensor connector pinout documented.
- Reservoir switch connector documented.
- Flow sensor connector documented if included.
- ADC protection/filtering considered.
- 5V sensor outputs are level-shifted or divided if needed.

### UI

- OLED module footprint/header matches actual module pin order.
- Encoder footprint matches actual EC11 encoder variant.
- Button footprint matches actual part.
- LED polarity clear on silkscreen.
- UI connector pinout documented on both boards if separate.

### Mechanical

- Mounting holes sized and grounded or isolated intentionally.
- Connector access matches enclosure concept.
- USB-C port location matches enclosure access.
- OLED/encoder positions match front panel if using UI board.
- Board edge clearances are reasonable.

---

## KiCad novice reminders

Codex should remind the user of these at the right times, especially before committing to PCB layout.

### Schematic reminders

- Every symbol needs the correct footprint assigned before PCB layout.
- Power symbols create global nets; use them carefully.
- ERC warnings are not automatically false. Read them.
- Add PWR_FLAG only when the warning is about KiCad not understanding a rail is powered, not to hide real problems.
- Label connector pins clearly.
- Keep pin numbers and connector order consistent with real wiring.
- Make a pinout table before routing.
- Check OLED module pin order; cheap modules may differ.

### PCB reminders

- Set board outline on `Edge.Cuts`.
- Place mounting holes early.
- Place mechanical connectors early.
- Place the ESP32 antenna near board edge with keepout respected.
- Keep pump current loop short.
- Keep moisture ADC away from pump and switching regulator noise.
- Use a ground plane/pour.
- Refill zones before final DRC.
- Use thicker traces for pump/current paths.
- Use 45-degree routing unless there is a reason not to.
- Label connectors on silkscreen.
- Add version text like `Self-Watering Flower Pot V2 Rev A` on silkscreen.
- Export a 3D model and check in SolidWorks or KiCad 3D viewer.

### Manufacturing reminders

- Run DRC before ordering.
- Check drill files.
- Check Gerbers in a Gerber viewer.
- Verify board dimensions.
- Verify connector footprints with datasheets or printed 1:1 footprint check.
- Confirm minimum trace/space and hole sizes match the PCB manufacturer.
- Do not order before checking polarity marks and connector pinouts.

---

## Firmware behavior target

The PCB should support this kind of firmware flow:

```text
Boot
  -> initialize display, sensors, pump output off
  -> read calibration/settings from NVM
  -> show status

Main loop
  -> read moisture multiple times and average/filter
  -> compare against calibrated dry/wet thresholds
  -> check reservoir state
  -> if too dry and allowed by cooldown, water plant
  -> after watering, wait/cooldown and recheck
  -> handle errors visibly
```

Watering logic requirements:

- Use hysteresis so pump does not rapidly toggle.
- Use cooldown so watering is not repeated too quickly.
- Use maximum runtime or maximum mL limit.
- If flow sensor exists, count pulses using interrupt-based logic.
- If pump is on but no flow is detected after a short grace period, stop and show fault.
- Store calibration in nonvolatile memory.

UI target:

- Show current moisture value/percentage.
- Show dry/wet calibration state.
- Show pump status.
- Show reservoir/fault status.
- Allow calibration of dry and wet readings.
- Allow threshold adjustment.
- Allow manual pump test with safety timeout.

---

## Suggested project folder structure

```text
SelfWateringFlowerPotV2/
  SelfWateringFlowerPotV2.kicad_pro
  SelfWateringFlowerPotV2.kicad_sch
  SelfWateringFlowerPotV2.kicad_pcb
  CODEX_HANDOVER.md
  AGENTS.md
  /docs
    architecture.md
    pinout.md
    bringup-checklist.md
    bom-notes.md
  /reports
    erc.txt
    drc.txt
  /outputs
  /fabrication
    gerbers/
    drill/
    bom/
    pickplace/
  /mechanical
    board-export.step
    enclosure-notes.md
  /firmware
    README.md
```

If the KiCad project already has a different name, use that project name consistently.

---

## Recommended `AGENTS.md` content

Create or update an `AGENTS.md` file in the project root with this content:

```md
# KiCad Project Rules for Codex

This is a KiCad PCB project for the Self-Watering Flower Pot V2.

Rules:
- Do not blindly rewrite .kicad_sch or .kicad_pcb files.
- Before editing schematic or PCB files, explain the intended change first.
- Prefer using kicad-cli, Python scripts, reports, and small controlled edits.
- Run ERC after schematic changes when possible.
- Run DRC after PCB changes when possible.
- Put reports in /reports.
- Put generated manufacturing/export files in /fabrication or /outputs.
- Do not change footprints, net names, connector pinouts, power paths, board shape, mounting holes, or enclosure constraints unless explicitly asked.
- If a change is risky, create a script or patch instead of directly modifying the KiCad design file.
- KiCad GUI remains the source of truth for visual PCB edits.
- Treat CODEX_HANDOVER.md as the main project context file.
```

---

## First Codex prompt to use in the Codex app

Paste this into Codex while the KiCad project folder is open:

```text
Read CODEX_HANDOVER.md and AGENTS.md first.

This is a new KiCad project for the Self-Watering Flower Pot V2. Do not edit schematic or PCB files yet.

First tasks:
1. Identify the KiCad project files in this folder.
2. Check whether kicad-cli works.
3. Create /docs, /reports, /outputs, /fabrication, and /mechanical folders if they do not exist.
4. Create docs/pinout.md with a draft pin allocation table for ESP32-S3-WROOM-1-N8 based on the handover requirements.
5. Create docs/bringup-checklist.md with a first-power-on checklist.
6. Create docs/bom-notes.md with a draft BOM category list, not final part orders.
7. Do not modify .kicad_sch or .kicad_pcb yet.
8. Summarize what you created and what decisions still need user approval.
```

---

## Second Codex prompt: schematic planning

Use this after the first prompt succeeds:

```text
Using CODEX_HANDOVER.md, propose the schematic structure for this KiCad project.

Do not edit design files yet.

Give me:
1. Recommended schematic sheets or sections.
2. Exact connector list and pinout draft.
3. Power tree.
4. ESP32 pin allocation draft.
5. Required support components for each block.
6. ERC risks to watch for.
7. A checklist of what I should place manually in KiCad GUI first.

Keep the design realistic for Rev A and do not add feature creep.
```

---

## Third Codex prompt: after schematic parts are placed

Use this once the user has started placing parts in KiCad:

```text
Inspect the KiCad schematic. Do not edit it yet.

Check for:
1. Missing footprints.
2. Missing power symbols or unclear rails.
3. ESP32 pin conflicts or bad boot strap choices.
4. Missing pump gate pulldown, gate resistor, or flyback diode.
5. Sensor outputs that may not be 3.3V-safe.
6. Missing I2C pullups.
7. Missing test points.
8. Connector pinout ambiguity.
9. ERC issues.

Run ERC using kicad-cli if possible and save the report in /reports.
Then summarize fixes in priority order.
```

---

## Fourth Codex prompt: before PCB routing

Use this after schematic is mostly done and footprints assigned:

```text
Before I start PCB routing, review the schematic and footprint assignments.

Do not edit the PCB yet.

Create a placement strategy for:
1. USB-C input.
2. Buck regulator and power path.
3. ESP32 module and antenna keepout.
4. Pump connector and MOSFET loop.
5. Moisture sensor connector and ADC trace.
6. UI connector/OLED/encoder path.
7. Mounting holes and enclosure constraints.
8. Test points.

Also create a list of high-risk footprints I should print/check at 1:1 scale before ordering.
```

---

## Fifth Codex prompt: after PCB placement/routing

Use this after layout work:

```text
Inspect the PCB layout. Do not make direct edits yet.

Check for:
1. Board outline on Edge.Cuts.
2. Mounting holes.
3. ESP32 antenna keepout.
4. Pump current trace width and loop area.
5. Buck regulator layout risks.
6. Analog moisture trace noise risks.
7. Ground pour / zone fill issues.
8. Connector silkscreen labels.
9. Polarity markings.
10. Test point access.
11. DRC errors.

Run DRC using kicad-cli if possible and save the report in /reports.
Summarize manufacturing risks and exact fixes.
```

---

## Sixth Codex prompt: fabrication export

Use this only when schematic and PCB are reviewed:

```text
Prepare fabrication outputs for review.

Before exporting, run ERC and DRC and save reports.
If there are blocking errors, stop and summarize them instead of exporting.

If checks pass or only approved warnings remain:
1. Export Gerbers to /fabrication/gerbers.
2. Export drill files to /fabrication/drill.
3. Export BOM if possible to /fabrication/bom.
4. Export position file if useful to /fabrication/pickplace.
5. Export STEP model to /mechanical/board-export.step if possible.
6. Summarize exactly what files were created.
```

---

## Bring-up checklist draft

Use this after the PCB arrives, before connecting the pump.

### Before soldering

- Inspect board for visible fab defects.
- Confirm board revision text.
- Confirm USB-C footprint orientation.
- Confirm ESP32 module orientation.
- Confirm diode polarity markings.
- Confirm MOSFET orientation.
- Confirm connector labels and pin 1 markers.

### After partial soldering: power only

- Solder power input/regulator section first if possible.
- Do not install ESP32 module until 3.3V rail is verified, if assembly order allows.
- Apply 5V current-limited power.
- Check 5V rail.
- Check 3.3V rail.
- Check regulator temperature.
- Check for shorts between 5V/GND and 3.3V/GND.

### After MCU soldering

- Verify ESP32 power pins.
- Try programming or USB detection.
- Blink status/error LED.
- Confirm boot/reset behavior.

### Sensor/UI tests

- Test OLED I2C scan.
- Test encoder rotation and button.
- Read moisture ADC in air.
- Read moisture ADC with sensor in water/damp soil only if sensor supports it.
- Test reservoir switch input.
- Test flow sensor pulses if included.

### Pump test

- Test MOSFET gate output with pump disconnected.
- Connect dummy load or LED/resistor if useful.
- Connect pump with current-limited supply.
- Run very short pump pulse.
- Verify flyback diode orientation and MOSFET temperature.
- Verify pump cannot turn on during reset/boot.

---

## Open decisions to resolve

Codex should help track these decisions rather than assuming them.

1. Exact KiCad project name.
2. Exact USB-C connector part.
3. Exact pump part and current draw.
4. Exact moisture sensor module pin order and voltage behavior.
5. Whether flow sensor is included on Rev A or only connector-ready.
6. Whether reservoir float switch is included on Rev A.
7. Whether OLED is direct on main board, cabled, or on UI daughterboard.
8. Whether board is one PCB, breakaway panel, or separate boards.
9. Exact enclosure layout and mounting hole positions.
10. Exact PCB manufacturer constraints.
11. Whether native USB programming is enough or UART header is required.
12. Whether battery operation is completely out of scope for Rev A.

Current likely defaults:

- Use USB-C 5V, not battery.
- Include reservoir switch connector.
- Include flow sensor connector if not too much complexity, but design should still work without installing it.
- Use main board + possible UI daughterboard if enclosure fit benefits.
- Focus on a reliable Rev A, not maximum features.

---

## Rough pricing categories

This is not a final BOM. It is for planning.

| Category | Rough expected cost range | Notes |
|---|---:|---|
| PCB fabrication | $5–$30 | Depends on size, quantity, shipping, panels. |
| ESP32-S3 module | $3–$8 | Depends source and module variant. |
| Power parts | $3–$10 | USB-C, buck, inductor, caps, protection. |
| Pump driver parts | $1–$4 | MOSFET, diode, resistors, connector. |
| OLED | $2–$8 | Module style varies. |
| Encoder/buttons/LEDs | $1–$5 | Depends quality and quantity. |
| Sensors | $2–$15 | Moisture, float, optional flow. |
| Connectors/wires | $3–$15 | JST housings, crimps, premade leads. |
| Enclosure hardware | $2–$15 | Screws, inserts, gasket, printed parts. |
| Spare/debug parts | $5–$20 | Strongly recommended. |

Likely Rev-A total: about **$30–$100**, depending on parts already owned, shipping, and whether multiple boards are ordered.

---

## What Codex should produce as receipts

Good project artifacts:

- `docs/architecture.md`
- `docs/pinout.md`
- `docs/bringup-checklist.md`
- `docs/bom-notes.md`
- ERC and DRC reports.
- PCB screenshots or exported images.
- STEP model for enclosure fitting.
- Gerber/drill outputs when ready.
- A short engineering writeup explaining design choices and safety features.
- A test log after bring-up.

Avoid:

- Random generated files with unclear purpose.
- Large direct rewrites of KiCad files.
- Feature creep.
- Hiding ERC/DRC warnings without understanding them.

---

## Human design notes

The user is strong technically but new to KiCad, so Codex should act like a careful engineering assistant:

- Catch missing details early.
- Explain risks clearly.
- Use checklists.
- Keep changes controlled.
- Assume the user will make many final choices in KiCad GUI.
- Do not water down the project. The goal is a serious embedded/mechatronics prototype.

The ideal Codex behavior is not “finish the PCB for me.” The ideal behavior is:

```text
organize -> inspect -> warn -> generate checklists -> run checks -> explain errors -> help export -> document evidence
```

---

## Immediate next step

After this file is placed in the KiCad project root, create `AGENTS.md` using the recommended content above, then open the project folder in the Codex app and run the first Codex prompt.
