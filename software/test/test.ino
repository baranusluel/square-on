#define X_RANGE_STEPS 1950
#define Y_RANGE_STEPS 2400

#define dirA 13
#define stepA 12
#define dirB 11
#define stepB 10
#define stopX A0
#define stopY A1

enum AXIS { X, Y };

int curX = 0;
int curY = 0;

void zero();
void moveSteps(AXIS axis, int steps);
void moveTo(int newX, int newY);

void setup() {
  Serial.begin(9600);
  
  pinMode(dirA, OUTPUT);
  pinMode(stepA, OUTPUT);
  pinMode(dirB, OUTPUT);
  pinMode(stepB, OUTPUT);

  pinMode(stopX, INPUT);
  pinMode(stopY, INPUT);

  zero();
}

void loop() {
  while (true) {
    if (Serial.available()) {
      float x = Serial.parseFloat();
      float y = Serial.parseFloat();
      char r = Serial.read();
      if (r != '\n') continue;
      moveTo(x*X_RANGE_STEPS, y*Y_RANGE_STEPS);
    }
  }
}

void zero() {
  digitalWrite(dirA, HIGH);
  digitalWrite(dirB, HIGH);
  while (digitalRead(stopX)) {
    digitalWrite(stepA, HIGH);
    digitalWrite(stepB, HIGH);
    delay(1);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delay(1);
  }
  
  digitalWrite(dirA, LOW);
  digitalWrite(dirB, HIGH);
  while (digitalRead(stopY)) {
    digitalWrite(stepA, HIGH);
    digitalWrite(stepB, HIGH);
    delay(1);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delay(1);
  }
}

void moveTo(int newX, int newY) {
  moveSteps(X, newX - curX);
  moveSteps(Y, newY - curY);
  curX = newX;
  curY = newY;
}

void moveSteps(AXIS axis, int steps) {
  digitalWrite(dirB, steps < 0);
  digitalWrite(dirA,
      (axis == Y && steps > 0) || (axis == X && steps < 0));
      
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(stepA, HIGH);
    digitalWrite(stepB, HIGH);
    delay(1);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delay(1);
  }
}
