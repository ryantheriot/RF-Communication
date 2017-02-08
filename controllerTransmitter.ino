/*
  Reading a serial ASCII-encoded string.
 
 This sketch demonstrates the Serial parseInt() function.
 It looks for an ASCII string of comma-separated values.
 It parses them into ints, and uses those to fade an RGB LED.
 
 Circuit: Common-anode RGB LED wired like so:
 * Red cathode: digital pin 3
 * Green cathode: digital pin 5
 * blue cathode: digital pin 6
 * anode: +5V
 
 created 13 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 */
 
#include <digitalWriteFast.h> 
#include <SoftwareSerial.h>
 
#define NETWORK_SIG_SIZE 3

#define VAL_SIZE         1
#define CHECKSUM_SIZE    1
#define PACKET_SIZE      (NETWORK_SIG_SIZE + VAL_SIZE + CHECKSUM_SIZE)

// The network address byte and can be change if you want to run different devices in proximity to each other without interfearance
#define NET_ADDR 5

const int RFSEND = 11;
const int RFREC = 10;

SoftwareSerial RFSerial = SoftwareSerial(RFREC, RFSEND); // RX, TX

const byte g_network_sig[NETWORK_SIG_SIZE] = {0x8F, 0xAA, NET_ADDR};  // Few bytes used to initiate a transfer


// Sends an unsigned int over the RF network
void writeUInt(unsigned int val)
{
  byte checksum = (val/256) ^ (val&0xFF);
  RFSerial.write(0xF0);  // This gets reciever in sync with transmitter
  RFSerial.write(g_network_sig, NETWORK_SIG_SIZE);
  RFSerial.write((byte*)&val, VAL_SIZE);
  RFSerial.write(checksum); //CHECKSUM_SIZE
}



void setup() {
  // initialize serial:
  
  pinModeFast(RFSEND, OUTPUT);
  pinModeFast(RFREC, INPUT);
  
  RFSerial.begin(1200);
  Serial.begin(9600);
  
  Serial.println("Connected to the RC car controller!");
}

void loop() {
  // if there's any serial available, read it:
  if (Serial.available() > 0) {
    unsigned char c = Serial.read();
    writeUInt(c - 'A');
  }
}
