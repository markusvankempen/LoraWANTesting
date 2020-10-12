
/*
   test Arduino Lora Node

   06-June-2020 - markus mvk@ca.ibm.com

   TODO Clean up

   This code conains Sensors DHT22 , DS(onewire), Oled,GPS Beitain,

*/
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoLowPower.h>
#include <MKRWAN.h>
#include <ZeroRegs.h>
#include <RTCZero.h>
#include "arduino_secrets.h"
#include <CayenneLPP.h>
#include <DHT.h>
#include <DS18B20.h>
#include <ArduinoUniqueID.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <pins_arduino.h>
#include <SD.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <CayenneLPPDec.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



DS18B20 ds(2);
CayenneLPP lpp(55);

#define SLEEP_PERIOD ( (uint32_t) 120000 ) //Wait 5 min

// ADC configuration
#define extSensorPin PIN_A1
#define extSensorEnablePin 1
#define VERSION "2020-06-06  15:40:00"
#define magnetSensorA PIN_A3
#define magnetSensorD 3
#define DHTPIN 0
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

LoRaModem modem;
static uint32_t msgCount = 0;
bool regsShown = false;
// RTCZero rtc;

// inizializzazioni partitore di tensione
double Vbatt; double Vbatt_perc; double V_R2;
double VbattMax = 4.3; double VbattMin = 2.7;
double resolutionVoltage = 0.00107422; // resolution = AREF / 1024 = 1.1V / 1024
double R1 = 20000; double R2 = 100000;

int lightsensor=0;

// Uncomment if using the Murata chip as a module
// LoRaModem modem(Serial1);

// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

#define MAX_STR_LEN 200
char *printHexBuffer (const uint8_t *buffer, uint16_t len) {
  static char tempStr[MAX_STR_LEN];
  int charIndex = 0;

    memset (tempStr, 0, MAX_STR_LEN);

    for (int i = 0; i < len; i++) {
        if (i < MAX_STR_LEN-1) {
            charIndex += sprintf (tempStr + charIndex, "%02X ", buffer[i]);
        }
    }
  return tempStr;
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

void printTimeStamp(Print* _logOutput) {
  char c[12];
  int m = sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}

void printNewline(Print* _logOutput) {
  _logOutput->print('\n');
}

void reboot() {
  NVIC_SystemReset();
  while (1) ;
}

void pinStr( uint32_t ulPin, unsigned strength) // works like pinMode(), but to set drive strength
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return ;
  }
  if (strength) strength = 1;     // set drive strength to either 0 or 1 copied
  PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].bit.DRVSTR = strength ;
}

void InitIO() {
  //  Note:  wiring.c initializes all GPIO as inputs with no pull ups ... it has been modified to eliminate this

  // Configure digital pins used by the application
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(extSensorPin, OUTPUT);
  pinStr(extSensorPin, 1);
  digitalWrite(extSensorEnablePin, LOW);


  // Configure the ADC
  analogReadResolution(12);
  analogWriteResolution(12);
  analogReference(AR_INTERNAL1V65);
}

void alarmEvent() {
  // RTC alarm wake interrupt callback
  // do nothing
  Log.notice(F("alarm wake interrupt callback" CR));
  blink(1);
  delay(1000);
    blink(1);
  delay(1000);
  if (msgCount>100)
     reboot();
}
/*
   showFlashDir

*/

SerialFlashFile flashFile;
const int FlashChipSelect = 32;
int FCstate = 8;
int i = 0;
int showFlashDir(boolean d)
{
  unsigned char buf[256];
  int FCstate = SerialFlash.begin(SPI1, FlashChipSelect);
  //Serial.println(FCstate);
  if (FCstate == 1)
  {
    Serial.println("We have FlashMemory :)");
    Serial.println("Read Chip Identification:");
    SerialFlash.readID(buf);
    Serial.print("  JEDEC ID:     ");
    Serial.print(buf[0], HEX);
    Serial.print(" ");
    Serial.print(buf[1], HEX);
    Serial.print(" ");
    Serial.println(buf[2], HEX);
    Serial.print("  Memory Size:  ");

    Serial.print(SerialFlash.capacity(buf));
    Serial.println(" bytes");

    SerialFlash.opendir();
    while (1) {
      char filename[64];
      uint32_t filesize;

      if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
        Serial.print("  ");
        Serial.print(filename);
        spaces(20 - strlen(filename));
        Serial.print("  ");
        Serial.print(filesize);
        Serial.print(" bytes");
        Serial.println();
        i++;
        if (d)
          SerialFlash.remove(filename);
      } else {
        break; // no more files

      }
    }

  } else {

    Serial.println("No FlashMemory :(");
  }
  return i;
}
/*
   Read a FlashFile - return message counter only
*/
String readFlashfile(char fname[], int pos, boolean d)
{
  int FCstate = SerialFlash.begin(SPI1, FlashChipSelect);
  //Serial.println(FCstate);
  SerialFlashFile flashFile;
  Serial.print("Reading flashFile: ");
  Serial.println(fname);
  char buf[1024];
  char tmp[6];
  String strData;
  if (FCstate == 1)
  {
    if (SerialFlash.exists(fname)) {
      Serial.println("File exists");
      flashFile = SerialFlash.open(fname);
      flashFile.read( buf, 1024);
      Serial.println(buf);
      //    char* pos = strstr_P(buf, PSTR(","));
      //   int i=strlen(buf)-strlen(pos);
      //  strncpy(tmp, strstr(buf,","), 6 )  ;
      //  Serial.println(tmp);
      strData = String ((char*)buf);

      // Serial.println("strData=");
      // Serial.println(strData);
      int firstCommaIndex = strData.indexOf(',');
      int secondCommaIndex = strData.indexOf(',', firstCommaIndex + 1);
      Serial.print("msgcounter=");
      Serial.println(strData.substring(0, firstCommaIndex));
      return ( strData.substring(0, firstCommaIndex));

    }
    return "0";
  } else {

    Serial.println("Some Flash Error ");

  }
  return "0";
}

boolean createFlashfile(char myfname[], int fsize, char buffer[], boolean d)
{
  int FCstate = SerialFlash.begin(SPI1, FlashChipSelect);
  //Serial.println(FCstate);
  SerialFlashFile flashFile;
  Serial.println("Creating flashFile");

  //  char myfname[] = "info1.txt";
  char buf[1024];
  //int fsize = 1024;
  Serial.println(myfname);

  if (FCstate == 1)
  {
    if (d)
    {

      if (SerialFlash.exists(myfname)) {
        Serial.println("File exists:  will delete");
        flashFile = SerialFlash.open(myfname);
        flashFile.read( buf, fsize);
        Serial.println(buf);
        SerialFlash.remove(myfname);  //It doesn't reclaim the space, but it does let you create a new file with the same name.

      }
    }//delete


    //Create a new file and open it for writing
    if (SerialFlash.create(myfname, fsize))
    {

      flashFile = SerialFlash.open(myfname);
     // Serial.println("hier2");
      if (!flashFile)
      {
        //Error flash file open
        Serial.println("Error:   flash file open");
        return false;
      } else {
        flashFile.write(buffer, fsize);//sizeof( buffer));
        flashFile.close();

      }
    } else {
      //Error flash create (no room left?)
      Serial.println("Error:no room left - can not create file ");
      return false;
    }
    return true;

  } else {

    Serial.println("Some Flash Error ");
    return false;
  }
}
unsigned int address = 0;
byte value;


void spaces(int num) {
  for (int i = 0; i < num; i++) {
    Serial.print(" ");
  }
}

int connected =0;

void setup() {

  //InitIO();

  pinMode(PIN_A1, INPUT);
  pinMode(PIN_A4, INPUT);
  pinMode(magnetSensorA,INPUT);
  pinMode(magnetSensorD, INPUT);
  pinMode(LORA_RESET, OUTPUT);
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);

  Serial.println(" >>>> Mag Analog ="+String(analogRead(magnetSensorA)));
  Serial.println(" >>> Mag Dialog ="+String(digitalRead(magnetSensorD)));

  //GPSserial.begin(GPSBaud);  
 
  int waitForSerial = 5;
  while (!Serial && !Serial.available() && waitForSerial ) {
    delay(2000);
    blink(2);
    waitForSerial--;
  }
  Serial.println("------------------------");
  UniqueID8dump(Serial);
  Serial.print("Display check");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  //  for(;;); // Don't proceed, loop forever
  }else{
     Serial.println("..ok");
       // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();
  display.clearDisplay();
  delay(2000); // Pause for 2 seconds
  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, SSD1306_WHITE);
    display.display();
    delay(1);
  }
   
  
  delay(2000); // Pause for 2 seconds
  // Clear the buffer
  display.clearDisplay();
  display.display();

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Hello"));
  display.setCursor(0,8);
  display.println(F("Weather Stattion v1"));
  display.setCursor(20,16);
  display.println(F("20,16"));  
  display.setCursor(30,24);
  display.println(F("30,24__123456789012345"));
  display.display();
  delay(2000); // Pause for 2 seconds
    }
  Serial.println("FlashDrive");
  showFlashDir(false);
  String smsgcounter = readFlashfile("info3.txt", 0, false);
  msgCount = smsgcounter.toInt();
  msgCount++;
  if (msgCount> 60000)
     msgCount=1;
  // Fileformat is a CSV file
  // MsgCounter,DHTTemp,DHTHumidiy,DSTem,Volatge,
  String mymsg = "0" + String(msgCount) + ",DHTTemp,DHTHumidiy,DSTem,Volatge,#";
  Serial.println("mymsg = " + mymsg);

  Serial.println("Length = " + String(mymsg.length()));
  char buf[1024];
    char buf1[1024];
  mymsg.toCharArray(buf, mymsg.length());

  createFlashfile("info4.txt", 1024, buf, true); // create delete old one
  Serial.println("------------Write LPP data------------");
    CayenneLPP lpp(160);
    lpp.reset();
    lpp.addTemperature(1, 23.12);
    lpp.addVoltage(2, 5 );
    lpp.addAnalogInput(3, msgCount);
    lpp.addGenericSensor(4, analogRead(PIN_A4));
    lpp.addAccelerometer(5, 1.234f, -1.234f, 0.567f);
    lpp.addBarometricPressure(6, 1023.4f);
    lpp.addGyrometer(7, -12.34f, 45.56f, 89.01f);
    lpp.addGPS(8, -12.34f, 45.56f, 9.01f);
    
     memcpy(buf,  lpp.getBuffer(), lpp.getSize());

    //createFlashfile("lpp1.txt",  lpp.getSize(),buf, true); // create delete old one


//  DynamicJsonDocument jsonBuffer(4096);
 // JsonArray root = jsonBuffer.to<JsonArray>();

  
  //  flashFile = SerialFlash.open("lpp1.txt");
  //  flashFile.read( buf1,  lpp.getSize());
     

   //  uint8_t *lppbuf = (uint8_t)atoi(buf1);
   //lpp.decode(lpp.getBuffer(), lpp.getSize(), root);
  
   // serializeJsonPretty(root, Serial);
  
  Serial.println("------------------------");
  
  //
  Log.notice(F("Start" CR));
  Log.notice(F("VERSION = %s" CR), VERSION);
  // initialize the RTC - need to do this or the sleep function is unreliable after reset
//  rtc.begin(false);

   RTCZero().begin(false);
    
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmEvent, CHANGE);

  //  Log.notice(F("Reset modem ..." CR));
  //  modem.restart();  // This does not work for some reason
  Log.notice(F("Initialize modem ..." CR));
  if (!modem.begin(US915)) {
    Log.error(F("Failed to start module, rebooting MKRWAN in 1 seconds ..." CR));
    delay(1000);
    reboot();
  };



  Log.notice(F("Your module version is: %s" CR), modem.version().c_str());
  Log.notice(F("Your device EUI is: %s" CR), modem.deviceEUI().c_str());

  Serial.println(appEui);
  Serial.println(appKey);
  dht.begin();
  delay(5000);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);


  Serial.print("Analog Light reading A2= ");
  Serial.println( analogRead(A2));   // the raw analog reading 
  
  /*

    Log.notice(F("Humidity: "));
    Log.notice(h);
    Log.notice(F("%  Temperature: "));
    Log.notice(t);
  */
  // Log.notice("DS Temperature: % ");
  //Log.notice(ds.getTempC());
  Log.notice(F("Temperature: %F C" CR), t);
  Log.notice(F("Humidity: %F %" CR), h);
  Log.notice(F("DS Temperature: %F C" CR), ds.getTempC());
  float voltage = analogRead(PIN_A1) * (3.3 / 1023.0);
  Log.notice(F("Voltage: %F" CR), voltage );

  
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("JOINING !!"));
  display.display();
  
Log.notice(F("Trying to join Lora ") );
 Log.notice( LORA );
   //delay(5000) ;
  connected = modem.connected();
  int joinFailed = 0;
  if ( !connected ) {
     joinFailed = 0;
    while ( !connected && joinFailed < 6 ) {
      connected = modem.joinOTAA(appEui, appKey,modem.deviceEUI().c_str());
      if (!connected) {
             display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("FAILED JOINED"));
    display.setCursor(0, 20);
  display.println("Retry "+String(joinFailed));
  display.display();
        Log.notice(F("LoRaWAN network not joined, retry join in 60 seconds ..." CR));
        blink(1);
        delay(60000);
        joinFailed++;
      }
    }
  }
  if(joinFailed>1)
    reboot();
  if ( connected ) {
      display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("JOINED !!"));
  display.display();
  
    Log.notice(F("LoRaWAN network joined" CR));
    modem.setPort(2);
    modem.dataRate(3);
    modem.setADR(true);
     // Set poll interval to 60 secs.
  modem.minPollInterval(60);
  // NOTE: independently by this setting the modem will
  // not allow to send more than one message every 2 minutes,
  // this is enforced by firmware and can not be changed.
  
    blink(3);
  }


  
}
void loop1() {
  Log.notice(F("Wake" CR));

  Log.notice(F("Water: %d" CR), analogRead(PIN_A4 ));


  delay(5000);
}
void loop() {

  display.ssd1306_command(SSD1306_DISPLAYON);
  Log.notice(F("---- In the LOOP -----" CR));
/*
  // Dump SAMD21 registers
  if ( Serial ) {
    if (! regsShown ) {
      ZeroRegOptions opts = { Serial, false };
      //   printZeroRegs(opts);
      regsShown = true;
    }
  } else {
    regsShown = false;
  }
*/

  //  digitalWrite(extSensorEnablePin, HIGH);
  // analogWrite(PIN_A0, 1024);  // apply voltage to DAC to test analog input
  // delay(10);
  // int extSensor = analogRead(extSensorPin);
  //int extSensor=0;
  //digitalWrite(extSensorEnablePin, LOW);
  //int err;

  Serial.println(" >>>> Mag Analog ="+String(analogRead(magnetSensorA)));
  Serial.println(" >>> Mag Dialog ="+String(digitalRead(magnetSensorD)));


  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  lightsensor = analogRead(A2);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(1000);
    // reboot();
    // return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  
  //gps();
  
  float t2 = ds.getTempC();
  Log.notice(F("Temperature: %F C" CR), t);
  Log.notice(F("Humidity: %F in Percent" CR), h);
  Log.notice(F("DS Temperature: %F C" CR), t2);
  float extSensor = analogRead(PIN_A1) * (5.0 / 1023.0);
  Log.notice(F("Voltage: %F" CR), extSensor );
  Log.notice(F("Water: %d" CR),  analogRead(PIN_A4) );
  Log.notice(F("Light: %d" CR), lightsensor );
  // Prepare Cayenne LPP
  lpp.reset();
  if ((msgCount & 1) == 0)
  {
     Log.notice(F("Even Msg ?: %d = short" CR), msgCount );
    lpp.addTemperature(1, t);
    lpp.addRelativeHumidity(1, h);
    lpp.addVoltage(1, extSensor );
    lpp.addAnalogInput(1, msgCount);
    lpp.addGenericSensor(1, analogRead(PIN_A4));
  //  lpp.addGPS(1, gps_latitude, gps_longitude, 10);
     lpp.addLuminosity(1, lightsensor);
   //     lpp.addBarometricPressure(8, 1023.4f);
    lpp.addTemperature(9, t2);
  // lpp.addGenericSensor(10, 0);
 
  
  } else {
 
  display.println("Temperature:"+String(t2));
 
  
    Log.notice(F("Odd Msg ?: %d = long" CR), msgCount );
  // lpp.addTemperature(1, t);
  // lpp.addRelativeHumidity(2, h);
   lpp.addVoltage(1, extSensor );
   lpp.addAnalogInput(1, msgCount);
  // lpp.addGenericSensor(5, analogRead(PIN_A4));
   lpp.addGPS(1, gps_latitude, gps_longitude, 10);
  // lpp.addLuminosity(7, lightsensor);
  
 //   lpp.addBarometricPressure(8, 1023.4f);
      lpp.addTemperature(1, t2);
 //            lpp.addGenericSensor(10, 1);
  //             lpp.addAccelerometer(11, 1.234f, -1.234f, 0.567f);
     
  display.println("Lat:"+String(gps_latitude)+" Lon:"+String(gps_longitude));
  Serial.println("Lat:"+String(gps_latitude)+" Lon:"+String(gps_longitude));

 
  }

 display.display();
  /*

    lpp.reset();
    lpp.addDigitalInput(1, 0);
    lpp.addDigitalOutput(2, 1);
    lpp.addAnalogInput(3, 1.23f);
    lpp.addAnalogOutput(4, 3.45f);
    lpp.addLuminosity(5, 20304);
    lpp.addPresence(6, 1);
    lpp.addTemperature(7, 26.5f);
    lpp.addRelativeHumidity(8, 86.6f);
    lpp.addAccelerometer(9, 1.234f, -1.234f, 0.567f);
    lpp.addBarometricPressure(10, 1023.4f);
    lpp.addGyrometer(1, -12.34f, 45.56f, 89.01f);
    lpp.addGPS(1, -12.34f, 45.56f, 9.01f);

    lpp.addUnixTime(1, 135005160);

    lpp.addGenericSensor(1, 4294967295);
    lpp.addVoltage(1, 3.35);
    lpp.addCurrent(1, 0.321);
    lpp.addFrequency(1, analogRead(PIN_A4));
    lpp.addPercentage(1, 100);
    lpp.addAltitude(1 , 50);
    lpp.addPower(1 , 50000);
    lpp.addDistance(1 , 10.555);
    lpp.addEnergy(1 , 19.055);
    lpp.addDirection(1 , 90);
    lpp.addSwitch(1 , 0);
  */
  // Send the data

  //DynamicJsonBuffer jsonBuffer;
 // JsonObject& root = jsonBuffer.createObject(lpp.getSize());

  StaticJsonDocument<512> jsonBuffer;

    // Create an array to parse data to
  JsonArray root = jsonBuffer.createNestedArray();

    // Call parser
    CayenneLPPDec::ParseLPP (lpp.getBuffer(), lpp.getSize(), root);

    // Print JSON data to serial
  serializeJsonPretty (root, Serial);
  
  Serial.println();
  
  modem.beginPacket();
  modem.write(lpp.getBuffer(), lpp.getSize());
  
  int err = modem.endPacket(true); //not confirmed
  if (err > 0) {
    Log.notice(F("LoRa message sent: %l size:  %l" CR), msgCount, lpp.getSize());

  display.setCursor(0, 8);
  display.println("LoRa Sent: "+String (msgCount));
  display.display();
    msgCount++;
    blink(2);
  } else {

   display.setCursor(0, 8);
  display.println("LoRa Error: "+String (msgCount));
  display.display();
    blink(1);
  }
  delay(1000);
  if (!modem.available()) {
    Log.notice("No downlink message received at this time." CR);

  } else {
    char rcv[64];
    int i = 0;
    while (modem.available()) {
      rcv[i++] = (char)modem.read();
    }
    Log.notice("Received: ");
    for (unsigned int j = 0; j < i; j++) {
     Serial.println(rcv[j] >> 4, HEX);
     Serial.println(rcv[j] & 0xF, HEX);
      Log.notice   (  "hex values     : %x " CR                  , rcv);
      Log.notice(rcv);
    }
    Log.notice(CR);
  }

  if (msgCount > 50)
    reboot();
  Serial.println("Waiting 300sec");
  display.setCursor(0, 16);
  display.println("Waiting 300sec");
  display.display();

  //Serial.println(SLEEP_PERIOD);
  delay(5000);

  Serial.println("before going to deepSleep");
  Serial.println("Filename = LoraFlasgv1DHT11DSOneSleep");
  Serial.print("Message Count: ");
   
  Serial.println(msgCount);
    Serial.print("LowPower.idle (ms) = ");
      Serial.println(SLEEP_PERIOD);
         display.setCursor(0, 24);
   display.println("Deepsleep="+ String(SLEEP_PERIOD));
   display.display();
   delay(5000);
  display.ssd1306_command(SSD1306_DISPLAYOFF);
 delay(60000);
    //   LowPower.idle  (SLEEP_PERIOD);
 //LowPower.deepSleep(SLEEP_PERIOD);
  //LowPower.idle(SLEEP_PERIOD);
  //USBDevice.attach();
  if ( !connected ) 
  {
      Serial.println("NOT connected");
    LowPower.deepSleep  (SLEEP_PERIOD);
    reboot();
  }
  Serial.println("Check lightsensor < 100");
  if (lightsensor < 100) {

    Serial.println("Looks like Night time to reboot - wait 5 min");
   display.setCursor(0, 27);
   display.println("Deepsleep in mili = "+ String(SLEEP_PERIOD));
   display.display();
   LowPower.deepSleep  (SLEEP_PERIOD);
    //pinMode(0, OUTPUT);
    //digitalWrite(0, HIGH);
    reboot();
  }
     
     
}
