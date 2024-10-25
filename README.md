
# Whole Home Water System Control

This Arduino-based project automates and safeguards a whole-home water pump, featuring pressure monitoring, automatic state management, and fail-safe protections. The system displays live status updates on an LCD and stores configuration data in EEPROM to maintain settings across power cycles.

## Features

- **Automatic Pump Control**: Starts and stops the pump based on configurable pressure thresholds.
- **System States**: Manages pump behavior across three states—`RUN`, `RESET`, and `STOP`.
- **Fail-Safe Mechanisms**: Shuts off if pressure is too low or if the pump runs continuously for a set time.
- **LCD Display**: Real-time status, pressure, and error messages on a 16x2 I2C LCD.
- **EEPROM Persistence**: Saves calibration offsets and system state for continuity after restarts.

## System States

1. **STOP**: Shuts off the pump if pressure drops below the safe limit (`lowPressureShutOff`).
2. **RUN**: Normal operation—monitors pressure and controls the pump based on `pressureThresholdOn` and `pressureThresholdOff`.
3. **RESET**: Attempts to stabilize the system by pressurizing to at least 40 PSI. The pump transitions back to `RUN` if it reaches 40 PSI.

## Hardware Requirements

- **Arduino** (with analog input and EEPROM)
- **Pressure Sensor** (analog, 0-5V)
- **Relays** (for controlling the pump)
- **16x2 LCD Display** (I2C compatible)

## Wiring

1. **Relays**: Connect relays to Arduino digital pins for pump control.
2. **Pressure Sensor**: Connect to the analog input pin `A0`.
3. **LCD**: Connect to I2C pins (`SDA` and `SCL`).

## Configuration Parameters

The following parameters are customizable within the code to suit specific system requirements:

### 1. Pressure Thresholds
- **`pressureThresholdOn`**: Pressure to turn the pump on. Default: `40.0` PSI.
- **`pressureThresholdOff`**: Pressure to turn the pump off during normal operation. Default: `60.0` PSI.
- **`lowPressureShutOff`**: Critical threshold to stop the pump if pressure falls below this level. Default: `32.0` PSI.
- **Example Modification**:
  ```cpp
  float pressureThresholdOn = 35.0;       // Turns pump on at 35 PSI
  float pressureThresholdOff = 55.0;      // Turns pump off at 55 PSI
  const float lowPressureShutOff = 30.0;  // Stops pump if below 30 PSI
  ```

### 2. Continuous Runtime Limit
- **`maxRunTimeThreshold`**: Maximum time (in milliseconds) the pump can run continuously before reducing `pressureThresholdOff` to avoid overheating.
- **Default**: 5 minutes (300,000 milliseconds). For a 3-minute runtime, adjust as shown below:
  ```cpp
  unsigned long maxRunTimeThreshold = 180000;  // 3 minutes in milliseconds
  ```

### 3. Reduced Pressure Threshold After Long Runtime
- After the pump exceeds `maxRunTimeThreshold`, `pressureThresholdOff` is reduced (default is `45.0` PSI) to prevent excessive pressure.
- **Example**:
  ```cpp
  float pressureThresholdOff = 45.0;  // Reduced pressure threshold after long runtime
  ```

### 4. Fail-Safe Duration in `RESET` State
- **`resetLowPressureMaxDuration`**: Maximum time (in milliseconds) allowed for the pump to reach at least 30 PSI during `RESET`. If this limit is exceeded, the pump stops.
- **Default**: 1 minute (60,000 milliseconds). Modify as needed:
  ```cpp
  const unsigned long resetLowPressureMaxDuration = 60000;  // 1 minute
  ```

### 5. Moving Average Filter
- **`numReadings`**: Number of readings for averaging pressure to stabilize sensor data.
- **Default**: 20 readings. Adjust to improve stability or responsiveness:
  ```cpp
  const int numReadings = 20;  // Increase for more stability, decrease for faster response
  ```

## Code Overview

### Pressure Monitoring
The `readPressure()` function reads the pressure sensor data, converts it to PSI, and applies a moving average filter to reduce noise.

### EEPROM Storage
- **`readOffset()` and `storeOffset()`**: Manage calibration offset stored in EEPROM.
- **`readState()` and `storeState()`**: Save and retrieve the system state from EEPROM for continuity.

### Main Loop Logic
1. Reads and calculates average pressure every 500 milliseconds.
2. Updates the system state (`RUN`, `RESET`, or `STOP`) based on conditions.
3. Controls the pump and displays information on the LCD.

## Using the System

1. **Serial Commands**:
   - **Set Calibration Offset**:
     - Enter `set <actual pressure>` in the Serial Monitor to set a pressure calibration offset.
   - **Reset System**:
     - Enter `reset` in the Serial Monitor to reset the system to the `RUN` state.

2. **LCD Display Messages**:
   - **Pump ON/OFF**: Displays the current state of the pump.
   - **Error and Warning Messages**: Shows “Low Pressure” or “Pressure Reduced” messages when applicable.

## Example Serial Output

```plaintext
Pressure: 30 PSI, Adjusted Pressure: 32 PSI, Average Pressure: 31 PSI
System reset. Entering RESET state.
Pressure reached 30 PSI, continuing to 40 PSI...
Pressure stabilized at 40 PSI, moving to RUN state.
```

## License

This project is licensed under the MIT License.
