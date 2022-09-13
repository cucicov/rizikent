/* 
  Menu driven control of a sound board over UART.
  Commands for playing by # or by name (full 11-char name)
  Hard reset and List files (when not playing audio)
  Vol + and - (only when not playing audio)
  Pause, unpause, quit playing (when playing audio)
  Current play time, and bytes remaining & total bytes (when playing audio)

  Connect UG to ground to have the sound board boot into UART mode
*/

#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#define echoPin 2 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 3 //attach pin D3 Arduino to pin Trig of HC-SR04
#define SFX_TX 5
#define SFX_RX 6
#define MAX_DISTANCE 100

#define TRACK_TIME_MILLIS 61000

// Connect to the RST pin on the Sound Board
#define SFX_RST 4

const int numReadings = 100;
int readings[numReadings]; 
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

// Choose any two pins that can be used with SoftwareSerial to RX & TX
#define SFX_TX 5
#define SFX_RX 6

// Connect to the RST pin on the Sound Board
#define SFX_RST 4

// You can also monitor the ACT pin for when audio is playing!

// we'll be using software serial
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);

// pass the software serial to Adafruit_soundboard, the second
// argument is the debug port (not used really) and the third 
// arg is the reset pin
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);
// can also try hardware serial with
// Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

void setup() {
  pinMode(7,OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  Serial.begin(115200);
  Serial.println("Adafruit Sound Board!");
  
  // softwareserial at 9600 baud
  ss.begin(9600);
  // can also do Serial1.begin(9600)

  if (!sfx.reset()) {
    Serial.println("Not found");
    while (1);
  }
  Serial.println("SFX board found");
  
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  if (! sfx.playTrack((uint8_t)2) ) {
    Serial.println("Failed to init play track?");
  }
}

int frameCount = 0;
boolean trackStop = false;
long timePassedPlaying = 0;
long timestamp = 0;

void loop() {

//  flushInput();
//
//  Serial.println(average);

//  if (trackStop) {
//    if (average > MAX_DISTANCE) {
//      frameCount = 290;
//    } else {
//      frameCount = 0;
//    }
//  }

  // first trigger
//  Serial.println(timePassedPlaying);
  if (trackStop && average > MAX_DISTANCE && average > MAX_DISTANCE) {
    Serial.println("PLAY! T");
    digitalWrite(7,HIGH);
    
    if (timePassedPlaying > TRACK_TIME_MILLIS) {
      if (! sfx.playTrack((uint8_t)2) ) {
        Serial.println("Failed to play track?");
      }
    } else {
      if (! sfx.unpause() ) Serial.println("Failed to unpause");
    }
    timestamp = millis() - timePassedPlaying;
    frameCount = 0;
    trackStop = false;
  }

  // continue looping
  if (!trackStop && timePassedPlaying > TRACK_TIME_MILLIS) {
    timePassedPlaying = 0;
    timestamp = millis();
    
    Serial.println("PLAY!");
    digitalWrite(7,HIGH);
    
    if (! sfx.playTrack((uint8_t)2) ) {
      Serial.println("Failed to play track?");
    }
    frameCount = 0;
  }

  if (!trackStop) {
    timePassedPlaying = millis() - timestamp;
  }
  frameCount++; //TODO: remove?

//   uint32_t current, total;
//    if (! sfx.trackTime(&current, &total) ) Serial.println("Failed to query");
//    Serial.print(current); Serial.println(" seconds");
  

// Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  // Displays the distance on the Serial Monitor
//  if (distance > 200) {

  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = distance;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;

  if (!trackStop && average < MAX_DISTANCE && average > 0) {
    Serial.println("STOP!");
    digitalWrite(7,LOW);
    
    Serial.print("Distance: ");
    Serial.print(average);
    Serial.println(" + ");
    if (! sfx.pause() ) Serial.println("Failed to pause");
    trackStop = true;
  }
  
}






/************************ MENU HELPERS ***************************/

void flushInput() {
  // Read all available serial input to flush pending data.
  uint16_t timeoutloop = 0;
  while (timeoutloop++ < 40) {
    while(ss.available()) {
      ss.read();
      timeoutloop = 0;  // If char was received reset the timer
    }
    delay(1);
  }
}

char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}

uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

uint8_t readline(char *buff, uint8_t maxbuff) {
  uint16_t buffidx = 0;
  
  while (true) {
    if (buffidx > maxbuff) {
      break;
    }

    if (Serial.available()) {
      char c =  Serial.read();
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0) {  // the first 0x0A is ignored
          continue;
        }
        buff[buffidx] = 0;  // null term
        return buffidx;
      }
      buff[buffidx] = c;
      buffidx++;
    }
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}
/************************ MENU HELPERS ***************************/
