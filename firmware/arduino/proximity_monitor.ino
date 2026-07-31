// ======================
// OBJECTS
// ======================
#include <Servo.h>
Servo scanner;

// ======================
// GLOBAL VARIABLES
// ======================

// Ultrasonic sensor pins
const int trigpin = 12;
const int echopin = 11;

// Buzzer
const int buzzerpin = 10;
const int warningtone = 1000;
const int dangertone = 1000;

// Button
const int buttonpin = 9;

// LEDs
const int redpin = 7;
const int greenpin = 6;
const int bluepin = 5;

// Servo
const int servopin = 3;

// Distance thresholds (adjusted by potentiometer in AUTO mode)
int baseWarning = 25;
int warningdistance = 25;
const int dangerdistance = 10;

// Potentiometer
const int potpin = A0;

// ======================
// ENUMS
// ======================
enum Mode { AUTO, MANUAL };
enum systemState { SAFE, WARNING, DANGER };

Mode currentmode = AUTO;
systemState currentstate = SAFE;

// ======================
// SETUP PINS
// ======================
void initializePins() {

  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  digitalWrite(trigpin, LOW);

  pinMode(buzzerpin, OUTPUT);

  pinMode(buttonpin, INPUT);

  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);

  scanner.attach(servopin);
}

// ======================
// MODE SWITCH (button toggle)
// ======================
void checkModeSwitch() {

  static bool previousState = LOW;
  bool currentButtonState = digitalRead(buttonpin);

  // Detect rising edge (button press)
  if (previousState == LOW && currentButtonState == HIGH) {
    currentmode = (currentmode == AUTO) ? MANUAL : AUTO;
  }

  previousState = currentButtonState;

  // Potentiometer ONLY used in MANUAL mode
  if (currentmode ==  MANUAL) {

    int potState = analogRead(potpin);

    // map pot to a small adjustment range instead of full override
    int offset = map(potState, 0, 1023, -10, 10);

    warningdistance = baseWarning + offset;
  }
}

// ======================
// ULTRASONIC READ
// ======================
long readDistance() {

  digitalWrite(trigpin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigpin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigpin, LOW);

  long duration = pulseIn(echopin, HIGH, 20000);

  // If sensor fails, return invalid marker
  if (duration == 0) return -1;

  return (0.0343 * duration) / 2;
}

// ======================
// DISTANCE FILTER
// ======================
long getSmoothedDistance() {

  static long lastValidDistance = 100;   // starting value

  long d = readDistance();

  // Sensor failed or reading is impossible
  if (d < 2 || d > 400) {
    return lastValidDistance;
  }

  // Save the newest valid reading
  lastValidDistance = d;

  return lastValidDistance;
}

// ======================
// STATE MACHINE 
// ======================
void updateState(long distance) {

  if (distance < 0) {
    return; // ignore bad readings completely
  }

  if (distance <= dangerdistance) {
    currentstate = DANGER;
  }
  else if (distance <= warningdistance) {
    currentstate = WARNING;
  }
  else {
    currentstate = SAFE;
  }
}
// ======================
// OUTPUTS (LED + BUZZER)
// ======================
void updateOutputs() {

  noTone(buzzerpin);

  // HARD OFF all LEDs first
  digitalWrite(redpin, LOW);
  digitalWrite(greenpin, LOW);
  digitalWrite(bluepin, LOW);

  switch (currentstate) {

    case SAFE:
      digitalWrite(greenpin, HIGH);
      break;

    case WARNING:
      digitalWrite(bluepin, HIGH);
      break;

    case DANGER:
      digitalWrite(redpin, HIGH);
      //tone(buzzerpin, dangertone);
      break;
  }
}
// ======================
// SERVO (SMOOTH NON-BLOCKING SWEEP)
// ======================
void rotateServo() {

  static int angle = 0;
  static int step = 2;
  static unsigned long lastUpdate = 0;

  const int interval = 10;

  // timing control prevents jitter in the servo
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();

    if (currentmode == AUTO) {

      angle += step;

      // reverse direction at limits
      if (angle >= 180 || angle <= 0) {
        step = -step;
      }

      scanner.write(angle);
    }

    else {
      int potState = analogRead(potpin);
      int rotation = map(potState, 0, 1023, 0, 180);

      scanner.write(rotation);
    }
  }
}

// ======================
// SERIAL DEBUG OUTPUT
// ======================
void printSystemData(long distance) {

  Serial.print("[MODE]: ");
  Serial.print(currentmode == AUTO ? "AUTO" : "MANUAL");

  Serial.print(" | DIST: ");
  Serial.print(distance);

  Serial.print(" | STATE: ");

  switch (currentstate) {

    case SAFE:
      Serial.println("SAFE");
      break;

    case WARNING:
      Serial.println("WARNING");
      break;

    case DANGER:
      tone(buzzerpin, dangertone);
      Serial.println("DANGER");
      break;
  }
}

// ======================
// SETUP
// ======================
void setup() {
  initializePins();
  Serial.begin(115200);
}

// ======================
// LOOP
// ======================
void loop() {

  checkModeSwitch();

  long distance = getSmoothedDistance();

  updateState(distance);

  updateOutputs();

  rotateServo();

  printSystemData(distance);
}



