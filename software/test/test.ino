#define X_RANGE_STEPS 1900
#define Y_RANGE_STEPS 2350

#define dirA 13
#define stepA 12
#define dirB 11
#define stepB 10
#define stopX A0
#define stopY A1
#define magnet 9

enum AXIS { X, Y };

void zero();
void zeroAxis(AXIS axis);
void moveTo(int newX, int newY);
void moveSteps(AXIS axis, int steps);

int curX = 0;
int curY = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(dirA, OUTPUT);
  pinMode(stepA, OUTPUT);
  pinMode(dirB, OUTPUT);
  pinMode(stepB, OUTPUT);
  
  pinMode(magnet, OUTPUT);
  digitalWrite(magnet, LOW);

  pinMode(stopX, INPUT);
  pinMode(stopY, INPUT);

  zero();
}

void loop() {
  while (true) {
    if (Serial.available()) {
      char r = Serial.read(); // prefix
      if (r == 'm') { // magnet on/off
        int on = Serial.parseInt();
        if (Serial.read() != '\n') continue; // valid EOL
        digitalWrite(magnet, on);
      } else if (r == 'x') { // coordinate
        float x = Serial.parseFloat();
        float y = Serial.parseFloat();
        if (Serial.read() != '\n') continue; // valid EOL
        moveTo(x*X_RANGE_STEPS, y*Y_RANGE_STEPS);
      }
    }
  }
}

void zero() {
  zeroAxis(X);
  zeroAxis(Y);
}

void zeroAxis(AXIS axis) {
  // Depending on axis being zeroed, set motor directions
  // and endstop pin. Reset current position variable
  int stopPin = 0;
  if (axis == X) {
    digitalWrite(dirA, HIGH);
    digitalWrite(dirB, HIGH);
    stopPin = stopX;
    curX = 0;
  } else if (axis == Y) {
    digitalWrite(dirA, LOW);
    digitalWrite(dirB, HIGH);
    stopPin = stopY;
    curY = 0;
  }
  // Move until switch is activated
  while (digitalRead(stopPin)) {
    digitalWrite(stepA, HIGH);
    digitalWrite(stepB, HIGH);
    delay(1);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delay(1);
  }
}

void moveTo(int newX, int newY) {
  // Clamp coordinates to valid range (0 to max range)
  newX = max(min(newX, X_RANGE_STEPS), 0);
  newY = max(min(newY, Y_RANGE_STEPS), 0);
  // Move to desired location
  moveSteps(X, newX - curX);
  moveSteps(Y, newY - curY);
  // Update current position
  curX = newX;
  curY = newY;
}

void moveSteps(AXIS axis, int steps) {
  // Set stepper motor directions
  digitalWrite(dirB, steps < 0);
  digitalWrite(dirA,
      (axis == Y && steps > 0) || (axis == X && steps < 0));
      
  // Pulse given number of steps
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(stepA, HIGH);
    digitalWrite(stepB, HIGH);
    delay(1);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delay(1);
  }
}
