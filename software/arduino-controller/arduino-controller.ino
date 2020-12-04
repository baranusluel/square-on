/// CONSTANTS
// ROM of robot in steps
#define X_RANGE_STEPS 1900*4
#define Y_RANGE_STEPS 2350*4
// Speed of robot, as delay between steps
#define speedMicros 500
// Number of steps to overshoot target by when moving a piece,
// to combat lagging piece effect depending on size of magnet.
#define OVERSHOOT_STEPS 250

// Stepper driver pins
#define stepB 2
#define dirB 4
#define stepA 3
#define dirA 5
// Endstop switch pins
#define stopX 6
#define stopY 7
// Electromagnet pin
#define magnet 9

// Axis/direction symbols:
// X is horizontal, Y is vertical,
// main diagonal is / direction (from origin),
// anti diagonal is \ direction.
enum AXIS { X, Y, MAIN_DIAG, ANTI_DIAG};

// Board state buffers
char initialBoard[8][8] = {{'r','n','b','q','k','b','n','r'},
                           {'p','p','p','p','p','p','p','p'},
                           {'.','.','.','.','.','.','.','.'},
                           {'.','.','.','.','.','.','.','.'},
                           {'.','.','.','.','.','.','.','.'},
                           {'.','.','.','.','.','.','.','.'},
                           {'P','P','P','P','P','P','P','P'},
                           {'R','N','B','Q','K','B','N','R'}};
char currentBoard[8][8];
char newBoard[8][8];

// Represents movement of a single chess piece and possibly a capture.
struct BoardMove {
  char piece;
  char pieceCaptured;
  int fromRow;
  int fromCol;
  int toRow;
  int toCol;
};

// Current position in steps
int curX = 0;
int curY = 0;

/// METHODS

void zero();
void zeroAxis(AXIS axis);
void moveTo(int newX, int newY, int speedDelay, bool isKnight, bool isCapture, bool overshoot);
void moveSteps(AXIS axis, int steps, int speedDelay);
void moveToSquare(int col, int row, int speedDelay, bool isKnight, bool isCapture, bool overshoot);
void copyBoard(char fromBoard[8][8], char toBoard[8][8]);
void scanBoardChanges();

void setup() {
  Serial.begin(9600);
  Serial.println("start");
  
  pinMode(dirA, OUTPUT);
  pinMode(stepA, OUTPUT);
  pinMode(dirB, OUTPUT);
  pinMode(stepB, OUTPUT);
  
  pinMode(magnet, OUTPUT);
  digitalWrite(magnet, LOW);

  pinMode(stopX, INPUT);
  pinMode(stopY, INPUT);

  zero();
  copyBoard(initialBoard, currentBoard);
}

void loop() {
  while (true) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      Serial.println(cmd);
      if (cmd == "new") {
        Serial.println("new ack");
        // Assume player manually resets pieces
        copyBoard(initialBoard, currentBoard);
      } else if (cmd == "board") {
        Serial.println("board ack");
        // Read new board state into buffer
        for (int r = 0; r < 8; r++) {
          String row = Serial.readStringUntil('\n');
          Serial.println(row);
          for (int c = 0; c < 8; c++) {
            newBoard[r][c] = row[c*2];
          }
        }
        // Identify different between current and new board,
        // make the necessary chess move.
        scanBoardChanges();
        Serial.println("done");
      } else {
        Serial.println("what?");
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
    // Go left
    digitalWrite(dirA, HIGH);
    digitalWrite(dirB, HIGH);
    stopPin = stopX;
    curX = 0;
  } else if (axis == Y) {
    // Go down
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

void moveTo(int newX, int newY, int speedDelay, bool isKnight, bool isCapture, bool overshoot) {
  // Clamp coordinates to valid range (0 to max range)
  newX = max(min(newX, X_RANGE_STEPS), 0);
  newY = max(min(newY, Y_RANGE_STEPS), 0);
  // Move to desired location
  if (abs(newX - curX) == abs(newY - curY)) {
    // If perfectly diagonal motion, just move diagonally
    if (newX - curX == newY - curY) { // If same sign
      int overshootOffset = overshoot * OVERSHOOT_STEPS * (newX - curX > 0 ? 1 : -1);
      overshootOffset = max(min(overshootOffset + newX, X_RANGE_STEPS), 0) - newX;
      moveSteps(MAIN_DIAG, 2*(newX - curX + overshootOffset), speedDelay);
      moveSteps(MAIN_DIAG, -2*overshootOffset, speedDelay);
    } else { // Opposite signs
      int overshootOffset = overshoot * OVERSHOOT_STEPS * (newX - curX > 0 ? 1 : -1);
      overshootOffset = max(min(overshootOffset + newX, X_RANGE_STEPS), 0) - newX;
      moveSteps(ANTI_DIAG, 2*(newX - curX + overshootOffset), speedDelay);
      moveSteps(ANTI_DIAG, -2*overshootOffset, speedDelay);
    }
  } else {
    // Otherwise move in X and Y separately, Manhattan-distance.
    // Check for special cases (knight movement and capturing) that require extra
    // offsets to move halfway between squares and avoid collisions.
    if (isKnight) {
      // If knight moving further right/left than up/down
      if (abs(newX - curX) > abs(newY - curY)) {
        moveSteps(Y, (newY - curY) / 2, speedDelay);
        moveSteps(X, newX - curX, speedDelay);
        int overshootOffset = overshoot * OVERSHOOT_STEPS * (newY - curY > 0 ? 1 : -1);
        moveSteps(Y, (newY - curY) / 2 + overshootOffset, speedDelay);
        moveSteps(Y, -overshootOffset, speedDelay);
      } else {
        moveSteps(X, (newX - curX) / 2, speedDelay);
        moveSteps(Y, newY - curY, speedDelay);
        int overshootOffset = overshoot * OVERSHOOT_STEPS * (newX - curX > 0 ? 1 : -1);
        overshootOffset = max(min(overshootOffset + newX, X_RANGE_STEPS), 0) - newX;
        moveSteps(X, (newX - curX) / 2 + overshootOffset, speedDelay);
        moveSteps(X, -overshootOffset, speedDelay);
      }
    } else if (isCapture) {
      // If on right half of board, offset half a square to left, otherwise to right.
      int offsetSign = (curX > X_RANGE_STEPS / 2) ? -1 : 1;
      moveSteps(X, newX - curX + (offsetSign * 1140 / 2), speedDelay);
      moveSteps(Y, newY - curY, speedDelay);
      moveSteps(X, -(offsetSign * 1140 / 2), speedDelay);
    } else {
      moveSteps(X, newX - curX, speedDelay);
      int overshootOffset = overshoot * OVERSHOOT_STEPS * (newY - curY > 0 ? 1 : -1);
      moveSteps(Y, newY - curY + overshootOffset, speedDelay);
      moveSteps(Y, -overshootOffset, speedDelay);
    }
  }
  // Update current position
  curX = newX;
  curY = newY;
}

void moveSteps(AXIS axis, int steps, int speedDelay) {
  // Set stepper motor directions.
  // See explanation here: http://corexy.com/theory.html
  // Note that our A and B motors are switched,
  // and setting direction HIGH represents negative direction,
  // compared to above reference.
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
    delayMicroseconds(speedDelay);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delayMicroseconds(speedDelay);
  }
}

void moveToSquare(int col, int row, int speedDelay, bool isKnight, bool isCapture, bool overshoot) {
  // Calibrated empirically according to square spacing and offsets on board
  moveTo(col*1140-100,
         (7-row)*1140+376, /* approximately (0.9-0.04)/7.0*Y_RANGE_STEPS */
         speedDelay, isKnight, isCapture, overshoot);
}

void copyBoard(char fromBoard[8][8], char toBoard[8][8]) {
  memcpy(toBoard, fromBoard, sizeof(fromBoard[0][0])*64);
}

void scanBoardChanges() {
  Serial.println("scanning");
  BoardMove move = {0, 0, -1, -1, -1, -1};
  
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (currentBoard[r][c] == newBoard[r][c]) {}
      else if (newBoard[r][c] == '.') {
        // A piece moved away from this square
        if (move.piece > 0 && move.piece != currentBoard[r][c]) {
          // We were previously tracking a different piece's move.
          // Error, there should be only be one change on every call.
          Serial.println("error 1");
          zero(); // Indicate error state by zeroing robot
          return;
        }
        move.piece = currentBoard[r][c];
        move.fromRow = r;
        move.fromCol = c;
      } else {
        // A piece moved to this square, possibly capturing a piece
        if (move.piece > 0 && move.piece != newBoard[r][c]) {
          // We were previously tracking a different piece's move.
          // Error, there should be only be one change on every call.
          Serial.println("error 2");
          zero(); // Indicate error state by zeroing robot
          return;
        }
        move.piece = newBoard[r][c];
        move.toRow = r;
        move.toCol = c;
        if (currentBoard[r][c] != '.') {
          // The destination square was occupied, the piece is captured
          move.pieceCaptured = currentBoard[r][c];
        }
      }
    }
  }

  Serial.println("verifying");
  // Verify a valid move has been detected
  if (move.piece == 0
      || move.fromRow < 0 || move.fromCol < 0
      || move.toRow < 0 || move.toCol < 0)
    return;
  
  Serial.print(move.piece);
  Serial.print(move.pieceCaptured);
  Serial.print(move.fromRow);
  Serial.print(move.fromCol);
  Serial.print(move.toRow);
  Serial.println(move.toCol);

  Serial.println("moving");
  if (move.pieceCaptured != 0) {
    moveToSquare(move.toCol, move.toRow, speedMicros/2, false, false, false);
    delay(100);
    digitalWrite(magnet, HIGH);
    moveTo(curX, Y_RANGE_STEPS, speedMicros*2, false, true, false); // Go to top of board
    moveTo(X_RANGE_STEPS, Y_RANGE_STEPS, speedMicros*2, false, false, false); // Top right corner
    delay(100);
    digitalWrite(magnet, LOW);
  }
  moveToSquare(move.fromCol, move.fromRow, speedMicros/2, false, false, false);
  delay(100);
  digitalWrite(magnet, HIGH);
  delay(100);
  moveToSquare(move.toCol, move.toRow, speedMicros*2,
      move.piece == 'n' || move.piece == 'N', false, true);
  delay(100);
  digitalWrite(magnet, LOW);
  delay(100);

  // Update current board buffer
  copyBoard(newBoard, currentBoard);
}