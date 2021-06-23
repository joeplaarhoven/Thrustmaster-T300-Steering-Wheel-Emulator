#include      <EEPROM.h>                              // Load EEPROM library for storing settings
#include      <Wire.h>
#include      <i2cEncoderLibV2.h>

//
// Debugging & Setup
#define       DEBUG_SETUP true                       // Debug Setup information
#define       DEBUG_KEYS false                        // Debug the button presses
#define       DEBUG_WHEEL true                       // Debug wheel output
#define       DEBUG_ROTARY_SWITCHES false             // Debug Rotary Switches: Display the values returned by the Rotary Switches
#define       DEBUG_LATENCY false                     // Debug response
#define       Rotary_Switch_T300 false                 // Select the values for the Rotary Switches. 'true:T300', 'false:USB'
#define       DEBOUNCE 90                             // Set this to the lowest value that gives the best result
#define       DEBUG_ENCODERS true
#define       WHEELMODE true

const int intPin = 3; // INT pins, change according to your board

i2cEncoderLibV2 encoderCAB = i2cEncoderLibV2(0x7);
i2cEncoderLibV2 encoderTC = i2cEncoderLibV2(0x28);
i2cEncoderLibV2 encoderABS = i2cEncoderLibV2(0x46);

uint8_t       encoder_status, i;
int           encoderIncCount = 0;
int           encoderDecCount = 0;

unsigned long startLoop;

//
// Button Matrix
const int     MATRIX_INTERRUPT_PIN = 2;
int           foundColumn = 0;
int           buttonValue = 0;
byte          wheelState[8];
volatile byte pos;

//
// Setup Button Matrix
int           rowPin[] = {5, 6, 7, 8, 9};                   // Set pins for rows > OUTPUT
int           colPin[] = {A0, A1, A2, A3, 11};              // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int           rowSize = sizeof(rowPin) / sizeof(rowPin[0]);
int           colSize = sizeof(colPin) / sizeof(colPin[0]);



void setup() {
  resetWheelState();
  Serial.begin(115200);

  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(MATRIX_INTERRUPT_PIN), ss_rising, RISING); // Interrupt for Button Matrix


  #if DEBUG_SETUP
    Serial.println("Steering Wheel Emulator");
    Serial.println();
    Serial.print("Setup ");
    Serial.print(rowSize);
    Serial.print("x");
    Serial.print(colSize);
    Serial.println(" Button Matrix");
  #endif

  Wire.begin();


  // Setup rotary encoder pin
  pinMode(intPin, INPUT_PULLUP);
  encoderCAB.begin(
    i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
    | i2cEncoderLibV2::DIRE_RIGHT
    | i2cEncoderLibV2::IPUP_ENABLE
    | i2cEncoderLibV2::RMOD_X1
    | i2cEncoderLibV2::RGB_ENCODER);
  encoderTC.begin(
    i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
    | i2cEncoderLibV2::DIRE_RIGHT
    | i2cEncoderLibV2::IPUP_ENABLE
    | i2cEncoderLibV2::RMOD_X1
    | i2cEncoderLibV2::RGB_ENCODER);
  encoderABS.begin(
    i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
    | i2cEncoderLibV2::DIRE_RIGHT
    | i2cEncoderLibV2::IPUP_ENABLE
    | i2cEncoderLibV2::RMOD_X1
    | i2cEncoderLibV2::RGB_ENCODER);

  encoderCAB.writeCounter((int32_t) 0); //Reset of the CVAL register
  encoderCAB.writeMax((int32_t) 12); //Set the maximum threshold to 50
  encoderCAB.writeMin((int32_t) 0); //Set the minimum threshold to 0
  encoderCAB.writeStep((int32_t) 1); //The step at every encoder click is 1
  encoderCAB.writeAntibouncingPeriod(25); //250ms of debouncing
  encoderCAB.writeDoublePushPeriod(0); //Set the double push period to 500ms

  /* Configure the events */
  encoderCAB.onIncrement = encoderCAB_Increment;
  encoderCAB.onDecrement = encoderCAB_Decrement;

  /* Enable the I2C Encoder V2 interrupts according to the previus attached callback */
  encoderCAB.autoconfigInterrupt();


  encoderTC.writeCounter((int32_t) 0); //Reset of the CVAL register
  encoderTC.writeMax((int32_t) 12); //Set the maximum threshold to 50
  encoderTC.writeMin((int32_t) 0); //Set the minimum threshold to 0
  encoderTC.writeStep((int32_t) 1); //The step at every encoder click is 1
  encoderTC.writeAntibouncingPeriod(25); //250ms of debouncing
  encoderTC.writeDoublePushPeriod(0); //Set the double push period to 500ms

  /* Configure the events */
  encoderTC.onIncrement = encoderTC_Increment;
  encoderTC.onDecrement = encoderTC_Decrement;

  /* Enable the I2C Encoder V2 interrupts according to the previus attached callback */
  encoderTC.autoconfigInterrupt();


  encoderABS.writeCounter((int32_t) 0); //Reset of the CVAL register
  encoderABS.writeMax((int32_t) 12); //Set the maximum threshold to 50
  encoderABS.writeMin((int32_t) 0); //Set the minimum threshold to 0
  encoderABS.writeStep((int32_t) 1); //The step at every encoder click is 1
  encoderABS.writeAntibouncingPeriod(25); //250ms of debouncing
  encoderABS.writeDoublePushPeriod(0); //Set the double push period to 500ms

  /* Configure the events */
  encoderABS.onIncrement = encoderABS_Increment;
  encoderABS.onDecrement = encoderABS_Decrement;

  /* Enable the I2C Encoder V2 interrupts according to the previus attached callback */
  encoderABS.autoconfigInterrupt();

  //
  // Row setup
  // Set rows as OUTPUT
  for (int i = 0; i < rowSize; i++) {
    pinMode(rowPin[i], OUTPUT);
    #if DEBUG_SETUP
        Serial.print("Pin: ");
        Serial.print(rowPin[i]);
        Serial.print(" is set as OUTPUT on row: ");
        Serial.println(i);
    #endif
    digitalWrite(rowPin[i], HIGH);
  }

  //
  // Column setup
  // Set columns as INPUT_PULLUP
  for (int i = 0; i < colSize; i++) {
    pinMode(colPin[i], INPUT_PULLUP);
    #if DEBUG_SETUP
      Serial.print("Pin: ");
      Serial.print(colPin[i]);
      Serial.print(" is set as INPUT_PULLUP on col: ");
      Serial.println(i);
    #endif
  }
}

// SPI interrupt routine
ISR (SPI_STC_vect) {
  SPDR = wheelState[pos++]; // load the next byte to SPI output register and return.
}

void loop() {

  // put your main code here, to run repeatedly:
  scanButtonMatrix();

  // Set a debounce delay
  delay(DEBOUNCE);

  if (digitalRead(intPin) == LOW) {
    //Interrupt from the encoders, start to scan the encoder matrix
    encoderCAB.updateStatus();
    encoderTC.updateStatus();
    encoderABS.updateStatus();

  }
  #if DEBUG_KEYS
    if(buttonValue > 0){
      Serial.println(buttonValue);
    }
  #endif

  if(buttonValue == 147){
    wheelState[0] = wheelState[0] & B11110111;
    #if DEBUG_KEYS
      Serial.print("L1 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 164){
    wheelState[0] = wheelState[0] & B11111011;
    #if DEBUG_KEYS
      Serial.print("R1 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 341){
    wheelState[0] = wheelState[0] & B11110110;
    #if DEBUG_KEYS
      Serial.print("- ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 368){
    wheelState[0] = wheelState[0] & B11111001;
    #if DEBUG_KEYS
      Serial.print("- ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 293){
    wheelState[1] = wheelState[1] & B11110111;
    #if DEBUG_KEYS
      Serial.print("R2 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 247){
    wheelState[2] = wheelState[2] & B11110111;
    #if DEBUG_KEYS
        Serial.print("Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
  }

  if(buttonValue == 316){
    wheelState[2] = wheelState[2] & B11011111;
    #if DEBUG_KEYS
      Serial.print("Right ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 246){
    wheelState[1] = wheelState[1] & B10111111;
    #if DEBUG_KEYS
      Serial.print("Cross ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 204){
    wheelState[0] = wheelState[0] & B11111101;
    #if DEBUG_KEYS
      Serial.print("Triangle ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 270){
    wheelState[1] = wheelState[1] & B11111011;
    #if DEBUG_KEYS
      Serial.print("L2 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 269){
    wheelState[2] = wheelState[2] & B10111111;
    #if DEBUG_KEYS
      Serial.print("Down ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 292){
    wheelState[2] = wheelState[2] & B11101111;
    #if DEBUG_KEYS
      Serial.print("Left ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 225){
    wheelState[0] = wheelState[0] & B01111111;
    #if DEBUG_KEYS
      Serial.print("Circle ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 224){
    wheelState[0] = wheelState[0] & B11111110;
    #if DEBUG_KEYS
      Serial.print("Square ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 201){
    wheelState[1] = wheelState[1] & B11111110;
    #if DEBUG_KEYS
      Serial.print("R3 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 182){
    wheelState[1] = wheelState[1] & B11111101;
    #if DEBUG_KEYS
      Serial.print("L3 ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 267){
   wheelState[1] = wheelState[1] & B11101111;
    #if DEBUG_KEYS
      Serial.print("Options ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 245){
   wheelState[1] = wheelState[1] & B11011111;
    #if DEBUG_KEYS
      Serial.print("Share ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 1000){
   wheelState[4] = wheelState[4] & B01111111;
    #if DEBUG_KEYS
      Serial.print("CAB increase ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 1100){
   wheelState[3] = wheelState[3] & B10111111;
    #if DEBUG_KEYS
      Serial.print("CAB decrease ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 2000){
   wheelState[3] = wheelState[3] & B11011111;
    #if DEBUG_KEYS
      Serial.print("TC increase ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 2100){
   wheelState[3] = wheelState[3] & B11101111;
    #if DEBUG_KEYS
      Serial.print("TC decrease ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 3000){
   wheelState[3] = wheelState[3] & B11110111;
    #if DEBUG_KEYS
      Serial.print("ABS increase ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  if(buttonValue == 3100){
   wheelState[3] = wheelState[3] & B11111011;
    #if DEBUG_KEYS
      Serial.print("ABS decrease ("); Serial.print(buttonValue); Serial.println(") ");
    #endif
  }

  #if DEBUG_WHEEL
    for (int i = 0; i < 8; i++) {
      Serial.print(wheelState[i], BIN);
      Serial.print(" ");
    }
    Serial.println();
  #endif

  delay(DEBOUNCE);

  resetWheelState();
}
