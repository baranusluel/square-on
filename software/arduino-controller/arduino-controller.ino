/// CONSTANTS
// ROM of robot in steps
#define X_RANGE_STEPS 1900
#define Y_RANGE_STEPS 2350
// Speed of robot, as delay between steps
#define speedMicros 500

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

// Axis symbols
enum AXIS { X, Y };

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
void moveTo(int newX, int newY);
void moveSteps(AXIS axis, int steps);
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
      if (cmd == "new") {
        Serial.println("new ack");
        // Assume player manually resets pieces
        copyBoard(initialBoard, currentBoard);
      } else if (cmd == "board") {
        Serial.println("board ack");
        // Read new board state into buffer
        for (int r = 0; r < 8; r++) {
          while (!Serial.available()) {}
          String row = Serial.readStringUntil('\n');
          for (int c = 0; c < 8; c++) {
            newBoard[r][c] = row[c*2];
          }
        }
        // Identify different between current and new board,
        // make the necessary chess move.
        scanBoardChanges();
      } else {
        Serial.println("what?");
      }
      
//      char r = Serial.read(); // prefix
//      if (r == 'm') { // magnet on/off
//        int on = Serial.parseInt();
//        if (Serial.read() != '\n') continue; // valid EOL
//        digitalWrite(magnet, on);
//      } else if (r == 'x') { // coordinate
//        float x = Serial.parseFloat();
//        float y = Serial.parseFloat();
//        if (Serial.read() != '\n') continue; // valid EOL
//        moveTo(x*X_RANGE_STEPS, y*Y_RANGE_STEPS);
//      }
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
    delayMicroseconds(speedMicros);
    digitalWrite(stepA, LOW);
    digitalWrite(stepB, LOW);
    delayMicroseconds(speedMicros);
  }
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
  
  // TODO: Make move
  Serial.print(move.piece);
  Serial.print(move.pieceCaptured);
  Serial.print(move.fromRow);
  Serial.print(move.fromCol);
  Serial.print(move.toRow);
  Serial.println(move.toCol);

  // Update current board buffer
  copyBoard(newBoard, currentBoard);
}

