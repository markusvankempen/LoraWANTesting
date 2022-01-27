/*
  MKR ENV and MKR1310 Shields - Read Sensors

  This example reads the sensors on-board the MKR ENV shield
  and prints them to the Serial Monitor its sends the sensor data via LoraWAN in LPP format.
  After sending the data the MKR1310 goes in to deepsleep.
  Note: After the deepsleep you need the 1Sec delay to reconnect to serial.

  The circuit:
  - Arduino MKR board
  - Arduino MKR ENV Shield attached

  This example code is in the public domain.
  mvk@ca.ibm.com - 2021-Apr-05 / markus@vankempen.org

*/
#include <Arduino_MKRIoTCarrier.h>
MKRIoTCarrier carrier; //Constructor of the carrier maybe we can include it on the library itself

//#include <Arduino_MKRENV.h>
#include <CayenneLPP.h>
#include <MKRWAN.h>
#include <pins_arduino.h>
#include "ArduinoLowPower.h"
LoRaModem modem;
#include <Arduino_APDS9960.h>

// Adjust for you Network
String appEui = "70B3D57ED0040A0C";
String appKey = "1E9C687133EED21221D98016ABEA4E71";
String EUI = "1234567812345678"; //test

#include <FlashStorage.h>

// Reserve a portion of flash memory to store an "int" variable
// and call it "my_flash_store".
FlashStorage(my_flash_store, int);
String VERSION = "2022-Jan-25";
//   RAK7249 DevADR for this device is 0201352b
//  "devEUI": "a8610a33341f7116",

CayenneLPP lpp(51);
int sleeptime = 900000;
int errsleeptime = 300000;

void mydeepSleep(int t){
 //  delay(t);  
  LowPower.deepSleep(t);
  return;
}

bool carriererr=false;

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
 
  delay(2000);
  Serial.println("");
  Serial.println("---------START - 10 sec wait--------");
  delay(15000);
  // Read the content of "my_flash_store" and assign it to "number"
  int number = my_flash_store.read();
  Serial.print("FLash number =");
  Serial.println(number);
  
  // Print the current number on the serial monitor
  // my_flash_store.write(number + 1);
 
// carrier.noCase();

 // if (!ENV.begin()) {
  if (! carrier.begin()) {
    Serial.println("Failed to initialize MKR IoT Shield shield!");
    carriererr=true;
    // Serial.println("Going to SLEEP for 300 Sec");
    //mydeepSleep(errsleeptime);
    //    NVIC_SystemReset();
  }else{
     Serial.println("IoT Board ok - Set LED ");
     /* carrier.leds.setPixelColor(0,  0 ,  0 , 10);
      carrier.leds.show();
      delay(2000);
      carrier.leds.setPixelColor(0,  0 ,  0 , 0);
      carrier.leds.show();
      */
    }
  Serial.print("Version:");
  Serial.println(VERSION);
  Serial.println("Welcome to MKRWAN1310 and Iot Opla Shield with LoraConfig for US915");
  // change this to your regional band (eg. US915, AS923, ...)
    Serial.print("Region:");
  Serial.println("US915_HYBRID");
// if (!modem.begin(US915)) {
 if (!modem.begin(US915_HYBRID)) {
    Serial.println("Failed to start LORA module");
    Serial.println("Going to SLEEP for 300 Sec and reboot");
    mydeepSleep(errsleeptime);
    NVIC_SystemReset();
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  Serial.println("Trying to Join LoraWAN with");
  Serial.println("APP EUI =" + appEui);
  Serial.println("APP KEY =" + appKey);
  delay(1000);
  int connected = modem.joinOTAA(appEui, appKey,EUI);
  Serial.print("connected? = ");
  Serial.println(connected);

  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    Serial.println("Going to SLEEP for 300 Sec and reboot");
    mydeepSleep(errsleeptime);
    NVIC_SystemReset();
  }
  Serial.println("Connected and Joined to LORAWan ");

 // modem.setPort(4);
  modem.dataRate(3);
  modem.setADR(true);
  delay(1000);

}
float temperature = 0;
float humidity    = 0;
float pressure    = 0;
float illuminance = 0;
float uva         = 0;
float uvb         = 0;
float uvIndex     = 0;
float Gx, Gy, Gz;
float Ax, Ay, Az;
int light;
int r,g,b;
float battery_voltage=0;
int errenv = 0;
int errlora = 0;
int msgcnt = 0;
int cmd =0;
int para = 0;


void loop() {

   Serial.println("In Loop");
   
   if (!carriererr) {
    Serial.println("Set LED and Display Test");

  //carrier.leds.setPixelColor(0,  0 ,  0 , 20);
  //carrier.leds.setPixelColor(1,  0 , 20 , 0 );
  //carrier.leds.setPixelColor(2, 20 ,  0 , 0 );
  //carrier.leds.setPixelColor(3,  0 , 20 , 20);
  //carrier.leds.setPixelColor(4, 20 , 20 , 20);
  carrier.leds.setPixelColor(0,  0 ,  0 , 10);
  carrier.leds.show();
  delay(1000);
    carrier.leds.setPixelColor(0,  0 ,  0 , 0);
  carrier.leds.show();
  //Function to display
  displayTitle(5000);

   }


  
 // if (!ENV.begin()) {
   if (carriererr) {
    Serial.println("Failed to initialize MKR IoT Shield !");
    temperature = 0;
    humidity    = 0;
    pressure    = 0;
    illuminance = 0;
    light = 0;
    uva         = 0;
    uvb         = 0;
    uvIndex     = 0;
    errenv++;
  } else {

/* MKR1310 needs a voltage devider 
     delay(100);
  int Batt = analogRead(ADC_BATTERY);
   delay(100);
 Serial.print("batt read:");
 Serial.println(Batt);
  int batteryLevel = map(Batt, 0, 1023, 0, 420); //need callibration agaist AREF
  Serial.print("batt Volts:");
  Serial.println(batteryLevel);
  */
    // read all the sensor values
    temperature = carrier.Env.readTemperature();
    humidity    = carrier.Env.readHumidity();
    pressure    = carrier.Pressure.readPressure();
  carrier.Light.readColor(r,g, b, light);
  Serial.println("Ambient light sensor");
  Serial.print("\t light: ");
  Serial.println(light);
  // read the color
  carrier.Light.readColor(r, g, b);

  // print the values
  Serial.print("r = ");
  Serial.println(r);
  Serial.print("g = ");
  Serial.println(g);
  Serial.print("b = ");
  Serial.println(b);

    Serial.print("proximity = ");
  Serial.println(carrier.Light.readProximity());

    int illuminanceInt;
    int none;
      Serial.print("illuminanceInt = ");
  carrier.Light.readColor(none, none, none, illuminanceInt);
  Serial.println(illuminanceInt);
  if(APDS.colorAvailable()){
    
    Serial.print("APDS.colorAvailable = ");

      APDS.readColor(r, g, b);

  // print the values
  Serial.print("r = ");
  Serial.println(r);
  Serial.print("g = ");
  Serial.println(g);
  Serial.print("b = ");
  Serial.println(b);
  Serial.println();
  
    }
  
  Serial.println();
  
     //IMU
  //Gyroscope
  Serial.println("IMU module");
  carrier.IMUmodule.readGyroscope(Gx, Gy, Gz);
  Serial.println("Gyroscope:");
  Serial.print ("\t X:");
  Serial.println(Gx);
  Serial.print ("\t Y:");
  Serial.println(Gy);
  Serial.print ("\t Z:");
  Serial.println(Gz);

  //Accelerometer
  carrier.IMUmodule.readAcceleration(Ax, Ay, Az);
  Serial.println("Accelerometer:");
  Serial.print ("\t X:");
  Serial.println(Ax);
  Serial.print ("\t Y:");
  Serial.println(Ay);
  Serial.print ("\t Z:");
  Serial.println(Az);
  

  }
  // print each of the sensor values
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity    = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Pressure    = ");
  Serial.print(pressure);
  Serial.println(" kPa");

  Serial.print("Illuminance = ");
  Serial.print(illuminance);
  Serial.println(" lx");

  Serial.print("ENV Error   = ");
  Serial.println(errenv);

  Serial.print("Lora Error  = ");
  Serial.println(errlora);

  Serial.print("MSG counter = ");
  Serial.println(msgcnt);

    Serial.print("Sleeeptime = ");
  Serial.println(sleeptime);
  
  battery_voltage = 0;//analogRead(ADC_BATTERY);
  Serial.print("voltage   = ");
  Serial.println(battery_voltage);

  // Prepare Cayenne LPP
  lpp.reset();
  lpp.addTemperature(1, temperature); 
  lpp.addBarometricPressure(1, pressure);
  lpp.addRelativeHumidity(1, humidity);
  lpp.addLuminosity(1, light);
  lpp.addAnalogInput(1, uvIndex);
  lpp.addAnalogOutput(3, errenv);
  lpp.addAnalogOutput(2, errlora);
  lpp.addAnalogOutput(4, msgcnt);
  lpp.addAnalogOutput(1, int(sleeptime/1000));

  
  Serial.print("Sending Lora Package! Size=");
  Serial.println(lpp.getSize());
  delay(1000) ;
  modem.beginPacket();
  modem.write(lpp.getBuffer(), lpp.getSize());
  //modem.print(msg);
  int err = modem.endPacket(true);
  Serial.print("Packsend result =");
      Serial.println(err);
  if (err > 0) {
    Serial.println("Message sent correctly!");
      digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
    msgcnt++;
  } else {
    Serial.println(err);
    errlora++;
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }

  if(errlora>10)
  {
      Serial.println("Reset too many errors :(");
      errlora=0;
      NVIC_SystemReset();
  }    

  if(msgcnt>100)
  {
      Serial.println("Reset after 200 message ;) ");
      msgcnt=0;
      NVIC_SystemReset();
  }  

  delay(2000);
  if (!modem.available()) {
    Serial.println("No downlink message received at this time.");

  } else {
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
    // downlink commands
    // CMD A1 10 = change sleep time to 10*16*10000 =160000
    cmd = rcv[0] >> 4;
    cmd = cmd + (rcv[0] & 0xF);
    para = 0;
    Serial.println("");
    Serial.print("cmd=");
    Serial.println(cmd);  



    if (cmd == 10) // reboot
    {
      Serial.println("Received command to reboot");
      NVIC_SystemReset();

    }
    if (cmd == 11) // change sleep time
    {
      para = (rcv[1] >> 4) * 16;        //AA = 170   90 = 0x5A  /for every 15min A1 5A
      para = para + (rcv[1] & 0xF);
      Serial.print("para=");
      Serial.println(para);

      Serial.print("Received command to change deep sleep time(10000*para) to ");
      Serial.println(10000 * para); 
      sleeptime= 10000*para;

    }

  }
  Serial.println("display off");
// display off
      carrier.display.fillScreen(ST77XX_BLACK);
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, LOW);
    
  Serial.print("DeepSleep (in milli sec) for ");
  Serial.println(sleeptime);
  delay(1000);
  USBDevice.detach();
  delay(5000);
  Serial.println("Serial Detached");
  //LowPower.deepSleep(sleeptime);
  mydeepSleep(sleeptime);//sleeptime);
  //  modem.sleep();
  // wait 1 second to print get serial back again
  USBDevice.attach();
  delay(5000);
  Serial.println("");
  Serial.println("Awake");
}


void displayTitle(int d) {
  Serial.println("displayTitle");
  carrier.display.fillScreen(ST77XX_BLACK);

  carrier.display.setCursor(80, 120);
  carrier.display.setTextColor(ST77XX_RED);
  carrier.display.print("MKR ");
  carrier.display.setTextColor(ST77XX_GREEN);
  carrier.display.print("IoT ");
  carrier.display.setTextColor(ST77XX_MAGENTA);
  carrier.display.print("Carrier");
  carrier.display.setCursor(105, 130);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.print("Library");
  delay(d);
  carrier.display.enableDisplay(false);
  carrier.display.fillScreen(ST77XX_BLACK);
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, LOW);

   // digitalWrite(TFT_BACKLIGHT, HIGH);
    
}
