#include <HX711.h>
#include <EEPROM.h>

// Define HX711 connections
const int dout = 2;
const int sck = 3;

// Define Thumbstick connections
const int vrxPin = A0;
const int vryPin = A1;
const int swPin = 4;

HX711 scale;

// Calibration factor (needs to be determined)
float calibrationFactor = -819.2; // Example value, needs calibration

enum Mode { MENU, CALIBRATE, TEST, VIEW_RESULTS, VIEW_TEST_RESULTS, MANAGE_RESULTS, DELETE_CONFIRM, DELETE_ALL_CONFIRM };
Mode currentMode = MENU;

int menuSelection = 0; // 0 for Calibrate, 1 for Test, 2 for View Results, 3 for Manage Results
int testSelection = 0; // To select which test results to view or manage
bool menuChanged = true; // Flag to indicate if the menu needs to be printed

const int maxReadings = 10;  // Reduced number of readings per test to save memory
struct Test {
  unsigned long timestamps[maxReadings];
  float measurements[maxReadings];
  int count;
  int id; // Unique identifier for each test
};

const int maxTests = 3;  // Reduced number of tests to save memory
Test tests[maxTests];
int testIndex = 0; // Index for the current test
int nextTestId = 1; // Next test ID to assign

void setup() {
  Serial.begin(9600);

  // Initialize HX711
  scale.begin(dout, sck);
  scale.set_scale(); // Not using calibration factor initially
  scale.tare(); // Set zero point

  // Initialize Thumbstick
  pinMode(vrxPin, INPUT);
  pinMode(vryPin, INPUT);
  pinMode(swPin, INPUT_PULLUP); // Internal pull-up resistor

  // Load test data from EEPROM
  loadTestsFromEEPROM();

  printMenu();
}

void loop() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;

  // Read thumbstick values
  int vryValue = analogRead(vryPin);

  // Use thumbstick to navigate the menu
  if (millis() - lastDebounceTime > debounceDelay) {
    if (vryValue < 400 && menuSelection > 0) {
      menuSelection--; // Move up
      menuChanged = true;
      lastDebounceTime = millis();
    } else if (vryValue > 600 && menuSelection < 3) {
      menuSelection++; // Move down
      menuChanged = true;
      lastDebounceTime = millis();
    }
  }

  // Print the menu only if it has changed
  if (menuChanged) {
    printMenu();
    menuChanged = false;
  }

  // Read thumbstick switch for menu selection
  if (digitalRead(swPin) == LOW) {
    delay(50); // Debounce
    if (digitalRead(swPin) == LOW) {
      selectMenuOption();
      while (digitalRead(swPin) == LOW); // Wait for release
      delay(50); // Debounce
    }
  }

  switch (currentMode) {
    case MENU:
      // Stay in menu
      break;
    case CALIBRATE:
      // Run calibration routine
      calibrate();
      break;
    case TEST:
      // Run test routine
      test();
      break;
    case VIEW_RESULTS:
      // View previous test results
      viewResults();
      break;
    case VIEW_TEST_RESULTS:
      // View specific test results
      viewSpecificTestResults();
      break;
    case MANAGE_RESULTS:
      // Manage test results
      manageResults();
      break;
    case DELETE_CONFIRM:
      // Confirm deletion of a test result
      deleteConfirm();
      break;
    case DELETE_ALL_CONFIRM:
      // Confirm deletion of all test results
      deleteAllConfirm();
      break;
  }
}

void printMenu() {
  Serial.println(F("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")); // Clear the Serial Monitor
  Serial.println(F("Select an option:"));
  if (menuSelection == 0) {
    Serial.println(F("> Calibrate"));
  } else {
    Serial.println(F("  Calibrate"));
  }
  if (menuSelection == 1) {
    Serial.println(F("> Test"));
  } else {
    Serial.println(F("  Test"));
  }
  if (menuSelection == 2) {
    Serial.println(F("> View Results"));
  } else {
    Serial.println(F("  View Results"));
  }
  if (menuSelection == 3) {
    Serial.println(F("> Manage Results"));
  } else {
    Serial.println(F("  Manage Results"));
  }
}

void selectMenuOption() {
  if (menuSelection == 0) {
    currentMode = CALIBRATE;
    Serial.println(F("Calibrate selected."));
  } else if (menuSelection == 1) {
    currentMode = TEST;
    Serial.println(F("Test selected."));
  } else if (menuSelection == 2) {
    currentMode = VIEW_RESULTS;
    Serial.println(F("View Results selected."));
    testSelection = 0; // Reset the test selection
    printViewResultsMenu(); // Print the view results menu
  } else if (menuSelection == 3) {
    currentMode = MANAGE_RESULTS;
    Serial.println(F("Manage Results selected."));
    testSelection = 0; // Reset the test selection
    printManageResultsMenu(); // Print the manage results menu
  }
}

void calibrate() {
  Serial.println(F("Calibration mode. Place a known weight on the scale."));
  delay(5000); // Wait for user to place weight

  long reading = scale.read();
  Serial.print(F("Raw Reading: "));
  Serial.println(reading);
  
  Serial.println(F("Enter the known weight in grams: "));
  while (!Serial.available());
  float knownWeight = Serial.parseFloat();

  calibrationFactor = reading / knownWeight;
  Serial.print(F("New Calibration Factor: "));
  Serial.println(calibrationFactor);

  scale.set_scale(calibrationFactor);
  Serial.println(F("Calibration complete. Returning to menu."));
  currentMode = MENU;
  menuChanged = true;
}

void test() {
  if (testIndex >= maxTests) {
    Serial.println(F("Maximum number of tests reached. Cannot save more tests."));
    currentMode = MENU;
    menuChanged = true;
    return;
  }

  Serial.println(F("Test mode. Press the thumbstick to stop and display results."));
  tests[testIndex].count = 0;
  tests[testIndex].id = nextTestId++;

  while (digitalRead(swPin) == HIGH && tests[testIndex].count < maxReadings) {
    long reading = scale.read();
    float weight = reading / calibrationFactor;

    // Save the reading and timestamp
    tests[testIndex].timestamps[tests[testIndex].count] = millis();
    tests[testIndex].measurements[tests[testIndex].count] = weight;
    tests[testIndex].count++;

    Serial.print(F("Time (ms): "));
    Serial.print(tests[testIndex].timestamps[tests[testIndex].count - 1]);
    Serial.print(F("  Weight (grams): "));
    Serial.println(weight);

    delay(500); // Update every half second
  }

  delay(50); // Debounce
  saveTestToEEPROM(testIndex);
  testIndex++;

  Serial.println(F("Test completed and saved. Returning to menu."));
  currentMode = MENU;
  menuChanged = true;
}

void viewResults() {
  navigateViewResultsMenu();
}

void printViewResultsMenu() {
  Serial.println(F("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")); // Clear the Serial Monitor
  Serial.println(F("Select a test to view results:"));
  for (int i = 0; i < testIndex; i++) {
    if (testSelection == i) {
      Serial.print(F("> Test "));
      Serial.println(tests[i].id);
    } else {
      Serial.print(F("  Test "));
      Serial.println(tests[i].id);
    }
  }
  if (testSelection == testIndex) {
    Serial.println(F("> Return to Main Menu"));
  } else {
    Serial.println(F("  Return to Main Menu"));
  }
}

void navigateViewResultsMenu() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;

  // Read thumbstick values
  int vryValue = analogRead(vryPin);

  // Use thumbstick to navigate the menu
  if (millis() - lastDebounceTime > debounceDelay) {
    if (vryValue < 400 && testSelection > 0) {
      testSelection--; // Move up
      menuChanged = true;
      lastDebounceTime = millis();
    } else if (vryValue > 600 && testSelection < testIndex) {
      testSelection++; // Move down
      menuChanged = true;
      lastDebounceTime = millis();
    }
  }

  // Print the menu only if it has changed
  if (menuChanged) {
    printViewResultsMenu();
    menuChanged = false;
  }

  // Read thumbstick switch for menu selection
  if (digitalRead(swPin) == LOW) {
    delay(50); // Debounce
    if (digitalRead(swPin) == LOW) {
      selectViewResultsOption();
      while (digitalRead(swPin) == LOW); // Wait for release
      delay(50); // Debounce
    }
  }
}

void selectViewResultsOption() {
  if (testSelection >= 0 && testSelection < testIndex) {
    currentMode = VIEW_TEST_RESULTS;
    Serial.print(F("Viewing results for test: "));
    Serial.println(tests[testSelection].id);
    viewSpecificTestResults();
  } else if (testSelection == testIndex) {
    currentMode = MENU;
    menuChanged = true;
  }
}

void viewSpecificTestResults() {
  // Print selected test results
  Serial.print(F("Results for test: "));
  Serial.println(tests[testSelection].id);
  for (int i = 0; i < tests[testSelection].count; i++) {
    Serial.print(F("Time (ms): "));
    Serial.print(tests[testSelection].timestamps[i]);
    Serial.print(F("  Weight (grams): "));
    Serial.println(tests[testSelection].measurements[i]);
  }

  Serial.println(F("Press the thumbstick to return to the menu."));
  while (digitalRead(swPin) == HIGH);
  delay(50); // Debounce

  currentMode = VIEW_RESULTS;
  menuChanged = true;
}

void manageResults() {
  navigateManageResultsMenu();
}

void printManageResultsMenu() {
  Serial.println(F("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")); // Clear the Serial Monitor
  Serial.println(F("Select a test to delete:"));
  for (int i = 0; i < testIndex; i++) {
    if (testSelection == i) {
      Serial.print(F("> Test "));
      Serial.println(tests[i].id);
    } else {
      Serial.print(F("  Test "));
      Serial.println(tests[i].id);
    }
  }
  if (testSelection == testIndex) {
    Serial.println(F("> Delete All Tests"));
  } else {
    Serial.println(F("  Delete All Tests"));
  }
  if (testSelection == testIndex + 1) {
    Serial.println(F("> Return to Main Menu"));
  } else {
    Serial.println(F("  Return to Main Menu"));
  }
}

void navigateManageResultsMenu() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;

  // Read thumbstick values
  int vryValue = analogRead(vryPin);

  // Use thumbstick to navigate the menu
  if (millis() - lastDebounceTime > debounceDelay) {
    if (vryValue < 400 && testSelection > 0) {
      testSelection--; // Move up
      menuChanged = true;
      lastDebounceTime = millis();
    } else if (vryValue > 600 && testSelection < testIndex + 1) {
      testSelection++; // Move down
      menuChanged = true;
      lastDebounceTime = millis();
    }
  }

  // Print the menu only if it has changed
  if (menuChanged) {
    printManageResultsMenu();
    menuChanged = false;
  }

  // Read thumbstick switch for menu selection
  if (digitalRead(swPin) == LOW) {
    delay(50); // Debounce
    if (digitalRead(swPin) == LOW) {
      selectManageResultsOption();
      while (digitalRead(swPin) == LOW); // Wait for release
      delay(50); // Debounce
    }
  }
}

void selectManageResultsOption() {
  if (testSelection >= 0 && testSelection < testIndex) {
    currentMode = DELETE_CONFIRM;
    Serial.print(F("Are you sure you want to delete Test "));
    Serial.print(tests[testSelection].id);
    Serial.println(F("? (Press to confirm, hold to cancel)"));
  } else if (testSelection == testIndex) {
    currentMode = DELETE_ALL_CONFIRM;
    Serial.println(F("Are you sure you want to delete all tests? (Press to confirm, hold to cancel)"));
  } else if (testSelection == testIndex + 1) {
    currentMode = MENU;
    menuChanged = true;
  }
}

void deleteConfirm() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;
  bool confirm = false;
  Serial.println(F("Confirm deletion:"));
  Serial.print(F("> Yes\n  No\n"));

  while (true) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce
        if (digitalRead(swPin) == LOW) {
          confirm = true;
          break;
        }
      }
      int vryValue = analogRead(vryPin);
      if (vryValue > 600) {
        break;
      }
      lastDebounceTime = millis();
    }
  }

  if (confirm) {
    // Delete the test
    deleteTest(testSelection);
  } else {
    // Cancel deletion
    currentMode = MANAGE_RESULTS;
  }

  menuChanged = true;
}

void deleteAllConfirm() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;
  bool confirm = false;
  Serial.println(F("Confirm deletion of all tests:"));
  Serial.print(F("> Yes\n  No\n"));

  while (true) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (digitalRead(swPin) == LOW) {
        delay(50); // Debounce
        if (digitalRead(swPin) == LOW) {
          confirm = true;
          break;
        }
      }
      int vryValue = analogRead(vryPin);
      if (vryValue > 600) {
        break;
      }
      lastDebounceTime = millis();
    }
  }

  if (confirm) {
    // Delete all tests
    deleteAllTests();
  } else {
    // Cancel deletion
    currentMode = MANAGE_RESULTS;
  }

  menuChanged = true;
}

void deleteTest(int index) {
  for (int i = index; i < testIndex - 1; i++) {
    tests[i] = tests[i + 1];
  }
  testIndex--;
  saveAllTestsToEEPROM();
  Serial.println(F("Test deleted."));
  currentMode = MANAGE_RESULTS;
}

void deleteAllTests() {
  for (int i = 0; i < maxTests; i++) {
    tests[i].count = 0;
  }
  testIndex = 0;
  saveAllTestsToEEPROM();
  Serial.println(F("All tests deleted."));
  currentMode = MANAGE_RESULTS;
}

void saveTestToEEPROM(int index) {
  int eepromAddress = index * sizeof(Test);
  EEPROM.put(eepromAddress, tests[index]);
}

void saveAllTestsToEEPROM() {
  for (int i = 0; i < maxTests; i++) {
    int eepromAddress = i * sizeof(Test);
    EEPROM.put(eepromAddress, tests[i]);
  }
}

void loadTestsFromEEPROM() {
  for (int i = 0; i < maxTests; i++) {
    int eepromAddress = i * sizeof(Test);
    EEPROM.get(eepromAddress, tests[i]);
    if (tests[i].count > 0) {
      testIndex++;
      if (tests[i].id >= nextTestId) {
        nextTestId = tests[i].id + 1;
      }
    }
  }
}
