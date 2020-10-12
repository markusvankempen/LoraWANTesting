/*
  Lora Send And Receive
  This sketch demonstrates how to send and receive data with the MKR WAN 1300/1310 LoRa module.
  This example code is in the public domain.
  
  I added some distance sensor and a light sensor 
*/

#include <MKRWAN.h>
#include <CayenneLPP.h>
#include <ArduinoLowPower.h>
LoRaModem modem;

// Uncomment if using the Murata chip as a module
// LoRaModem modem(Serial1);

#include "arduino_secrets.h"
// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

#define echoPin 2 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 3 //attach pin D3 Arduino to pin Trig of HC-SR04

// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

void reboot() {
  NVIC_SystemReset();
  while (1) ;
}

int analogValue=0;
int oldlight=0;
 int myi =0;
 int myintcnt=0;
void repetitionsIncrease() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delSomething went wrongay() and long running functions since this functions executes in interrupt context
 // Serial.println("Wake UP");
  myi = oldlight-analogValue;
  myintcnt++;
 if(myi<0)
  myi=myi*(-1);
 
 if(myi>50)
 {
  reboot() ;
      pinMode(0, OUTPUT);
  //  digitalWrite(0, HIGH);
 }
}

void blink( int numBlink = 1, unsigned int speed = 200 ) {
  int i;
  while ( numBlink-- ) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(speed);                       // wait
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(speed);                       // wait
  }
}
int msgcnt=1;
void setup() {

  // put your setup code here, to run once:
  Serial.begin(115200);
  //while (!Serial);
  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(US915)) {
    Serial.println("Failed to start module");
    delay(5000);
        pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
   return;
  };
    Serial.println();
   Serial.println(">>>>>START");
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());
  Serial.print("Your APP EUI is: ");
  Serial.println(appEui);
  Serial.print("Your APP KEY is: ");
  Serial.println(appKey);
  
  Serial.println("Ultra Sound Pins D1/D2 =  Echo /Trig ");
   pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  //pinMode(0, OUTPUT);
   digitalWrite(0,LOW);     // pre-set output value to HIGH
  //pinMode(0,OUTPUT);        // set to output, it will be HIGH


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
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.println("Light Sensor on  =  A1 "); 
  Serial.print("Analog Light reading A1 = ");
  Serial.println( analogRead(A1));   // the raw analog reading
  Serial.print("Analog  reading A2= ");
  Serial.println( analogRead(A2));   // the raw analog reading  
  
  LowPower.attachInterruptWakeup(A1, repetitionsIncrease, CHANGE);
  
   Serial.println("Connecting to Lora "); 
    Serial.println(LORA);
    
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    Serial.println("Reboot");
    
    delay(30000);
    
        pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
   return;
  }
    modem.setPort(10);
    modem.dataRate(3);
    modem.setADR(true);
  // Set poll interval to 60 secs.
  modem.minPollInterval(90);
  // NOTE: independently by this setting the modem will
  // not allow to send more than one message every 2 minutes,
  // this is enforced by firmware and can not be changed.
    blink(3);
    
}

void loop() {
    digitalWrite(0, LOW);
    oldlight=analogValue;
 // reads the input on analog pin A0 (value between 0 and 1023)
   analogValue = analogRead(A1);

  Serial.print("Analog reading = ");
  Serial.print(analogValue);   // the raw analog reading

  // We'll have a few threshholds, qualitatively determined
  if (analogValue < 10) {
    Serial.println(" - Dark");
  } else if (analogValue < 200) {
    Serial.println(" - Dim");
  } else if (analogValue < 500) {
    Serial.println(" - Light");
  } else if (analogValue < 800) {
    Serial.println(" - Bright");
  } else {
    Serial.println(" - Very bright");
  }


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
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(500);
  float battery_voltage = 3.01; // use voltmeter
  analogReadResolution(10);
  analogReference(AR_INTERNAL1V0); //AR_DEFAULT: the default analog reference of 3.3V //
  int sensorValuebat = analogRead(ADC_BATTERY);
    // Convert the analog reading (which goes from 0 - 1023)
  float voltagebat = sensorValuebat * (battery_voltage / 1023.0);
    Serial.print("Voltage: ");
  Serial.println(voltagebat);
Serial.println("------------Write LPP data------------");
    CayenneLPP lpp(30);
    lpp.reset();
    lpp.addVoltage(1, voltagebat );
    lpp.addDistance(1 , distance);
    lpp.addLuminosity(1, analogValue);

    lpp.addAnalogInput(2, myintcnt);
    lpp.addAnalogInput(3, myi);
    lpp.addAnalogInput(4, msgcnt);
    
  int err;

  modem.beginPacket();
  modem.write(lpp.getBuffer(), lpp.getSize());
  
  err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent correctly!");
    Serial.println(lpp.getSize());

     Serial.print("Msg #");
       Serial.println(msgcnt);
           msgcnt++;
     blink(1);
  } else {
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }
  delay(1000);
   Serial.print("hier1");
 Serial.print("myintcnt="); 
   Serial.println(myintcnt);
     
  if (analogValue < 200) {
    Serial.println("Looks like Night time to reboot - wait 5 min");
    delay(360000);
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
  }

  
  if (!modem.available()) {
    Serial.println("No downlink message received at this time.");
    Serial.println("Wait 90sec");
    delay(90000);
   return;
  }

  char rcv[64];
  int i = 0;
  while (modem.available()) {
    rcv[i++] = (char)modem.read();
  }
  Serial.print("Received: ");
  for (unsigned int j = 0; j < i; j++) {
    Serial.print(rcv[j] >> 4, HEX);
    Serial.print(rcv[j] & 0xF, HEX);
    Serial.print(" ");
  }
  Serial.println();
 //   Serial.println("Wait 10sec ");
   //  delay(10000);


     Serial.println(myi);
Serial.println("Sleep");
  //LowPower.sleep(300000);
  Serial.println("Wake");
  Serial.println("Wait 60sec ");
    delay(60000);


     
}
