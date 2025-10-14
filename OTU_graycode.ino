#include <Nextion.h>
#include <AccelStepper.h>
#include <avr/wdt.h>

//STEPPER PINS-------------------------------------------------------------------
#define PUL_PIN 2
#define DIR_PIN 5
#define WATER_PIN 7

//INITIALIZE STEPPER-------------------------------------------------------------
AccelStepper OTUStep(1,PUL_PIN,DIR_PIN);

//Encoder Pins-------------------------------------------------------------------
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

//NEXTION INPUT GLOBALS---------------------------------------------------------
int jogStatus;
int degPerSec;
int startStatus;
int stopStatus = 0;
int angle;
int minCnt;
int secCnt;
long fullInterval;
long oscInterval;

//STEPPER AND ENCODER CONFIGURATION-------------------------------------------
#define ENCODER_MAX 1023
#define HOME 0          // For Homing function, Can be changed later
#define TOLERANCE 0 //For Homing function, Can be changed later
#define GEARFACTOR 30
#define STEPS_PER_REV 200
const float stepsPerDegree = STEPS_PER_REV * GEARFACTOR / 360.0;
float degPerStep = 1/stepsPerDegree;

//GLOBALS FOR OSCILATTION CALCULATIONS----------------------------------------
float destPt;
int stepsToMove;
//speed when "jog" is pushed, adjust if necessary
#define jogSPD 500
long startTime = 0;
long prevTime = 0;

//FUNCTION DEFINITIONS--------------------------------------------------------

//STOP Function
void stopMotor();

//Functions for JOG Calls
void jogForward();
void jogReverse();
void jogOFF();



//Conversion Functions
int rpmToSPS(int rpm);
void setRPM(int rpm);
float setDPS(float dps);
unsigned int grayToBinary(unsigned int gray);

//Homing Function
void returnHome(int pos);
//encoder function
int readEncoder();

//Reset/Emergency Stop functions
void resetMotor();
void checkStop();
void timerExp();


void setup() {
  //Serial comm initialization
	Serial.begin(9600);
  Serial3.begin(9600);
  delay(500);
  Serial.println("Start initialization");

  //Encoder Initialization
  unsigned int gray = 0;
  for (int i = 0; i < 10; i++) {
    int raw = digitalRead(inputPins[i]);
    int bitVal = (raw == LOW) ? 1 : 0;
    gray |= (bitVal << (9 - i)); // MSB first, with 10 bits (0-9)
  }

  //Set relay Pin as Output
  pinMode(WATER_PIN, OUTPUT);

  //Max Speed/accel for motor
  OTUStep.setMaxSpeed(1000);
  OTUStep.setAcceleration(1666.67);
  //begin at pos "0"
  OTUStep.setCurrentPosition(0); 

  //initialize global vars
  jogStatus = 0;
  degPerSec = 0;
  startStatus = 0;
  angle = 0;
  minCnt = 0;
  secCnt = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  //CHeck for serial Input
  if(nexSerial.available()) {
    // Read until Termination Char, q
    String inp = nexSerial.readStringUntil('q');

        //"RPM" signifies start push
    if (inp.indexOf("RPM") != -1) {
      //concatenate to termination char
      String data = inp.substring(inp.indexOf("RPM:"), inp.indexOf('q'));//string from RPM statement to termination char
      //Parse Data to find out which test to run
      parseData(data);  
    }

    //Check for JOG
    if (inp.indexOf("JOG0") != -1) jogOFF();
    if (inp.indexOf("JOGF") != -1) jogForward();
    if (inp.indexOf("JOGR") != -1) jogReverse();

    //Return to "Home"
    if (inp.indexOf("ret") != -1){
      int pos = readEncoder();
      if(pos!=HOME){
        returnHome(pos);
      }
    }

    //Relay High/low for water on/off
    if (inp.indexOf("relay1") != -1) digitalWrite(WATER_PIN, HIGH);
    if (inp.indexOf("relay0") != -1) digitalWrite(WATER_PIN, LOW);

    //Check For STOP
    if(inp.indexOf("STOP") != -1){
      stopMotor();
    }
    //Clear Serial input
    inp = "";
  }

  //JOG Call
  //CW/CCW
  if(jogStatus==1){
    OTUStep.run();
  }

  //Prepare for test
  if(startStatus==1) 
  {
    OTUStep.setCurrentPosition(0);
    startStatus = 2;
    Serial.print("ready\n");
  } 

  //Begin Test
  if(startStatus==2){
    //Water ON
    digitalWrite(WATER_PIN, HIGH);  // Turn relay ON
    delay(250);
    //Begin First Oscillation
    //Move to Destination given by angle
    if(!stopStatus)OTUStep.moveTo(stepsToMove);
    while(OTUStep.distanceToGo()!=0){
      //Check for Stop Press
      if(nexSerial.available()) checkStop();
      else OTUStep.run();

      //check for timer expiration
      if(millis()-startTime>=fullInterval){
        timerExp();
        break;
      }  
    } 
    //Reverse Side of Oscillation
    if(!stopStatus) OTUStep.move(-stepsToMove*2); 
    delay(250);
    while(OTUStep.distanceToGo()!=0){
      //Check for Stop Press
      if(nexSerial.available()) checkStop();
      else OTUStep.run();

        //check for timer expiration
      if(millis()-startTime>=fullInterval){
        timerExp();
        break;
      }
    }
    //Check for Timer Expiration
    if(millis()-startTime>=fullInterval){
      timerExp();
    }
  }
}

void parseData(String data) {
  Serial.print("Parsing\n");
  // Handle data in the format "RPM: XX ANG: YY"

  Serial.println(data);
  Serial.print("\nIndexing\n");
  int rpmStartIndex = data.indexOf("RPM:") + 5; // Position after "RPM: "
  int rpmEndIndex = data.indexOf(" ", rpmStartIndex); // Space after RPM value
  int angStartIndex = data.indexOf("ANG:") + 5; // Position after "ANG: "
  int minIdx = data.indexOf("MIN: ") + 5;
  int secIdx = data.indexOf("SEC: ") + 5;
    
  // Extract and convert multi-character degPerSec (RPM value)
  String degPerSecStr = data.substring(rpmStartIndex, angStartIndex); 
  degPerSec = degPerSecStr.toInt();   // Convert to integer

  // Extract and convert multi-character angle value
  String angleStr = data.substring(angStartIndex, minIdx);
  angle = angleStr.toInt();  

  // Extract and convert minCnt (Minutes)
  String minCntStr = data.substring(minIdx, secIdx);
  minCnt = minCntStr.toInt();  

  // Extract and convert secCnt (Seconds)
  String secCntStr = data.substring(secIdx, 'q');  // Assuming secCnt is the last value
  secCnt = secCntStr.toInt();  

  // Compute total time in milliseconds
  long totSec = (long)minCnt * 60 + (long)secCnt;
  fullInterval = totSec * 1000;

  // Ensure angle remains in the correct range
  if(angle < 0) angle += 256;

  // Calculate time interval for oscillation (avoiding division by zero)
  if (degPerSec != 0) {
      oscInterval = (long)(angle / (float)degPerSec) * 1000;
  } else {
      oscInterval = 0; // Prevent division by zero errors
  }

  // Calculate destination point
  destPt = (float)angle / degPerStep;
  stepsToMove = round(destPt);

   
  startStatus = 1;
  stopStatus = 0;
  startTime = millis();
  prevTime = startTime;
  jogStatus = 0;

}

void jogForward(){
  OTUStep.stop();
  Serial.print("JOG ON\n");
  //setRPM(6);
  jogStatus = 1;
  OTUStep.setMaxSpeed(jogSPD);
  OTUStep.move(10000000);
}

void jogReverse() {
  OTUStep.stop();
  jogStatus = 1;
  OTUStep.setAcceleration(1600);
  OTUStep.setMaxSpeed(jogSPD);
  OTUStep.move(-1000000); // Move backward a long distance
}

void jogOFF(){
  jogStatus = 0;
  OTUStep.stop(); while(OTUStep.run()); // Smooth deceleration
}

int rpmToSPS(int RPM){
  return RPM * STEPS_PER_REV/60;
}

void setRPM(int RPM){
  int SPS = rpmToSPS(RPM);
  OTUStep.setSpeed(SPS);
  Serial.print("Set speed to ");
  Serial.print(RPM);
  Serial.println(" RPM\n");
}

float setDPS(float dps) {
    float SPS = dps * STEPS_PER_REV * GEARFACTOR / 360.0;
    Serial.print("Set speed to ");
    Serial.print(dps);
    Serial.println(" Degrees per second\n");
    return SPS;
}


void resetMotor(){
  Serial3.print("rest");   // Send reset command to Nextion
  Serial3.write(0xFF);     // End of Nextion command
  Serial3.write(0xFF);
  Serial3.write(0xFF);
  delay(500);            // Small delay before reset
  wdt_enable(WDTO_250MS); // Enable WDT (250ms timeout)
  while (1);             // Wait for reset
}

void checkStop(){
  String input = nexSerial.readStringUntil('q'); // Read until 'q' character, 0x71
    if(input.indexOf("STOP") != -1){
      OTUStep.stop();while(OTUStep.run());
      startStatus = 0;
      stopStatus = 1;
      jogStatus = 0;
      setDPS(0);
      OTUStep.setCurrentPosition(0);
      digitalWrite(WATER_PIN, LOW);   // Turn relay OFF
      resetMotor();
    }
}

int readEncoder(){
  unsigned int gray = 0;
  for (int i = 0; i < 10; i++) {
    int raw = digitalRead(inputPins[i]);
    int bitVal = (raw == LOW) ? 1 : 0;
    gray |= (bitVal << (9 - i)); // MSB first, with 10 bits (0-9)
  }

  unsigned int binary = grayToBinary(gray);

  return binary; // Force it into 0–99 range
}

void returnHome(int pos) {
  int dir;
  if(abs(HOME-pos+ENCODER_MAX)%ENCODER_MAX>abs(pos-HOME+ENCODER_MAX)%ENCODER_MAX)dir=1;else dir=-1;
  //const int ENCODER_MAX = 100;
  const float homeSpeed = 800.0;  // degrees/sec or steps/sec depending on your setup

 // OTUStep.setMaxSpeed(homeSpeed*dir);
  OTUStep.move(dir*10000000);
  while(readEncoder()!=HOME){
    OTUStep.run();
    if(readEncoder==HOME)stopMotor();
  }
}

void stopMotor(){
  OTUStep.stop();while(OTUStep.run());
  startStatus = 0;
  stopStatus = 1;
  jogStatus = 0;
}

void timerExp(){
  startStatus = 0;
  stopStatus = 1;
  OTUStep.stop();while(OTUStep.run());
  digitalWrite(WATER_PIN, LOW);   // Turn relay OFF
  resetMotor();
}

// Convert n-bit Gray code to Binary
unsigned int grayToBinary(unsigned int gray) {
  unsigned int binary = gray;
  while (gray >>= 1) {
    binary ^= gray;
  }
  return binary;
}