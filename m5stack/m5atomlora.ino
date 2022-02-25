/*
    Description: 
    Use ATOM DTU LoRaWAN to connect to the Gateway by OTAA mode, and implement subscription and publishing messages.
    Before use, please configure the device information, receiving window, frequency band mask and other information according to the actual connected network.
    Please install library before compiling:  
    FastLED: https://github.com/FastLED/FastLED
    M5Atom: https://github.com/m5stack/M5Atom
    
    Modified: markus van kempen - markus@vankempen.org 
    add preference and counter llp format to send data and deepsleep
    
*/

#include "Wire.h"
#include "ATOM_DTU_LoRaWAN.h"
#include "M5Atom.h"

#include <CayenneLPP.h>

#include <Preferences.h>

#include "SHTSensor.h"

SHTSensor sht;
// To use a specific sensor instead of probing the bus use this command:
//SHTSensor sht(SHTSensor::SHT3X);
float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;

CayenneLPP lpp(51);

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  900        /* Time ESP32 will go to sleep (in seconds) */

ATOM_DTU_LoRaWAN LoRaWAN;
String response;
String mystr = "";
unsigned int counter =0;
unsigned int errcounter =0;


Preferences preferences;

typedef enum {
    kError = 0,
    kConnecting,
    kConnected, 
    kSending
} DTUState_t;

DTUState_t State = kConnecting;

void TaskLED(void *pvParameters) {
    while(1) {
        switch (State)
        {
        case kError:
            M5.dis.drawpix(0, 0x00ff00);
            break;
        case kConnecting:
            M5.dis.drawpix(0, 0xff0000);
            break;
        case kConnected:
            M5.dis.drawpix(0, 0x0000ff);
            break;
        case kSending:
            M5.dis.drawpix(0, 0xff000ff);
            break;
        }
        for(int i=10;i>0;i--) {
            M5.dis.setBrightness(i);
            FastLED.show();
            vTaskDelay( 50 / portTICK_RATE_MS );
        }
        for(int i=0;i<10;i++) {
            M5.dis.setBrightness(i);
            FastLED.show();
            vTaskDelay( 50 / portTICK_RATE_MS );
        }
    }
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

String getHexString(uint8_t *num)
{ //
  String s = "";
  for (int i = 0; i < sizeof(num); i++)
  {
    char hexCar[2];
    sprintf(hexCar, "%02X", num[i]);
    s = s + hexCar;
  }
  return (s);
}

String printHex(uint8_t num) {
   String s = "";
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);

  return(hexCar);
}
void setup()
{
       Wire.begin(25,21);
    M5.begin(true, false, true);
    //InIt
    LoRaWAN.Init();
    preferences.begin("app", false); 
    counter = preferences.getUInt("msgcounter", 0); 
    Serial.println("msgcounter = "+String(counter));
    errcounter = preferences.getUInt("errcounter", 0); 
    Serial.println("errcounter = "+String(errcounter));
        
    //Reset Module
    Serial.print("Module Rerest.....");
    print_wakeup_reason();


 Serial.print("Temperature read - sht.init()\n");
    // Temperature read
if (sht.init()) {
      Serial.print("init(): success\n");
  } else {
      Serial.print("init(): failed\n");
  }
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x



  if (sht.readSample()) {
      Serial.print("SHT:\n");
      Serial.print("  RH: ");
      hum = roundf((sht.getHumidity()*100))/100;
      Serial.print(hum, 2);
      Serial.print("\n");
      Serial.print("  T:  ");
      tmp = roundf((sht.getTemperature()*100))/100;
      Serial.print(tmp, 2);
      Serial.print("\n");
  } else {
      Serial.print("Error in readSample()\n");
  }
  
    LoRaWAN.writeCMD("AT+ILOGLVL=1\r\n");
    LoRaWAN.writeCMD("AT+CSAVE\r\n");
    LoRaWAN.writeCMD("AT+IREBOOT=0\r\n");
    delay(5000);

    //Create LED Task
    xTaskCreatePinnedToCore(
        TaskLED
        ,  "TaskLED"   // A name just for humans
        ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
        ,  NULL
        ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,  NULL 
        ,  0);


    LoRaWAN.configOTTA(
        "d896e0ff00000241",//Device EUI
        "0000000000000001",//APP EUI
        "98929b92f09e2daf676d646d0f61d251",//APP KEY
        "2"//Upload Download Mode
    );

    response = LoRaWAN.waitMsg(1000);
    Serial.println(response);
    
    //Set Class Mode 
    LoRaWAN.setClass("2");

    LoRaWAN.writeCMD("AT+CWORKMODE=2\r\n");

    //LoRaWAN470
    // LoRaWAN.setRxWindow("505300000");

    //LoRaWAN868
    // LoRaWAN.setRxWindow("869525000");

    //LoRaWAN915
    LoRaWAN.setRxWindow("923300000");

    // LoRaWAN470 TX Freq
    // 486.3
    // 486.5
    // 486.7
    // 486.9
    // 487.1
    // 487.3
    // 487.5
    // 487.7
    //MARK 0000 0100 0000 0000 | 0x0400
    // LoRaWAN.setFreqMask("0400");


    // LoRaWAN868 TX Freq
    // 868.1 - SF7BW125 to SF12BW125
    // 868.3 - SF7BW125 to SF12BW125 and SF7BW250
    // 868.5 - SF7BW125 to SF12BW125
    // 867.1 - SF7BW125 to SF12BW125
    // 867.3 - SF7BW125 to SF12BW125
    // 867.5 - SF7BW125 to SF12BW125
    // 867.7 - SF7BW125 to SF12BW125
    // 867.9 - SF7BW125 to SF12BW125
    // 868.8 - FSK
    // LoRaWAN.setFreqMask("0001");

    // LoRaWAN915 TX Freq
    // 902.3
    // 902.5
    // 902.7
    // 902.9
    // 903.1
    // 903.3
    // 903.5
    // 903.7
    //MARK 0000 0000 0000 0001 | 0x001
    LoRaWAN.setFreqMask("0001");

    delay(100);
    response = LoRaWAN.waitMsg(1000);
    Serial.println(response);
    LoRaWAN.startJoin();
    Serial.print("Start Join.....");
    while(1){
        response = LoRaWAN.waitMsg(1000);
        Serial.println(response);
        if(response.indexOf("+CJOIN:") != -1) {
            State = kConnected;
            break;
        }else if(response.indexOf("ERROR") != -1){
            State = kError;
            Serial.print("Join ERROR.");
            ESP.restart();
        }
    }
    delay(2000);
}

void loop()
{
     Serial.println("In Loop");
     delay(100);
     counter++;  
     Serial.printf("Increase counter value: %u\n", counter);  // Print the counter to Serial Monitor. 
    lpp.reset();
    lpp.addAnalogInput (1, counter);
    lpp.addAnalogOutput(1, errcounter);
    lpp.addTemperature(1, tmp);
    lpp.addRelativeHumidity(1, hum);
/*
 *     lpp.addTemperature(1, t);
    lpp.addRelativeHumidity(1, h);
    lpp.addBarometricPressure(1,p);
    lpp.addAnalogInput(1, lora.frameCounter);
    lpp.addAnalogOutput(1, g);
  
 */
   String t="";
    
    Serial.println("Sending: ");
   // lora.sendData(lpp.getBuffer(), lpp.getSize(), lora.frameCounter);
        //for (int i = 0; i < sizeof(lpp.getBuffer()); i++)
        for (int i = 0; i < lpp.getSize(); i++)
        {
        t = t+printHex(lpp.getBuffer()[i]);
        Serial.print(printHex(lpp.getBuffer()[i]));
        Serial.println(" = " +String(i));
        }
    Serial.println();

        mystr =  String(getHexString(lpp.getBuffer()));
        Serial.println("Payload = "+mystr);
        Serial.println("Payload Size = "+mystr.length());

        Serial.println("String/text = "+String(t));
        Serial.println("String/text Size = "+t.length());
        Serial.print("lpp.getSize() =");
        Serial.println(lpp.getSize());
         Serial.print("sizeof(lpp.getBuffer()); =");
        Serial.println(sizeof(lpp.getBuffer()));

    //send data
    // LoRaWAN.sendMsg(0,15,8,"03670110056700FF");
    // LoRaWAN.sendMsg(0,15,15,"0102157C010300000167022701686E");
    // LoRaWAN.sendMsg(0,8,4, "01020064");//lpp test
     //LoRaWAN.sendMsg(0,8,4, "0102044C");//lpp test
     LoRaWAN.sendMsg(1, 15,lpp.getSize(),t);
    //send data
    //   void sendMsg(uint8_t confirm, uint8_t nbtrials, size_t length, String data);
   //LoRaWAN.sendMsg(1,15,8,"01020E740E74");
    while(1) {
        State = kSending;
        response = LoRaWAN.waitMsg(1000);
        Serial.println(response);
        if(response.indexOf("OK") != -1) {

          if(counter>1000)
            counter=0;
          preferences.putUInt("msgcounter", counter);  // Store the counter to the Preferences.


            break;
        }else if(response.indexOf("ERR") != -1){
          State = kError;
          if(errcounter>1000)
            counter=0;
          errcounter++;
          preferences.putUInt("errcounter", errcounter);  // Store the counter to the Preferences.      
            break;
        }
    }
       preferences.end();  // Close the Preferences. 
    delay(3000);
    //receive data
    response = LoRaWAN.receiveMsg();
    Serial.print(response);
    delay(3000);
  M5.dis.drawpix(0, 0x000000);
  M5.dis.setBrightness(0);
  FastLED.show();  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +" Seconds");
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
  
}


