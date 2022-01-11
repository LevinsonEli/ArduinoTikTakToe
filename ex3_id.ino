
int DIN=9;
int CS=4;
int CLK=8;
int P_BTN = 2;
int S_BTN = 3;

// States for P/S buttons
int P_BtnState, S_BtnState;
// Time when P/S button was clicked last
unsigned long timeSwasClicked = 0, timePwasClicked = 0;

// clmnArr - is a (binary) number of the column to latch now (8 bits)
bool clmnArr[8] = {0};
// bitsArr - it's 8 bits to latch for the column
bool bitsArr[8] = {1};

// gameArr's cells have 5 states: 0 - empty, 1 - P1, 2 - P2,
// 3 - P1 blinking, 4 - P2 blinking
int gameArr[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
// matrixArr - game array that latched
bool matrixArr[8][8];

// matrix for latching "P1"
bool matrixP1[8][8] = 
  {
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 0, 0, 0, 0, 0, 0}
  };
// matrix for latching "P2"
bool matrixP2[8][8] = 
  {
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 0, 0, 1},
    {0, 1, 0, 0, 1, 0, 0, 1},
    {0, 1, 0, 0, 1, 0, 0, 1},
    {0, 1, 0, 0, 1, 1, 1, 1}
  };
// matrix for latching "win"
bool matrixWin[8][8] = 
  {
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 1},
    {0, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 1}
  };
// matrix for latching "tie"
bool matrixTie[8][8] = 
  {
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 1, 1, 0, 0, 0, 0},
    {1, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 1}
  };

// player that doing his move now - 1 or 2
int playerToMove = 1;
// does the player makes move now
bool playerMakesMove = false;

// Counter for empty cells that left
int emptyCells = 9;
// To disable buttons for clear handling the output
bool btnsActive = true;

void setup() {
  pinMode(DIN,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(CLK,OUTPUT);
  pinMode(P_BTN, INPUT_PULLUP);
  pinMode(S_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(P_BTN), handleP_BtnClick, HIGH);
  attachInterrupt(digitalPinToInterrupt(S_BTN), handleS_BtnClick, HIGH);
  Serial.begin(9600);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      bitsArr[j] = 1;
    }
    updateClmn(i + 1);
  }

//  Set registers: decode mode, scan limit, and shutdown (0x900, 0xB07, 0xC01)
  for (int i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  for (int i=0; i<2; i++) writeBit(LOW);
  writeBit(HIGH);
  for (int i=0; i<8; i++) writeBit(LOW);
  latchBuf();  
  for (int i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  writeBit(LOW);
  writeBit(HIGH);
  writeBit(HIGH);
  for (int i=0; i<5; i++) writeBit(LOW);
  for (int i=0; i<3; i++) writeBit(HIGH);
  latchBuf();  
  for (int i=0; i<4; i++) writeBit(LOW);
  for (int i=0; i<2; i++) writeBit(HIGH);
  for (int i=0; i<2; i++) writeBit(LOW);
  for (int i=0; i<7; i++) writeBit(LOW);
  writeBit(HIGH);
  latchBuf();
}

void loop() {
  setGameMatrixAndDisplay();
  handleWinner();
  handleStartMove();

  P_BtnState = !digitalRead(P_BTN);
  S_BtnState = !digitalRead(S_BTN);
}

// Function checks if it's end of the game and does an appropriate output
void handleWinner() {
  btnsActive = false;   // disable buttons to handle data correctly
  
  // If there is no empty cell
  if (emptyCells == 0)
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (gameArr[i][j] == 3 || gameArr[i][j] == 4) // turn on the blinking cell
          gameArr[i][j] -=2;
      }
    }
    
  // 3 cells horizontally
  handleWinnerHorizontally();
  // 3 cells vertically
  handleWinnerVertically();
  // 3 cells diagonally
  handleWinnerDiagonally();
  // If there is no winner
  if (emptyCells == 0) {
    setEndOfGame(0);
    return;
  }
  
  btnsActive = true;
}

// Function that displays P1/P2 and sets blinking P1/P2 cell to gameArr
void handleStartMove() {
  if (playerMakesMove == false) {
    if (playerToMove == 1)
      my_display(matrixP1);
    else if (playerToMove == 2)
      my_display(matrixP2);
    delay(1000);
    startCellForPLayer();
    playerMakesMove = true;
  }
}

// Function that checks if it's winning 3 horizontally and ends the game (if it is)
void handleWinnerHorizontally() {
  for (int i = 0; i < 3; i++)
    if (gameArr[i][0] == gameArr[i][1] && gameArr[i][0] == gameArr[i][2] && gameArr[i][0] != 0) {
      gameArr[i][0] = gameArr[i][1] = gameArr[i][2] += 2;
      setEndOfGame(gameArr[i][0] - 2);
      return;
    }
}

// Function that checks if it's winning 3 vertically and ends the game (if it is)
void handleWinnerVertically() {
  for (int j = 0; j < 3; j++)
    if (gameArr[0][j] == gameArr[1][j] && gameArr[0][j] == gameArr[2][j] && gameArr[0][j] != 0) {
      gameArr[0][j] = gameArr[1][j] = gameArr[2][j] += 2;
      setEndOfGame(gameArr[0][j] - 2);
      return;
    }
}

// Function that checks if it's winning 3 diagonally and ends the game (if it is)
void handleWinnerDiagonally() {
  if (gameArr[0][0] == gameArr[1][1] && gameArr[0][0] == gameArr[2][2] && gameArr[0][0] != 0) {
    gameArr[0][0] = gameArr[1][1] = gameArr[2][2] += 2;
    setEndOfGame(gameArr[0][0] - 2);
    return;
  }
  if (gameArr[0][2] == gameArr[1][1] && gameArr[0][2] == gameArr[2][0] && gameArr[0][2] != 0) {
    gameArr[0][2] = gameArr[1][1] = gameArr[2][0] += 2;
    setEndOfGame(gameArr[0][2] - 2);
    return;
  }
}

// Function that displays the winning P1/P2 or "tie" and resets the game
void setEndOfGame(int playerWon) {
  if (playerWon == 0) {   // If noone won
    emptyGameArr();
    for (int k = 0; k < 3; k++) {
      setMatrixArr();
      my_display(matrixArr);   delay(1000);
      my_display(matrixTie);   delay(1000);
    }
  }
  else {
    for (int k = 0; k < 4; k++)
      setGameMatrixAndDisplay();
    for (int k = 0; k < 3; k++){
      if (playerWon == 1)
        my_display(matrixP1);
      else if (playerWon == 2)
        my_display(matrixP2);
      delay(1000);   my_display(matrixWin);   delay(1000);
    }
  }
  emptyGameArr();   // reset the game resources
  emptyCells = 9;
  playerToMove = 1;
  playerMakesMove = false;
  btnsActive = true;
}

// Fanction that resets the gameArr
void emptyGameArr() {
  for (int i = 0; i < 3; i++) 
    for (int j = 0; j < 3; j++) 
      gameArr[i][j] = 0;
}

// Function that sets blinking P1/P2 to first empty cell
void startCellForPLayer() {
  btnsActive = false;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gameArr[i][j] == 0) {
        gameArr[i][j] = playerToMove + 2;
        emptyCells--;
        btnsActive = true;
        return;
      }
    }
  }
  btnsActive = true;
}

// Function that moves blinking P1/P2 to the next empty cell
void moveToNextEmptyCell() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gameArr[i][j] == 3 || gameArr[i][j] == 4)
      {
        int k = i, l = j;
        do {
          l++;
          if (l == 3){
            l = 0;
            k++;
          }
          if (k == 3)
            k = 0;
          if (gameArr[k][l] == 0)
            break;
        } while (k >=0 && k < 3 && l >= 0 && l < 3);
        int temp = gameArr[i][j];
        gameArr[i][j] = gameArr[k][l];
        gameArr[k][l] = temp;
        return;
      }
    }
  }
}

// Function that sets blinking cell to turned on cell
void setTheCell() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gameArr[i][j] == 3 || gameArr[i][j] == 4)
      {
        gameArr[i][j] -= 2;
        return;
      }
    }
  }
}

// Sets disaplaying matrix acording to gameArr
void setMatrixArr() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      switch(gameArr[i][j]) {
        case 1: case 3:
          matrixArr[j*3][7 - i*3] = HIGH;
          matrixArr[j*3][6 - i*3] = LOW;
          matrixArr[j*3 + 1][7 - i*3] = HIGH;
          matrixArr[j*3 + 1][6 - i*3] = HIGH;
          break;
        case 2: case 4:
          matrixArr[j*3][7 - i*3] = LOW;
          matrixArr[j*3][6 - i*3] = HIGH;
          matrixArr[j*3 + 1][7 - i*3] = HIGH;
          matrixArr[j*3 + 1][6 - i*3] = LOW;
          break;
        default:
          matrixArr[j*3][7 - i*3] = LOW;
          matrixArr[j*3][6 - i*3] = LOW;
          matrixArr[j*3 + 1][7 - i*3] = LOW;
          matrixArr[j*3 + 1][6 - i*3] = LOW;
          break;
      }
    }
  }
}

// Turns of blinking cells of displaying matrix (for blinking)
void setMatrixArrBlink() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      switch(gameArr[i][j]) {
        case 3: case 4:
          matrixArr[j*3][7 - i*3] = LOW;
          matrixArr[j*3][6 - i*3] = LOW;
          matrixArr[j*3 + 1][7 - i*3] = LOW;
          matrixArr[j*3 + 1][6 - i*3] = LOW;
          break;
      }
    }
  }
}

// Handle clickes on 'P' button
void handleP_BtnClick(){
  unsigned long thisTime = millis();
  if (thisTime - timePwasClicked >= 300 && btnsActive){
    btnsActive = false;
    timePwasClicked = thisTime;
    moveToNextEmptyCell();
    btnsActive = true;
  }
}

// Handle clickes on 'S' button
void handleS_BtnClick(){
  unsigned long thisTime = millis();
  if (thisTime - timeSwasClicked >= 300 && btnsActive){
    btnsActive = false;
    timeSwasClicked = thisTime;
    
    setTheCell();
    playerToMove = (playerToMove == 2) ? 1 : 2;
    playerMakesMove = false;
    btnsActive = true;
  }
}

// Latches the given matrix
void my_display(bool matrix[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      bitsArr[j] = matrix[i][j];
    }
    updateClmn(i + 1);
  }
}

// Turns a given number to binary 8 bits and sets it to clmnArr
void clmnNumToBitArr(int num) {
  for(int i = 0; i < 8; i++) clmnArr[i] = 0;
  if (num > 256 || num < 0)
    return;
  int i = 7;
  while(i >= 0 && num !=0 ) {
    clmnArr[i] = num%2;
    num /= 2;
    i--;
  }
}

// Latches a column
void updateClmn(int clmnNum) {
  clmnNumToBitArr(clmnNum);
  for (int i = 0; i < 8; i++) writeBit(clmnArr[i]);
  for (int i = 0; i < 8; i++) writeBit(bitsArr[i]);
  latchBuf();
}

// Sets the displaying matrix according to the gameArr and latches it
void setGameMatrixAndDisplay() {
  setMatrixArr();
  my_display(matrixArr);
  delay(500);
  
  setMatrixArrBlink();
  my_display(matrixArr);
  delay(500);
}

void writeBit(bool b) // Write 1 bit to the buffer
{
  digitalWrite(DIN,b);
  digitalWrite(CLK,LOW);
  digitalWrite(CLK,HIGH);
  //Serial.print(b);
  //Serial.print(".");
}
void latchBuf() // Latch the entire buffer
{
  digitalWrite(CS,LOW);
  digitalWrite(CS,HIGH);
  //Serial.println();
}
