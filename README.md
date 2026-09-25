# Polyurethane Internal Measurement Probe
 
A battery-powered, Arduino-based temperature probe that fits within the spokes of a buggy wheel to measure heat loss data, built to quantify how well fairings and fairing liner materials (mylar, heat tape) retain wheel heat during a roll.
 
## Context
 
Many buggy teams heat their wheels for better grip and speed on the hills, using various methods (floating, shielding) to retain that heat before the start. Fringe buggies are unique in that most of the fleet uses fairings to shield the wheels, which is presumed to help with both aerodynamics and heat retention. This project set out to quantify that benefit and determine whether lining the fairings with mylar or heat tape provides a meaningful improvement.
 
## The Problem
 
Initial testing was done on a stationary buggy: lifting it up, installing the heated wheels, spinning them manually, and measuring temperature at intervals with an infrared thermometer. This approach had several issues:
 
- No continuous stream of temperature data
- No way to test heated wheels on a buggy actually rolling
- Required removing the fairing covers to get a reading
- The IR thermometer sometimes picked up heat signals from the rim or other components, muddying results
What was actually needed: a device that continuously measures wheel temperature while storing the results, fits inside the fairings without interfering with the wheel, brakes, or steering, and can survive the wheels coming straight out of the heater at up to 170°F.
 
## Design
 
**Constraints:** the fairings leave almost no clearance. The only viable free space is between the spokes of the rim — roughly 1.5-2 cubic inches. Because the wheel spins, batteries, sensors, and the microprocessor all have to live in that same space; there's no routing power or data elsewhere without a slip ring or modifying the shell.
 
**Microprocessor:** Arduino Pro Mini 3.3V — small enough to fit, and the 3.3V version means the device can run on common coin cells without needing the extra clock speed of the 5V version.
 
**Power:** Two CR2032 coin cells in series (~6V) feeding the Arduino's voltage regulator. A single CR2032 was tried first (it would've fit in one gap between spokes), but in practice it only supplies ~3.0-3.1V, which isn't enough for the regulator, and skipping the regulator risks brownouts as the cell heats up and its voltage sags — plus the flash module's peak current draw exceeds what one cell can provide. A ceramic capacitor smooths power, and 10 µF electrolytic capacitors help with voltage dips during flash writes. Only lithium primary cells are used — LiPo batteries would combust at these operating temperatures, while lithium primary cells are rated past 200°F, making them safer.
 
**Storage:** No good way to transmit readings live off a moving buggy, so data is stored on-device and pulled off afterward. Arduino RAM and EEPROM are both too small/limited for this (EEPROM tops out around 40 datapoints and has a limited write life), and a micro-SD rated for these temperatures would be costly. Settled on a 32MB SPI flash module — datapoints buffer in RAM and flush to flash periodically, then get pulled off over serial USB.
 
**Measurement:** A 10kΩ thermistor paired with a 10kΩ resistor in a voltage divider. The Arduino reads the analog voltage and runs the Steinhart-Hart equation to get a Fahrenheit reading. The probe can be taped to the side of the wheel with reflective heat tape, or seated in a small drilled hole in the polyurethane.
 
**Case:** A 3D-modeled case sized to fit snugly between the spokes. Common filaments like PLA and PETG have glass transition temperatures too close to (or below) 170°F to be viable, so the case is printed in Polymaker PA6-CF (carbon-fiber-reinforced nylon) for its much higher heat deflection temperature. Zip-tie holes secure it against vibration at buggy speeds up to 40mph with no suspension damping. Electronics are potted in a 2-part silicone compound (McMaster-Carr 74965A52) chosen for its gel-like consistency, which wicks under the heat shrink to fully envelope the solder joints.
[Case and lid](images/case.jpg)*Case and its lid*

## How It Works
 
Each datapoint is a temperature/timestamp pair — an unsigned 8-bit integer (0-255°F) and an unsigned 16-bit timestamp (0-65536), well within the expected temperature range of the wheels.
 
The probe is controlled by a single button:
- **Short press** → starts a test: reads the thermistor once per second, converts to Fahrenheit via Steinhart-Hart, and buffers the results, flushing to flash as the buffer fills or the test ends.
- **Long press (4+ seconds)** → dumps all stored data over UART serial to a connected PC.
## Testing
 
Control tests were run by mounting the wheel on an open-air frame and spinning it with a belt sander to simulate the road. The same setup was then used with the wheel mounted to the buggy, to isolate the effect of the fairing, fairing cover, and heat tape at matching speeds.

[Results graph](images/results2.png)*Temperature Graph*
 
## Usage
 
1. Seat the battery case and probe case between the wheel's spokes and snap the lids on to secure them.
2. Connect the battery pack to the probe.
   - **⚠️ Black (battery, negative) → black (probe). Yellow (battery) → orange (probe).** Wiring this wrong will permanently kill the probe.
3. Press the probe's button (**do not hold longer than 4 seconds**) to start recording.
4. When the run is done, connect the probe to the USB serial device:
   - Probe red → USB blue, probe blue → USB green, probe yellow → USB red, probe black (ground) → USB orange, probe orange (raw power) → USB yellow.
   - **⚠️ Incorrect ground/power connection here will also permanently kill the probe.**
   - These wire colors are specific to the serial connector left with the device.
5. Open a serial connection at 9600 baud (e.g. PuTTY) on the device's COM port.
   - If gibberish is showing up on the TTL, try halving the baud rate to 4800.
6. Hold the probe's button for 4+ seconds to dump the stored measurements to the serial connection.
## Future Improvements
 
- **Calibration:** the Steinhart-Hart constants currently used are generic for the 10kΩ/10kΩ pair rather than calibrated for this specific setup. Not critical for measuring relative thermal decay, but would improve absolute accuracy.
- **Case fit:** the current case clips on and relies on protruding tabs to stay seated in the spokes. A case that molds more closely to the spoke shape would reduce rattling and give a more secure, less rim-specific fit — though a universal design will always be a tradeoff against rim-to-rim variation.
 