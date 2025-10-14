//Used For TRD-NA720NWD or TRD-NA1024NWD Encoders
//Convert Gray Code->Binary, Binary->Decimal
//Prints out each corresponding encoder value to serial monitor
#include <Nextion.h>
#include <AccelStepper.h>
#include <avr/wdt.h>



//STEPPER PINS-------------------------------------------------------------------
#define PUL_PIN 2
#define DIR_PIN 5
AccelStepper OTUStep(1,PUL_PIN,DIR_PIN);
#define GEARFACTOR 30
#define STEPS_PER_REV 200
#define PULSES_PER_REV 1024
const float stepsPerDegree = STEPS_PER_REV * GEARFACTOR / 360.0;
float degPerStep = 1/stepsPerDegree;
unsigned int binary;
int position;

#define BLK 22
#define RED 23
#define ORN 24
#define YLW 25
#define GRN 26
#define PRP 27
#define GRA 28
#define WHT 29
#define BLK_WHT 34
#define RED_WHT 36 // MSB
#define ORN_WHT 35

int inputPins[10] = {ORN_WHT, RED_WHT, BLK_WHT, WHT, GRA, PRP, GRN, YLW, ORN, RED};

unsigned int grayToBinary(unsigned int gray);
String toBinaryString(unsigned int value, int bits);
int readEncoder();



void setup() {
  Serial.begin(9600);
  Serial3.begin(9600);
  delay(500);
  Serial.println("Start initialization");
  for (int i = 0; i < 10; i++) {
    pinMode(inputPins[i], INPUT);
  }
}

void loop() {


  binary = readEncoder();
  position = binary;


  Serial.print("  Binary: "); Serial.print(toBinaryString(binary, 10));
  Serial.print("  Dec: "); Serial.print(position);
  Serial.print("  Angle: "); 
  Serial.println((position * 360.0) / PULSES_PER_REV, 2);
  delay(500);
}

// Convert n-bit Gray code to Binary
unsigned int grayToBinary(unsigned int gray) {
  unsigned int binary = gray;
  while (gray >>= 1) {
    binary ^= gray;
  }
  return binary;
}

// Helper: convert to fixed-width binary string
String toBinaryString(unsigned int value, int bits) {
  String s = "";
  for (int i = bits - 1; i >= 0; i--) {
    s += (value & (1 << i)) ? '1' : '0';
  }
  return s;
}

int readEncoder(){
  unsigned int gray = 0;
  for (int i = 0; i < 10; i++) {
    int raw = digitalRead(inputPins[i]);
    int bitVal = (raw == LOW) ? 1 : 0;
    gray |= (bitVal << (9 - i)); // MSB first, with 10 bits (0-9)
  }

  unsigned int binary = grayToBinary(gray);
  return binary; 
}

