#define X_RANGE_STEPS 1900*4
#define Y_RANGE_STEPS 2350*4
#define speedMicros 500

#define stepB 2
#define dirB 4
#define stepA 3
#define dirA 5
#define stopX 6
#define stopY 7
#define magnet 9

// Directions: X is horizontal, Y is vertical,
// main diagonal is / direction (from origin),
// anti diagonal is \ direction.
enum AXIS { X, Y, MAIN_DIAG, ANTI_DIAG};

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
        moveTo(x, y);
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
    delayMicroseconds(speedMicros);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delayMicroseconds(speedMicros);
  }
}

void moveTo(int newX, int newY) {
  // Clamp coordinates to valid range (0 to max range)
  newX = max(min(newX, X_RANGE_STEPS), 0);
  newY = max(min(newY, Y_RANGE_STEPS), 0);
  // Move to desired location
  if (abs(newX - curX) == abs(newY - curY)) {
    // If perfectly diagonal motion, just move diagonally
    if (newX - curX == newY - curY) // If same sign
      moveSteps(MAIN_DIAG, 2*(newX - curX));
    else // Opposite signs
      moveSteps(ANTI_DIAG, 2*(newX - curX));
  } else {
    // Otherwise move in X and Y separately, Manhattan-distance
    moveSteps(X, newX - curX);
    moveSteps(Y, newY - curY);
  }
  // Update current position
  curX = newX;
  curY = newY;
}

void moveSteps(AXIS axis, int steps) {
  // Set stepper motor directions
  switch (axis) {
    case X:
      digitalWrite(dirB, steps < 0);
      digitalWrite(dirA, steps < 0);
      break;
    case Y:
      digitalWrite(dirB, steps < 0);
      digitalWrite(dirA, steps > 0);
      break;
    case MAIN_DIAG:
      digitalWrite(dirB, steps < 0);
      break;
    case ANTI_DIAG:
      digitalWrite(dirA, steps < 0);
      break;
  }
      
  // Pulse given number of steps
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(stepA, axis != MAIN_DIAG ? HIGH : LOW);
    digitalWrite(stepB, axis != ANTI_DIAG ? HIGH : LOW);
    delayMicroseconds(speedMicros);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delayMicroseconds(speedMicros);
  }
}

