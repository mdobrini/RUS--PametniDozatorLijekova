/**
 * @file sketch.ino
 * @brief Smart Medicine Scheduler and Dispenser
 * @details An Arduino-based system that schedules and dispenses medication
 * from a vertical stack using two servo motors. The system allows setting
 * up to 5 different medication times and displays information on a 16x2 LCD.
 * 
 * Features:
 * - Up to 5 scheduled medications
 * - 16x2 I2C LCD display interface
 * - 4x4 keypad input system
 * - DS1307 RTC for accurate timekeeping
 * - 2 servo motors for vertical dispenser mechanism
 * - Sleep mode for power efficiency
 * - EEPROM storage for medication schedules
 * - Buzzer for audio alerts
 * - Wake button for manual awakening
 * 
 * @author Mislav Dobrinić
 * @date 2025-04-14
 * @version 1.6
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

/** @brief Maximum number of medications that can be scheduled */
#define MAX_MEDICATIONS 5

/** @brief LCD Display Configuration (I2C) */
LiquidCrystal_I2C lcd(0x27, 16, 2); // address, columns, rows

/** @brief Real Time Clock Configuration */
RTC_DS1307 rtc;

/** @brief Servo Configuration */
/** @brief Top servo controls entry of medicine container into holding area */
Servo topServo;
/** @brief Bottom servo controls exit of medicine container from holding area */
Servo bottomServo;
/** @brief Pin for top servo */
#define TOP_SERVO_PIN 9
/** @brief Pin for bottom servo */
#define BOTTOM_SERVO_PIN 10
/** @brief Servo position for closed gate (no container passage) */
#define SERVO_CLOSED_POS 0
/** @brief Servo position for open gate (container passage allowed) */
#define SERVO_OPEN_POS 90
/** @brief Delay between servo operations in milliseconds */
#define DISPENSE_DELAY 1000

/** @brief Buzzer Configuration */
/** @brief Pin for buzzer */
#define BUZZER_PIN 11
/** @brief Buzzer tone frequency in Hz */
#define BUZZER_FREQ 2000
/** @brief Buzzer sound duration in ms */
#define BUZZER_DURATION 200
/** @brief Number of beeps to sound when dispensing */
#define BUZZER_DISPENSE_BEEPS 3

/** @brief LED Indicator Configuration */
/** @brief Pin for wake indicator LED */
#define WAKE_LED_PIN 12
/** @brief Blink duration in milliseconds */
#define LED_BLINK_DURATION 200
/** @brief Number of blinks for button wake */
#define BUTTON_WAKE_BLINKS 2
/** @brief Number of blinks for watchdog wake */
#define WATCHDOG_WAKE_BLINKS 1

/** @brief Wake Button Configuration */
/** @brief Pin for dedicated wake button */
#define WAKE_BUTTON_PIN 2  // Using INT0 (pin 2) for external interrupt

/** @brief Keypad Configuration */
/** @brief Number of rows in keypad matrix */
const byte ROWS = 4;
/** @brief Number of columns in keypad matrix */
const byte COLS = 4;
/** @brief Key mapping for 4x4 matrix */
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
/** @brief Arduino pins connected to keypad rows */
byte rowPins[ROWS] = {5, 4, 3, 7};  // Changed from pin 2 to pin 7 to avoid conflict with INT0
/** @brief Arduino pins connected to keypad columns */
byte colPins[COLS] = {A0, A1, A2, A3};
/** @brief Keypad object for handling input */
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/** @brief Sleep Mode Configuration */
/** @brief Time in milliseconds of inactivity before entering sleep mode */
#define SLEEP_TIMEOUT 10000  // 10 seconds for testing
/** @brief Flag to indicate wake-up has occurred by button press */
volatile bool buttonWakeFlag = false;
/** @brief Flag to indicate wake-up has occurred by watchdog timer */
volatile bool watchdogWakeFlag = false;
/** @brief Flag to track if the system is in sleep mode */
volatile bool inSleepMode = false;
/** @brief Display mode (0 = setup instructions, 1 = time to next medication) */
volatile byte displayMode = 0;
/** @brief Number of available display modes */
#define DISPLAY_MODES 2
/** @brief Button debounce time in milliseconds */
#define DEBOUNCE_TIME 200
/** @brief Last time the button was pressed */
volatile unsigned long lastButtonTime = 0;

/** @brief Medication Schedule Structure */
struct Medication {
  /** @brief Whether this medication is active in the schedule */
  bool active;
  /** @brief Hour to dispense (0-23) */
  byte hour;
  /** @brief Minute to dispense (0-59) */
  byte minute;
  /** @brief Whether this medication has been dispensed today */
  bool dispensedToday;
};

/** @brief Array of medication schedules */
Medication medications[MAX_MEDICATIONS];

/** @brief Timestamp of last user action for sleep mode determination */
unsigned long lastActionTime = 0;
/** @brief Timestamp of last schedule check */
unsigned long lastCheckTime = 0;
/** @brief Interval between schedule checks in milliseconds */
#define CHECK_INTERVAL 1000
/** @brief Flag indicating whether the system is in setup mode */
bool inSetupMode = false;

// Function prototypes
void setupMedications();
void saveMedications();
void loadMedications();
void displayTime(bool forceUpdate = false);
void displayMenu();
void setMedicationTime(int index);
void checkSchedule();
void dispense();
void enterSleepMode();
void setupWatchdog();
bool checkMedicationDue();
void blinkLED(int count);
bool getTimeToNextMedication(DateTime now, int &hoursToNext, int &minutesToNext);
bool sleep_is_active();

/**
 * @brief Interrupt Service Routine for external interrupt (button)
 */
ISR(INT0_vect) {
  // Get current time for debouncing
  unsigned long currentTime = millis();
  
  // Check if enough time has passed since the last button press
  if (currentTime - lastButtonTime >= DEBOUNCE_TIME) {
    // Check if we're already awake
    if (inSleepMode) {
      // We're in sleep mode, so just set the wake flag
      buttonWakeFlag = true;
    } else {
      // We're not in sleep mode, so toggle the display mode
      displayMode = (displayMode + 1) % DISPLAY_MODES;
      // Also reset activity timer
      lastActionTime = currentTime;
    }
    
    // Update the last button press time
    lastButtonTime = currentTime;
  }
}

/**
 * @brief Interrupt Service Routine for Watchdog Timer
 */
ISR(WDT_vect) {
  watchdogWakeFlag = true;
}

/**
 * @brief Setup function runs once at startup
 * 
 * Initializes all components, displays welcome message,
 * and loads medication schedules from EEPROM
 */
void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Scheduler");
  delay(2000);
  
  // Initialize RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC not found");
    while (1);
  }
  
  // Uncomment the following line to set the RTC to the date & time on PC this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // Initialize Servos
  topServo.attach(TOP_SERVO_PIN);
  bottomServo.attach(BOTTOM_SERVO_PIN);
  topServo.write(SERVO_CLOSED_POS);
  bottomServo.write(SERVO_CLOSED_POS);
  
  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize Wake LED
  pinMode(WAKE_LED_PIN, OUTPUT);
  digitalWrite(WAKE_LED_PIN, LOW);
  
  // Initialize Wake Button (active LOW with internal pullup)
  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
  
  // Configure external interrupt (INT0)
  EICRA |= (1 << ISC01); // Trigger on falling edge
  EIMSK |= (1 << INT0);  // Enable INT0 interrupt
  
  // Enable global interrupts
  sei();
  
  // Test buzzer with a single beep
  beep(1);
  
  // Test LED with a blink
  blinkLED(1);
  
  // Load medications from EEPROM
  loadMedications();
  
  delay(1000);
  lcd.clear();
  
  // Set initial timestamps
  lastActionTime = millis();
  lastCheckTime = millis();
}

/**
 * @brief Main program loop
 * 
 * Continuously checks for user input, updates display,
 * checks medication schedules, and handles sleep mode
 */
void loop() {
  // Check if we just woke up from button press
  if (buttonWakeFlag) {
    noInterrupts();
    lastActionTime = millis();
    // Reset the flag
    Serial.print("--Woken up manually!--");
    buttonWakeFlag = false;
    interrupts();
    
    // Blink the LED to indicate button wake-up
    blinkLED(BUTTON_WAKE_BLINKS);
    
    // Make sure display is on
    lcd.backlight();
    lcd.display();
    
    // Reset timers to prevent immediate sleep
    lastActionTime = millis();
    lastCheckTime = millis();
    
    // Display wakeup feedback
    lcd.clear();
    lcd.print("System active");
    delay(1000);
    lcd.clear();
    displayTime(true); // Force display update
  }
  
  // Check if we just woke up from watchdog timer
  if (watchdogWakeFlag && !buttonWakeFlag) {
    // Reset the flag
    watchdogWakeFlag = false;
    
    // Blink the LED to indicate watchdog wake-up
    blinkLED(WATCHDOG_WAKE_BLINKS);
    
    // Check if it's time for medication
    if (checkMedicationDue()) {
      // If medication is due, turn on the display
      lcd.backlight();
      lcd.display();
      
      // Reset timers
      lastActionTime = millis();
      lastCheckTime = millis();
    } else {
      // If no medication is due, go back to sleep immediately
      enterSleepMode();
      return; // Skip the rest of the loop
    }
  }
  
  // Check for keypad input
  char key = keypad.getKey();
  
  if (key) {
    // Reset the timer on key press
    lastActionTime = millis();
    
    // Handle key input
    if (key == 'A') {
      // Enter setup mode
      inSetupMode = true;
      setupMedications();
      inSetupMode = false;
    } else if (key == 'B') {
      // Display all scheduled medications
      displayAllMedications();
    } else if (key == 'C') {
      // Manual dispense test
      lcd.clear();
      lcd.print("Test dispenser");
      lcd.setCursor(0, 1);
      lcd.print("1=Yes D=Cancel");
      
      while (true) {
        char testKey = keypad.getKey();
        lastActionTime = millis();
        if (testKey == '1') {
          dispense();
          break;
        } else if (testKey == 'D') {
          break;
        }
      }
    } else if (key == 'D') {
      // Return to time display
      lcd.clear();
    }
    displayTime(true); // Force display update
  }
  
  // Check if it's time to check the schedule
  unsigned long currentMillis = millis();
  if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
    lastCheckTime = currentMillis;
    checkSchedule();
  }
  
  // Display current time when not in setup mode
  if (!inSetupMode) {
    displayTime(); // Normal display (checks minute change)
  }
  
  // Check if it's time to sleep
  if (millis() - lastActionTime > SLEEP_TIMEOUT && !inSetupMode) {
    lcd.clear();
    lcd.print("Entering sleep");
    lcd.setCursor(0, 1);
    lcd.print("Press wake btn");
    delay(1500);
    
    // Enter sleep mode
    enterSleepMode();
  }
}

/**
 * @brief Produce a series of beeps using the buzzer
 * 
 * @param count Number of beeps to produce
 */
void beep(int count) {
  for (int i = 0; i < count; i++) {
    tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_DURATION);
    delay(BUZZER_DURATION);
    noTone(BUZZER_PIN);
    if (i < count - 1) {
      delay(BUZZER_DURATION); // Pause between beeps
    }
  }
}

/**
 * @brief Setup medication schedules
 * 
 * Displays menu for selecting which medication to set up
 * and handles the process of setting each medication
 */
void setupMedications() {
  lcd.clear();
  lcd.print("Setup Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Pick 1-5 D=exit");
  
  char selection = '\0';
  while (true) {
    selection = keypad.getKey();
    lastActionTime = millis();
    if (selection >= '1' && selection <= '5') {
      break;
    } else if (selection == 'D') {
      return; // Exit setup
    }
  }
  
  int index = selection - '1';
  setMedicationTime(index);
  
  // Save to EEPROM
  saveMedications();
}

/**
 * @brief Set the time for a specific medication
 * 
 * @param index The index of the medication to set (0-4)
 */
void setMedicationTime(int index) {
  lcd.clear();
  lcd.print("Set Medicine ");
  lcd.print(index + 1);
  
  // First ask for active status
  lcd.setCursor(0, 1);
  lcd.print("Active? 1=Y 0=N");
  
  char activeKey = '\0';
  while (true) {
    activeKey = keypad.getKey();
    lastActionTime = millis();
    if (activeKey == '1' || activeKey == '0') {
      break;
    } else if (activeKey == 'D') {
      return; // Cancel
    }
  }
  
  medications[index].active = (activeKey == '1');
  
  if (!medications[index].active) {
    lcd.clear();
    lcd.print("Medicine ");
    lcd.print(index + 1);
    lcd.setCursor(0, 1);
    lcd.print("Deactivated");
    delay(1500);
    return;
  }
  
  // Set hour
  lcd.clear();
  lcd.print("Set Hour (0-23):");
  lcd.setCursor(0, 1);
  
  byte hour = 0;
  bool hourSet = false;
  
  while (!hourSet) {
    char hourKey = keypad.getKey();
    lastActionTime = millis();
    
    if (hourKey >= '0' && hourKey <= '9') {
      byte digit = hourKey - '0';
      
      if (hour == 0) {
        hour = digit;
      } else {
        hour = hour * 10 + digit;
      }
      
      if (hour > 23) hour = 23;
      
      lcd.clear();
      lcd.print("Set Hour (0-23):");
      lcd.setCursor(0, 1);
      lcd.print(hour);
    } else if (hourKey == '#') {
      hourSet = true;
    } else if (hourKey == '*') {
      hour = 0;
      lcd.setCursor(0, 1);
      lcd.print("    "); // Clear the displayed hour
      lcd.setCursor(0, 1);
    } else if (hourKey == 'D') {
      return; // Cancel
    }
  }
  
  medications[index].hour = hour;
  
  // Set minute
  lcd.clear();
  lcd.print("Set Minute (0-59):");
  lcd.setCursor(0, 1);
  
  byte minute = 0;
  bool minuteSet = false;
  
  while (!minuteSet) {
    char minuteKey = keypad.getKey();
    lastActionTime = millis();
    
    if (minuteKey >= '0' && minuteKey <= '9') {
      byte digit = minuteKey - '0';
      
      if (minute == 0) {
        minute = digit;
      } else {
        minute = minute * 10 + digit;
      }
      
      if (minute > 59) minute = 59;
      
      lcd.clear();
      lcd.print("Set Minute (0-59):");
      lcd.setCursor(0, 1);
      lcd.print(minute);
    } else if (minuteKey == '#') {
      minuteSet = true;
    } else if (minuteKey == '*') {
      minute = 0;
      lcd.setCursor(0, 1);
      lcd.print("    "); // Clear the displayed minute
      lcd.setCursor(0, 1);
    } else if (minuteKey == 'D') {
      return; // Cancel
    }
  }
  
  medications[index].minute = minute;
  medications[index].dispensedToday = false;
  
  lcd.clear();
  lcd.print("Medicine ");
  lcd.print(index + 1);
  lcd.setCursor(0, 1);
  lcd.print("Time Set!");
  delay(1500);
}

/**
 * @brief Display all active medication schedules
 * 
 * Loops through all scheduled medications and displays their times
 */
void displayAllMedications() {
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    if (medications[i].active) {
      lcd.clear();
      lcd.print("Med ");
      lcd.print(i + 1);
      lcd.print(": ");
      
      // Format time with leading zeros
      if (medications[i].hour < 10) lcd.print("0");
      lcd.print(medications[i].hour);
      lcd.print(":");
      if (medications[i].minute < 10) lcd.print("0");
      lcd.print(medications[i].minute);

       // Add command instructions in second row
      lcd.setCursor(0, 1);
      lcd.print("#=next | D=exit");
      
      // Wait for a keypress to continue or exit
      while (true) {
        char key = keypad.getKey();
        lastActionTime = millis();
        if (key == '#') {
          break; // Next medication
        } else if (key == 'D') {
          return; // Exit to main display
        }
      }
    }
  }
  
  lcd.clear();
  lcd.print("End of list");
  delay(1000);
}

/**
 * @brief Display current time on LCD
 * 
 * Shows current time from RTC on the LCD display.
 * Optimized to only update when minute changes unless forced update is requested.
 * 
 * @param forceUpdate If true, display will update regardless of time change
 */
void displayTime(bool forceUpdate) {
  static uint8_t lastDisplayedMinute = 99; // Initialize to invalid value
  static byte lastDisplayMode = 99; // Track last display mode to force update on change
  
  DateTime now = rtc.now();
  
  // Update the display if the minute has changed, display mode changed, or a force update is requested
  if (forceUpdate || now.minute() != lastDisplayedMinute || displayMode != lastDisplayMode) {
    lastDisplayedMinute = now.minute();
    lastDisplayMode = displayMode;
    
    lcd.clear();
    lcd.print("Time: ");
    
    // Format time with leading zeros
    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());
    
    lcd.setCursor(0, 1);
    
    // Display different info based on display mode
    if (displayMode == 0) {
      // Default mode - show setup instructions
      lcd.print("Press A -> Setup");
    } else if (displayMode == 1) {
      // Mode 1 - show time to next medication
      lcd.print("Next: ");
      
      // Calculate and display time to next medication
      int hoursToNext = 0;
      int minutesToNext = 0;
      bool hasNext = getTimeToNextMedication(now, hoursToNext, minutesToNext);
      
      if (hasNext) {
        if (hoursToNext > 0) {
          lcd.print(hoursToNext);
          lcd.print("h");
        }
        lcd.print(minutesToNext);
        lcd.print("m");
      } else {
        lcd.print("None");
      }
    }
  }
}

/**
 * @brief Get time remaining until next scheduled medication
 * 
 * @param now Current time from RTC
 * @param hoursToNext Reference to variable to store hours until next medication
 * @param minutesToNext Reference to variable to store minutes until next medication
 * @return true if there is a medication scheduled, false if not
 */
bool getTimeToNextMedication(DateTime now, int &hoursToNext, int &minutesToNext) {
  int minMinutes = 24 * 60; // Initialize to max possible (24 hours)
  bool found = false;
  
  // Check all medications
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    if (medications[i].active) {
      // Calculate minutes from now until this medication time
      int medicationTotalMinutes = medications[i].hour * 60 + medications[i].minute;
      int currentTotalMinutes = now.hour() * 60 + now.minute();
      int minutesDiff;
      
      if (medications[i].dispensedToday) {
        // If already dispensed today, calculate time to next occurrence (tomorrow)
        minutesDiff = medicationTotalMinutes + (24 * 60) - currentTotalMinutes;
      } else {
        // If not yet dispensed, calculate time to this occurrence
        minutesDiff = medicationTotalMinutes - currentTotalMinutes;
        // If the time has passed but not dispensed (might happen if device was off)
        if (minutesDiff < 0) {
          minutesDiff += 24 * 60; // Consider it for tomorrow
        }
      }
      
      // Keep track of the smallest time difference (nearest medication)
      if (minutesDiff < minMinutes) {
        minMinutes = minutesDiff;
        found = true;
      }
    }
  }
  
  // Convert total minutes to hours and minutes
  hoursToNext = minMinutes / 60;
  minutesToNext = minMinutes % 60;
  
  return found;
}

/**
 * @brief Check if the system is currently in sleep mode
 * 
 * @return true if the system is in sleep mode, false otherwise
 */
bool sleep_is_active() {
  // Return the sleep mode flag
  return inSleepMode;
}

/**
 * @brief Check if any medication is due right now
 * 
 * @return true if medication needs to be dispensed, false otherwise
 */
bool checkMedicationDue() {
  DateTime now = rtc.now();
  
  // Log current time
  Serial.print("Checking medication at: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  
  // Reset all dispensed flags at midnight
  if (now.hour() == 0 && now.minute() == 0) {
    Serial.println("Midnight detected! Resetting dispensed flags.");
    resetDailyDispensedFlags();
    return false; // Just resetting flags, no medication to dispense
  }
  
  // Check if any medication needs to be dispensed
  Serial.println("Checking scheduled medications:");
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    Serial.print("  Med #");
    Serial.print(i+1);
    Serial.print(": ");
    
    if (!medications[i].active) {
      Serial.println("Inactive - skipping");
      continue;
    }
    
    Serial.print("Active, ");
    Serial.print("Time=");
    Serial.print(medications[i].hour);
    Serial.print(":");
    Serial.print(medications[i].minute);
    Serial.print(", ");
    
    if (medications[i].dispensedToday) {
      Serial.println("Already dispensed today");
      continue;
    }
    
    Serial.print("Not yet dispensed, ");
    
    if (medications[i].hour == now.hour() && medications[i].minute == now.minute()) {
      Serial.println("TIME MATCH! Medication needs dispensing!");
      return true; // Found medication that needs dispensing
    } else {
      Serial.println("No time match");
    }
  }
  
  Serial.println("No medication due at this time");
  return false; // No medication needs dispensing
}

/**
 * @brief Check if any scheduled medications need to be dispensed
 * 
 * Compares current time with scheduled times and triggers
 * dispensing when a match is found
 */
void checkSchedule() {
  DateTime now = rtc.now();
  
  // Reset all dispensed flags at midnight
  if (now.hour() == 0 && now.minute() == 0) {
    resetDailyDispensedFlags();
  }
  
  // Check if any medication needs to be dispensed
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    if (medications[i].active && !medications[i].dispensedToday &&
        medications[i].hour == now.hour() && 
        medications[i].minute == now.minute()) {
      
      // Time to dispense!
      lcd.clear();
      lcd.print("MEDICINE TIME!");
      lcd.setCursor(0, 1);
      lcd.print("Dispensing #");
      lcd.print(i + 1);
      
      // Activate the dispenser
      dispense();
      
      // Mark as dispensed for today
      medications[i].dispensedToday = true;
      
      // Keep displayed for a while
      lastActionTime = millis(); // Prevent sleep during dispensing
      delay(5000);
    }
  }
}

/**
 * @brief Operate the dispenser mechanism
 * 
 * Controls the top and bottom servos in sequence to
 * dispense a single medication container
 */
void dispense() {
  // Step 1: Open top servo to let pill container drop into holding position
  lcd.clear();
  lcd.print("Dispensing...");
  lcd.setCursor(0, 1);
  lcd.print("Step 1/3");
  
  topServo.write(SERVO_OPEN_POS);
  delay(DISPENSE_DELAY);
  
  // Step 2: Close top servo to prevent more containers from falling
  lcd.setCursor(0, 1);
  lcd.print("Step 2/3       ");
  
  topServo.write(SERVO_CLOSED_POS);
  delay(DISPENSE_DELAY);
  
  // Step 3: Open bottom servo to release the container
  lcd.setCursor(0, 1);
  lcd.print("Step 3/3       ");
  
  bottomServo.write(SERVO_OPEN_POS);
  delay(DISPENSE_DELAY);
  
  // Step 4: Close bottom servo, ready for next dispense
  bottomServo.write(SERVO_CLOSED_POS);
  
  // Sound the buzzer to alert user
  beep(BUZZER_DISPENSE_BEEPS);
  
  lcd.clear();
  lcd.print("Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Dispensed!");
  delay(2000);
  lcd.clear();
  displayTime(true); // Force display update
}

/**
 * @brief Save medication schedules to EEPROM
 * 
 * Writes all medication data to non-volatile memory
 * so settings persist through power cycles
 */
void saveMedications() {
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    int addr = i * sizeof(Medication);
    EEPROM.put(addr, medications[i]);
  }
}

/**
 * @brief Load medication schedules from EEPROM
 * 
 * Reads all medication data from non-volatile memory
 * and initializes with defaults if memory is empty
 */
void loadMedications() {
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    int addr = i * sizeof(Medication);
    EEPROM.get(addr, medications[i]);
    
    // Initialize if EEPROM is empty (first run)
    if (medications[i].hour > 23 || medications[i].minute > 59) {
      medications[i].active = false;
      medications[i].hour = 0;
      medications[i].minute = 0;
      medications[i].dispensedToday = false;
    }
  }
}

/**
 * @brief Reset all dispensed flags
 * 
 * Called at midnight to reset tracking of which
 * medications have been dispensed today
 */
void resetDailyDispensedFlags() {
  for (int i = 0; i < MAX_MEDICATIONS; i++) {
    medications[i].dispensedToday = false;
  }
}

/**
 * @brief Setup watchdog timer for periodic wakeup
 * 
 * Configures the watchdog timer to generate an interrupt after ~8 seconds
 */
void setupWatchdog() {
  // Clear the reset flag
  MCUSR &= ~(1 << WDRF);
  
  // Enable configuration change
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  
  // Set to ~8 seconds timeout and enable interrupt
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
}

/**
 * @brief Enter low power sleep mode
 * 
 * Puts the system into power-saving mode after
 * a period of inactivity
 */
void enterSleepMode() {
  // Reset the wake flags
  buttonWakeFlag = false;
  watchdogWakeFlag = false;
  
  // Turn off display to save power
  lcd.noBacklight();
  lcd.noDisplay();
  
  // Set sleep mode flag
  inSleepMode = true;
  
  // Configure watchdog for periodic wake
  setupWatchdog();
  
  // Disable ADC to save power
  ADCSRA &= ~(1 << ADEN);
  
  // Configure sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  // Make sure interrupt flags are clear before sleeping
  EIFR = (1 << INTF0);  // Clear any pending external interrupt flag
  
  // Enter sleep mode
  sleep_cpu();
  
  // Code resumes here after waking up
  
  while(!buttonWakeFlag && !watchdogWakeFlag){}
  
  // Disable sleep mode
  sleep_disable();
  
  // Re-enable ADC
  ADCSRA |= (1 << ADEN);
  
  // Disable watchdog timer
  wdt_disable();
  
  // Clear sleep mode flag
  inSleepMode = false;
}

/**
 * @brief Blink the wake indicator LED
 * 
 * @param count Number of times to blink the LED
 */
void blinkLED(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(WAKE_LED_PIN, HIGH);
    delay(LED_BLINK_DURATION);
    digitalWrite(WAKE_LED_PIN, LOW);
    if (i < count - 1) {
      delay(LED_BLINK_DURATION); // Pause between blinks
    }
  }
}
