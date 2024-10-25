
# Whole Home Water System Control

An Arduino-based control system designed to automate and protect a whole-home water pump. This project includes pressure monitoring, automatic state management, and fail-safe protections, ensuring efficient and reliable pump operation for residential water systems. The system uses an LCD for real-time status display and EEPROM to maintain calibration and system state after power cycles.

## Features

- **Automatic Pump Control**: Monitors pressure and toggles the pump based on pre-set thresholds.
- **State Management**: Operates in `RUN`, `RESET`, or `STOP` states for efficient operation.
- **Fail-Safe Mechanisms**: Automatically shuts off if pressure remains critically low or if the pump runs too long.
- **Real-Time Display**: Shows live system status, pressure readings, and warnings on a 16x2 LCD.
- **EEPROM Persistence**: Saves calibration offsets and system state to EEPROM, preserving settings across restarts.

## System States

1. **STOP**: Ensures the pump is off if pressure falls below the critical threshold.
2. **RUN**: Normal operating state; monitors pressure to control the pump.
3. **RESET**: Tries to increase pressure to a stable level. If pressure reaches 30 PSI, the pump continues until reaching 40 PSI, then transitions to `RUN`.

## Hardware Requirements

- **Arduino** (compatible with analog inputs and EEPROM)
- **Pressure Sensor** (analog, 0-5V)
- **Relays** (for pump control)
- **16x2 LCD Display** (I2C compatible)
- **EEPROM** (onboard with Arduino)

## Wiring

1. **Relays**: Connected to Arduino digital pins to control the pump.
2. **Pressure Sensor**: Connected to analog input pin `A0`.
3. **LCD**: Connected to I2C pins (`SDA` and `SCL`).

## Code Overview

1. **Pressure Monitoring**: Reads sensor data, calculates PSI, and applies a moving average to stabilize readings.
2. **EEPROM Storage**: Stores calibration offsets and system state, enabling recovery after power cycles.
3. **Loop Logic**:
   - Reads pressure every 500ms.
   - Updates average pressure and manages state transitions.
   - Controls the pump based on `STOP`, `RUN`, and `RESET` logic.

## Setup and Calibration

1. Connect the components as specified in the wiring diagram.
2. Power on the Arduino and observe the LCD display.
3. Use the Serial Monitor to set calibration offsets:
   - Type `set <actual pressure>` to calibrate the sensor.
   - Type `reset` to reset the system to `RUN` state.

## Serial Commands

- **`set <actual pressure>`**: Adjusts pressure reading for accurate calibration.
- **`reset`**: Resets the system, attempting to pressurize and re-enter `RUN`.

## Example Output

```
Pressure: 30 PSI, Adjusted Pressure: 32 PSI, Average Pressure: 31 PSI
System reset. Entering RESET state.
Pressure reached 30 PSI, continuing to 40 PSI...
Pressure stabilized at 40 PSI, moving to RUN state.
```

## License

This project is licensed under the MIT License.
