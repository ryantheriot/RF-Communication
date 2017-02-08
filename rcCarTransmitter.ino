//////////////////////////////////////////////////////
// Property of Ryan Theriot. Not to be distributed. //
//                                                  //
// CONTROLS:                                        //
// key 'E' - Emergency Stop                         //
// key 'W' - Go Forward   (uses wads controls)      //
// key 'A' - Turn Left                              //
// key 'D' - Turn Right                             //
// key 'S' - Reverse                                //
//////////////////////////////////////////////////////


// Startup //
#include <Servo.h>
#include <SoftwareSerial.h>
#include <digitalWriteFast.h>
#include <SPI.h>
// end Startup//


//rf communication//
#define NETWORK_SIG_SIZE 3
#define VAL_SIZE         1
#define CHECKSUM_SIZE    1
#define PACKET_SIZE      (NETWORK_SIG_SIZE + VAL_SIZE + CHECKSUM_SIZE)
#define NET_ADDR 5        // The network address byte and can be change if you want to run different devices in proximity to each other without interfearance

const byte g_network_sig[NETWORK_SIG_SIZE] = {0x8F, 0xAA, NET_ADDR};  // Few bytes used to initiate a transfer
// end rf communications//


// Variable Declarations//
Servo servo;
int rfValue = 0;

boolean startUp = true;
boolean forward = false;      //boolean variable//
boolean reverse = false;      //boolean variable//  note: reverse is standard, one speed
boolean eBrake = true;
boolean power = false;
boolean blade = false;

unsigned char turnRight = 0;          //analog variable (+/-)// -- based on tilt angle (or duration of turn)
unsigned char turnLeft = 0;           //analog variable (+/-)// -- based on tilt angle (or duration of turn)
int avgSpeed = 0;        //analog variable (+ [0-10])// -- based on duration of button press 
int zero = 0; 

unsigned char turnSteps = 4;
unsigned int turnInterval = 0.2; 
unsigned char motor1Speed = 0;
const int motor1FullForwardValue = 127; // previously at 104; 
const int motor1ReverseValue = 34; //max 1, 63 is min
const int motor1StopValue = 64;
int motor1SpeedOld = motor1StopValue;    //set initial speed to zero

const int SABERBAUD = 9600;

int servoVal;

// note: 1 is full reverse, 64 is stop and 127 is full forward. Sending a character between 
//       128 and 255 will control motor 2. 128 is full reverse, 192 is stop and 255 is full forward.
// end Variable Declarations//


// Pin definitions//
const int SABERREC = 5;
const int SABERSEND = 6;
const int RFREC = 9;
const int HighPin10 = 10;      // rf transmitter needs its own +5V pin
const int RFSEND = 11;
int servoPin = 12; 
// end Pin definitions//


// Communication definitions//
SoftwareSerial SaberSerial = SoftwareSerial( SABERREC, SABERSEND );
SoftwareSerial RFSerial = SoftwareSerial( RFREC, RFSEND );
// end Communication definitions//


void setup() { 
  
  pinModeFast(SABERSEND, OUTPUT);
  pinModeFast(HighPin10, OUTPUT);
  pinModeFast(SABERREC, INPUT);
  SaberSerial.begin(SABERBAUD);
  delay(2000);     //allow enough time to initiate
  SaberSerial.write((byte) zero);   //initiate motor to zero before starting up  
  
  servo.attach(servoPin);    //PWM for servo 
  servoVal = 132;  // zero? --> PREVIOUSLY 90
  servo.write(servoVal);
  delay(15);

  pinModeFast(RFREC, INPUT);
  pinModeFast(RFSEND, OUTPUT);
  RFSerial.begin(1200);
  
  
  RFSerial.listen();
  
  
  digitalWrite(HighPin10, HIGH); // rf transmitter needs its own +5V pin
  
  Serial.begin(9600);    // Open serial communications and wait for port to open
  Serial.println("Welcome");
}


/////////////////////////////////
///// #region  - functions //////
/////////////////////////////////

void loop() { 
  
    
  
  
    //rf controls//  
    //RFSerial.listen();
    rfValue = readUInt(true);
    
    Serial.println("got data");
    Serial.println(rfValue);
    
    if (rfValue >= 0 && rfValue <= 4) {              // check for incoming data 
        int data = rfValue; //Serial.read();    // read incoming serial data:
  
    
    //keyboard controls//
    //if (Serial.available() > 0) {
      //  signed char data = Serial.read();    // read incoming serial data:
    
    
    
          
        switch(data) {
        case 1:                              //Emergency Stop
            forward = false;
            reverse = false;
            eBrake = true;
            avgSpeed = 0;
            setMotorSpeed(); 
            break;                                
        case 0:                              //Go Forward   (uses wads controls)    
            if (avgSpeed < 10) {                      //limits to a maximum motorSpeed of 10
                avgSpeed++;
                if (avgSpeed == 1) {
                    forward = true;
                    eBrake = false;
                }
                setMotorSpeed(); 
            }
            break;          
        case 4:    //Turn Left
            if (turnLeft < turnSteps) {
                if (turnRight == 0) {
                    turnLeft++;
                } else if (turnRight > 0){
                    turnRight--;
                }
                turnServo(); 
              }
            break;
        case 2:                              //Turn Right
            if (turnRight < turnSteps) {
                if (turnLeft == 0) {                      
                    turnRight++; 
                } else if (turnLeft > 0) {
                    turnLeft--;
                }
                turnServo();
            }
            break;
        case 3:                              //Reverse //for now this slows down wheels when motorSpeed > 0 and reverses(later add time duration of gas button depress to control slow down)
            if (avgSpeed > -10) {
                avgSpeed--;
                if(avgSpeed == 0) {
                    forward = false;
                }
                if (avgSpeed == -1) {          
                    reverse = true;
                    eBrake = false;
                }
                setMotorSpeed();
            }
            break;
        }
        
    
        //// debug code - start printing to console ////
        Serial.println(" Reverse=");
        Serial.println(reverse);
        Serial.println(" Forward=");
        Serial.println(forward);
        Serial.println(" avgSpeed=");
        Serial.println(avgSpeed);
        Serial.println(" ebrake=");
        Serial.println(eBrake);
        Serial.println(" turnLeft=");
        Serial.println(turnLeft);
        Serial.println(" turnRight=");
        Serial.println(turnRight);
        
        /////////////// end debug code //////////////////
    }
}


void turnServo() { 
  
    const int center = 132;
  
    if (turnLeft > 0 ) {           // turn servo one step left
        servoVal = map(turnLeft, 0, 5, 132, 162);  // need to map one more than steps allowed
        servo.write(servoVal);
        delay(15);
    } else if (turnRight > 0) {    // turn servo one step right
        servoVal = map(turnRight, 0, 5, 132, 102);   // need to map one more than steps allowed
        servo.write(servoVal);
        delay(15);
    } else if (turnRight == 0 && turnLeft == 0) {  // neutral
        servoVal = 132;  // zero
        servo.write(servoVal);
        delay(15);
    }
}


void setMotorSpeed() { 

    //// debug code - start printing to console ////
    //Serial.println("motorSpeedOld 1 =");
    //Serial.println(motor1SpeedOld);
    /////////////// end debug code //////////////////
    
    
    SaberSerial.listen();
    
  
    // read drive state then set motor speed 
    if( eBrake == true ) {        // Send stop command to both motors
        //SaberSerial.write((byte) zero );
        motor1Speed = motor1StopValue;
    } else if( forward == true ) {
        motor1Speed = map( avgSpeed, 0, 10, motor1StopValue, motor1FullForwardValue);
    } else if( reverse == true ) {
        motor1Speed = motor1ReverseValue;
        //motor2Speed = motor2ReverseValue;
    } else { }
  
  
  
  
  
    // increment old speed to new speed slowly to prevent motor jerk

    if (avgSpeed == 0 ) {
       SaberSerial.write((byte) motor1StopValue ); 


    } else if (motor1Speed != motor1SpeedOld) {
        int diff1 = (motor1Speed - motor1SpeedOld);
        diff1 = abs(diff1);
    
        for (int i=0; i < diff1; i++) {
            if (motor1Speed != motor1SpeedOld) {     
                if (motor1SpeedOld < motor1Speed ) {
                    motor1SpeedOld++;
                }
                if(motor1SpeedOld > motor1Speed) {
                    motor1SpeedOld--;
                }
                //write to motor controller
                SaberSerial.write((byte) motor1SpeedOld );  
            }
        }
    }
    
    RFSerial.listen();

    return;
}


// Receives an unsigned int over the RF network
unsigned int readUInt(bool wait) {
  int pos = 0;          // Position in the network signature
  unsigned int val;     // Value of the unsigned int
  byte c = 0;           // Current byte
  
  if((RFSerial.available() < PACKET_SIZE) && (wait == false)) {
    return 0xFFFF;
  }
  
  while(pos < NETWORK_SIG_SIZE) { 
    while(RFSerial.available() == 0); // Wait until something is avalible
    c = RFSerial.read();
    if (c == g_network_sig[pos]) {
      if (pos == NETWORK_SIG_SIZE-1) {
        byte checksum;
        while(RFSerial.available() < VAL_SIZE + CHECKSUM_SIZE); // Wait until something is avalible
        val = RFSerial.read();
        Serial.println(val);
        if (VAL_SIZE == 2)
           val += ((unsigned int)RFSerial.read())*256;
           
        checksum =  RFSerial.read();
        
        if (checksum != ((val/256) ^ (val&0xFF))) {
          // Checksum failed
          pos = -1;
         Serial.println("checksum failed");
        }
      }
      ++pos;
    } else if (c == g_network_sig[0]) {
      pos = 1;
    } else {
      pos = 0;
      if (!wait) {
        return 0xFFFF;
      }
    }
  }
  return val;
}



//// for transmitting --> Sends an unsigned int over the RF network
//void writeUInt(unsigned int val)
//{
//  byte checksum = (val/256) ^ (val&0xFF);
//  Serial.write(0xF0);  // This gets reciever in sync with transmitter
//  Serial.write(g_network_sig, NETWORK_SIG_SIZE);
//  Serial.write((byte*)&val, VAL_SIZE);
//  Serial.write(checksum); //CHECKSUM_SIZE
//}



