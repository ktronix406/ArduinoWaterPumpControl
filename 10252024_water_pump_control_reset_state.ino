#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Define relay pins
const int relay1 = 4;
const int relay2 = 5;

// Define pressure sensor pin
const int pressureSensor = A0;

// Pressure thresholds
float pressureThresholdOn = 40.0;                           // Example threshold to turn pump on (in PSI)
float pressureThresholdOff = 60.0;                          // Example threshold to turn pump off (in PSI)
const float lowPressureShutOff = 32.0;                      // Low pressure shut-off threshold (in PSI)
float originalPressureThresholdOff = pressureThresholdOff;  // Store the original pressure threshold

// Timer variables for the fail-safe mechanism in the RESET state
unsigned long resetPressureStartTime = 0;
bool resetLowPressureTimerRunning = false;                // Track if the fail-safe timer is running during RESET
const unsigned long resetLowPressureMaxDuration = 60000;  // 1 minute in milliseconds

// Time threshold for reducing max pressure (in milliseconds)
unsigned long maxRunTimeThreshold = 300000;  // Settable variable, 5 minutes (300,000 ms)

// EEPROM addresses
const int offsetAddress = 0;
const int stateAddress = sizeof(float);  // Store state after offset

// Initialize the LCD (address 0x27 for a 16x2 LCD)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Parameters for the moving average filter
const int numReadings = 20;   // Increased number of readings to average
float readings[numReadings];  // Array to store pressure readings
int readIndex = 0;            // Index of the current reading
float total = 0;              // Running total of readings
float averagePressure = 0;    // Calculated average pressure

// Timing variables
unsigned long previousMillis = 0;  // Stores the last time the pressure was read
const long interval = 500;         // Interval to read the pressure (500 milliseconds)
unsigned long pumpOnTime = 0;      // Tracks the time the pump is on

// System states
enum SystemState { STOP,
                   RUN,
                   RESET };
SystemState systemState = RUN;

// Cycle tracking
bool maxPressureReduced = false;
int cycleCount = 0;

// Function to read pressure in PSI
float readPressure() {
  int sensorValue = analogRead(pressureSensor);
  float voltage = sensorValue * (5.0 / 1023.0);
  float pressure = (voltage - 0.5) * (80.0 / (4.5 - 0.5));  // Linear conversion
  return pressure;
}

// Function to read the stored offset from EEPROM
float readOffset() {
  float offset;
  EEPROM.get(offsetAddress, offset);
  return offset;
}

// Function to store the offset in EEPROM
void storeOffset(float offset) {
  EEPROM.put(offsetAddress, offset);
}

// Function to read the stored system state from EEPROM
SystemState readState() {
  int state;
  EEPROM.get(stateAddress, state);
  return static_cast<SystemState>(state);
}

// Function to store the system state in EEPROM
void storeState(SystemState state) {
  EEPROM.put(stateAddress, static_cast<int>(state));
}

void setup() {
  // Initialize the relay pins as outputs
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  // Start with both relays off
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  // Initialize the LCD
  lcd.init();
  lcd.begin(16, 2);  // Set up the LCD's number of columns and rows
  lcd.backlight();

  // Initialize serial communication for debugging
  Serial.begin(9600);
  Serial.println("Enter 'set <actual pressure>' to set the pressure offset.");
  Serial.println("Enter 'reset' to reset the system.");

  // Initialize the array to 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  // Read the stored system state from EEPROM
  systemState = readState();
}

void loop() {
  unsigned long currentMillis = millis();

  // Read the pressure every 500 milliseconds
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float pressure = readPressure();
    float offset = readOffset();
    float adjustedPressure = pressure + offset;

    // Update the moving average
    total -= readings[readIndex];
    readings[readIndex] = adjustedPressure;
    total += readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    averagePressure = total / numReadings;

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.print(" PSI, Adjusted Pressure: ");
    Serial.print(adjustedPressure);
    Serial.print(" PSI, Average Pressure: ");
    Serial.print(averagePressure);
    Serial.println(" PSI");

    // Handle system states
    switch (systemState) {
      case STOP:
        // Ensure the pump is off
        digitalWrite(relay1, HIGH);
        digitalWrite(relay2, HIGH);
        lcd.setCursor(0, 1);
        lcd.print("Pump: OFF (STOP)");
        break;

      case RUN:
        // Track pump run time
        if (digitalRead(relay1) == LOW) {
          pumpOnTime += interval;  // Add interval time to pump on time
        } else {
          pumpOnTime = 0;  // Reset pump on time when pump is off
        }

        // If pump has been on for the set time, reduce max pressure
        if (pumpOnTime >= maxRunTimeThreshold && !maxPressureReduced) {
          pressureThresholdOff = 45.0;  // Reduce max pressure to 45 PSI
          maxPressureReduced = true;
          cycleCount = 0;  // Reset cycle count
          Serial.println("Max pressure reduced to 45 PSI");
          lcd.setCursor(0, 1);
          lcd.print("Pressure Reduced ");
        }

        // Control the relays based on the average pressure
        if (averagePressure < lowPressureShutOff) {
          // Turn the pump off (relays HIGH) due to low pressure
          digitalWrite(relay1, HIGH);
          digitalWrite(relay2, HIGH);
          lcd.setCursor(0, 1);
          lcd.print("Pump: OFF (Low) ");
          // Change state to STOP
          systemState = STOP;
          storeState(systemState);
        } else if (averagePressure < pressureThresholdOn) {
          // Turn the pump on (relays LOW)
          if (digitalRead(relay1) == HIGH) {
            // Increment cycle count when turning on from off state
            cycleCount++;
            if (cycleCount >= 2 && maxPressureReduced) {
              pressureThresholdOff = originalPressureThresholdOff;  // Restore original pressure
              maxPressureReduced = false;
              Serial.println("Max pressure restored to original");
              lcd.setCursor(0, 1);
              lcd.print("Pressure Restored");
            }
          }
          digitalWrite(relay1, LOW);
          digitalWrite(relay2, LOW);
          lcd.setCursor(0, 1);
          lcd.print("Pump: ON        ");
        } else if (averagePressure > pressureThresholdOff) {
          // Turn the pump off (relays HIGH)
          digitalWrite(relay1, HIGH);
          digitalWrite(relay2, HIGH);
          lcd.setCursor(0, 1);
          lcd.print("Pump: OFF       ");
        }
        break;

case RESET:
    // Turn on the pump and try to reach pressure above 30 PSI
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Pump: ON (RESET) ");
    Serial.println("Attempting to pressurize...");

    // Start the timer if pressure is below 30 PSI
    if (averagePressure < 30.0) {
        if (!resetLowPressureTimerRunning) {
            resetPressureStartTime = millis();  // Start the fail-safe timer
            resetLowPressureTimerRunning = true;
        } else if (millis() - resetPressureStartTime > resetLowPressureMaxDuration) {
            // If pressure is below 30 PSI for more than 1 minute, stop the pump
            digitalWrite(relay1, HIGH);
            digitalWrite(relay2, HIGH);
            lcd.setCursor(0, 1);
            lcd.print("Pump: OFF (Fail) ");
            Serial.println("Fail-safe triggered during RESET: Pressure couldn't maintain above 30 PSI. System stopped.");
            // Change state to STOP
            systemState = STOP;
            storeState(systemState);
            resetLowPressureTimerRunning = false;  // Reset the timer state
        }
    } else if (averagePressure >= 30.0 && averagePressure < 40.0) {
        // Continue pressurizing to reach 40 PSI
        digitalWrite(relay1, LOW);
        digitalWrite(relay2, LOW);
        lcd.setCursor(0, 1);
        lcd.print("Pump: ON (30-40 PSI) ");
        Serial.println("Pressure reached 30 PSI, continuing to 40 PSI...");
    } else if (averagePressure >= 40.0) {
        // Reset complete, switch to RUN state
        resetLowPressureTimerRunning = false;
        systemState = RUN;
        storeState(systemState);
        Serial.println("Pressure stabilized at 40 PSI, moving to RUN state.");
        lcd.setCursor(0, 1);
        lcd.print("System: RUN     ");
    }
    break;

    }

    // Display the average pressure on the LCD
    lcd.setCursor(0, 0);
    lcd.print("Pressure: ");
    lcd.print(averagePressure);
    lcd.print(" PSI   ");
  }

  // Check for serial input
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("set ")) {
      float actualPressure = input.substring(4).toFloat();
      float pressure = readPressure();
      float newOffset = actualPressure - pressure;
      storeOffset(newOffset);
      Serial.print("Offset set to: ");
      Serial.println(newOffset);
      lcd.setCursor(0, 1);
      lcd.print("Offset Set      ");
      delay(2000);  // Display "Offset Set" message for 2 seconds
    } else if (input.equals("reset")) {
      systemState = RESET;
      storeState(systemState);
      Serial.println("System reset. Entering RESET state.");
      lcd.setCursor(0, 1);
      lcd.print("System Reset    ");
      delay(2000);  // Display "System Reset" message for 2 seconds
    }
  }

  delay(100);  // Wait for 100ms before the next loop iteration
}
