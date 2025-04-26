#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

#define BT_RX A0
#define BT_TX A1


SoftwareSerial BTSerial(BT_RX, BT_TX); // 


// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
#define BUZZER_PIN 11
#define LED_PIN 12 // yellow led
unsigned long buttonPressStart = 0;
bool isPressing = false;
bool warningActive = false;
unsigned long lastBlinkTime = 0;
bool ledState = LOW;
#define RED_LED_PIN 13

// Modes for setting and running the timer
enum Mode {
  SET_PRESENTATION,
  SET_QUESTION,
  RUN_PRESENTATION,
  RUN_QUESTION,
  FINISHED
};

Mode currentMode = SET_PRESENTATION;

// Timers (in minutes)
int presentationTime = 5;
int questionTime = 2;
int secondsRemaining = 0;

// Button states
int stateSET = 1, stateUP = 1, stateDOWN = 1;

// Timer tracking
unsigned long lastTick = 0;

// Helper vars
int dg = 0;

void setup() {
  lcd.begin(16, 2);
  BTSerial.begin(9600); // Bluetooth default baud rate

  // button is set as high, when pressed goes to low
  pinMode(8, INPUT_PULLUP);  // SET
  pinMode(9, INPUT_PULLUP);  // UP
  pinMode(10, INPUT_PULLUP); // DOWN
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW); // Start OFF
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW); // Start OFF

   // ✨ New Welcome Screen
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Presentation");
  lcd.setCursor(0, 1);
  lcd.print("Timer Starting...");
  delay(2000); // wait for 2 seconds

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Present Time:");
  lcd.setCursor(0, 1);
  lcd.print(presentationTime);
  lcd.print(" min");
}

void loop() {
  handleWarningLED(); // link if needed

  handleButtons();

  handleBluetooth(); // 


  // Countdown logic
  if (currentMode == RUN_PRESENTATION || currentMode == RUN_QUESTION) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTick >= 1000 && secondsRemaining > 0) {
      lastTick = currentMillis;
      secondsRemaining--;
      displayTime();

      if (secondsRemaining == 60) {
        showWarning();
      } else if (secondsRemaining == 0) {
        showTimeUp();
        transitionToNextMode();
      }
    }
  }
}

void handleButtons() {
  // SET button
  if (!digitalRead(8)) { // Button is pressed
      if (!isPressing) {
        isPressing = true;
        buttonPressStart = millis();
      } else {
        if (millis() - buttonPressStart >= 3000) { // 3 seconds hold detected
          resetTimer();
          isPressing = false;
          delay(500); // prevent re-trigger
        }
      }
  } else { // Button is released
      if (isPressing) {
        isPressing = false;
        delay(200); // normal debounce when just tapped
        if (currentMode == SET_PRESENTATION) {
          currentMode = SET_QUESTION;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set Question Time:");
          lcd.setCursor(0, 1);
          lcd.print(questionTime);
          lcd.print(" min");
        } else if (currentMode == SET_QUESTION) {
          currentMode = RUN_PRESENTATION;
          secondsRemaining = presentationTime * 60;
          lcd.clear();
        }
      }
  }

  // UP button
  if (!digitalRead(9) && stateUP == 1) {
    stateUP = 0;
    delay(200);
    if (currentMode == SET_PRESENTATION) {
      presentationTime++;
    } else if (currentMode == SET_QUESTION) {
      questionTime++;
    }
    updateSettingDisplay();
  } else if (digitalRead(9) && stateUP == 0) {
    stateUP = 1;
  }

  // DOWN button
  if (!digitalRead(10) && stateDOWN == 1) {
    stateDOWN = 0;
    delay(200);
    if (currentMode == SET_PRESENTATION && presentationTime > 1) {
      presentationTime--;
    } else if (currentMode == SET_QUESTION && questionTime > 1) {
      questionTime--;
    }
    updateSettingDisplay();
  } else if (digitalRead(10) && stateDOWN == 0) {
    stateDOWN = 1;
  }
}

void handleBluetooth() {
  if (BTSerial.available()) {
    char command = BTSerial.read();
    
    if (command == 'S') { // 'S' = Set
      if (currentMode == SET_PRESENTATION) {
        currentMode = SET_QUESTION;
      } else if (currentMode == SET_QUESTION) {
        currentMode = RUN_PRESENTATION;
        secondsRemaining = presentationTime * 60;
        lcd.clear();
      }
    } else if (command == 'P') { // 'U' = UP
      if (currentMode == SET_PRESENTATION) {
        presentationTime++;
        updateSettingDisplay();
      } else if (currentMode == SET_QUESTION) {
        questionTime++;
        updateSettingDisplay();
      }
    } else if (command == 'R') { // 'D' = DOWN
      if (currentMode == SET_PRESENTATION && presentationTime > 1) {
        presentationTime--;
        updateSettingDisplay();
      } else if (currentMode == SET_QUESTION && questionTime > 1) {
        questionTime--;
        updateSettingDisplay();
      }
    } else if (command == 'T') { // 'R' = Reset
      resetTimer();
    }
  }
}


void displayTime() {
  int mins = secondsRemaining / 60;
  int secs = secondsRemaining % 60;
  lcd.setCursor(0, 0);
  lcd.print("Time Remaining:");
  lcd.setCursor(0, 1);
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print(":");
  if (secs < 10) lcd.print("0");
  lcd.print(secs);
}

void updateSettingDisplay() {
  lcd.setCursor(0, 1);
  lcd.print("                "); // clear line
  lcd.setCursor(0, 1);
  if (currentMode == SET_PRESENTATION) {
    lcd.print(presentationTime);
    lcd.print(" min");
  } else if (currentMode == SET_QUESTION) {
    lcd.print(questionTime);
    lcd.print(" min");
  }
}

void showWarning() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1 min left!");
  warningActive = true;  // Start blinking LED
  delay(1500);
  displayTime();
}

void showTimeUp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TIME UP");

  warningActive = false;   //  stop blinking
  digitalWrite(LED_PIN, LOW); // Turn LED OFF

  // Flash Red LED 
  for (int i = 0; i < 2; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(250);  // ON for 0.25s
    digitalWrite(RED_LED_PIN, LOW);
    delay(250);  // OFF for 0.25s
  }


  buzz(); 
  delay(2000);
}

void buzz() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
}

void resetTimer() {
  currentMode = SET_PRESENTATION;
  presentationTime = 5;
  questionTime = 2;
  secondsRemaining = 0;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Resetting...");
  delay(1000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Present Time:");
  lcd.setCursor(0, 1);
  lcd.print(presentationTime);
  lcd.print(" min");
}

void handleWarningLED() {
  if (warningActive) {
    if (millis() - lastBlinkTime >= 500) { // blink every 0.5 sec
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}

void transitionToNextMode() {
  if (currentMode == RUN_PRESENTATION) {
    currentMode = RUN_QUESTION;
    secondsRemaining = questionTime * 60;
    lcd.clear();
  } else if (currentMode == RUN_QUESTION) {
    currentMode = FINISHED;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All Done!");
  }
}