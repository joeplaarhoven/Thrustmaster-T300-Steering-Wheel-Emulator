void resetWheelState() {
  // Default Wheel State for the Thrustmaster F1
  wheelState[0] = B11001111;
  wheelState[1] = B11111111;
  wheelState[2] = B11111111;
  wheelState[3] = B11111111;
  wheelState[4] = B00000000;
  wheelState[5] = B00000000;
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  buttonValue = 0;
}

//
// We scan the Button matrix
void scanButtonMatrix() {
  for (int rowCounter = 0; rowCounter < rowSize; rowCounter++) {
    // Set the active rowPin to LOW
    digitalWrite(rowPin[rowCounter], LOW);
    // Then we scan all cols in the col[] array
    for (int colCounter = 0; colCounter < colSize; colCounter++) {
      // Scan each pin in column and if it's LOW
      if (digitalRead(colPin[colCounter]) == LOW) {
        // Using a Cantor Pairing function we create unique number
        buttonValue = (((rowPin[rowCounter] + colPin[colCounter]) * (rowPin[rowCounter] + colPin[colCounter] + 1)) / 2 + colPin[colCounter]);
      }
    }
    // Set the active rowPin back to HIGH
    digitalWrite(rowPin[rowCounter], HIGH);
  }
}


void encoderCAB_Decrement(i2cEncoderLibV2* obj) {
  buttonValue = 1000;
}
void encoderCAB_Increment(i2cEncoderLibV2* obj) {
  buttonValue = 1100;
}
void encoderTC_Decrement(i2cEncoderLibV2* obj) {
  buttonValue = 3000;
}
void encoderTC_Increment(i2cEncoderLibV2* obj) {
  buttonValue = 3100;
}
void encoderABS_Decrement(i2cEncoderLibV2* obj) {
  buttonValue = 2000;
}
void encoderABS_Increment(i2cEncoderLibV2* obj) {
  buttonValue = 2100;
}

// Interrupt0 (external, pin 2) - prepare to start the transfer
void ss_rising () {
  SPDR = wheelState[0]; // load first byte into SPI data register
  pos = 1;
}
