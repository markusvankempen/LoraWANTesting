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

#include <Arduino_MKRENV.h>
#include <CayenneLPP.h>
#include <MKRWAN.h>
#include <pins_arduino.h>
#include "ArduinoLowPower.h"
LoRaModem modem;
// Adjust for you Network
String appEui = "70B3D57ED0040A0C";
String appKey = "1E9C687133EED21221D98016ABEA4E79";

String VERSION = "20210405";
//   RAK7249 DevADR for this device is 0201352b
//  "devEUI": "a8610a33341f7116",

CayenneLPP lpp(51);
int sleeptime = 10000;


void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  delay(3000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  //while (!Serial);
  delay(3000);
  Serial.println("");
  Serial.println("---------START--------");
  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    // Serial.println("Going to SLEEP for 300 Sec");
    //LowPower.deepSleep(300000);
    //    NVIC_SystemReset();
  }
  Serial.print("Version:");
  Serial.println(VERSION);
  Serial.println("Welcome to MKRWAN1310 and ENV with LoraConfig for US915");
  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(US915_HYBRID)) {
    Serial.println("Failed to start LORA module");
    Serial.println("Going to SLEEP for 300 Sec and reboot");
    LowPower.deepSleep(300000);
    NVIC_SystemReset();
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  Serial.println("Trying to Join LoraWAN with");
  Serial.println("APP EUI =" + appEui);
  Serial.println("APP KEY =" + appKey);

  int connected = modem.joinOTAA(appEui, appKey);

  Serial.println(connected);

  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    Serial.println("Going to SLEEP for 300 Sec and reboot");
    LowPower.deepSleep(300000);
    NVIC_SystemReset();
  }
  Serial.println("Connected and Joined to LORAWan ");

  modem.setPort(11);
  modem.dataRate(3);
  modem.setADR(true);

}
float temperature = 0;
float humidity    = 0;
float pressure    = 0;
float illuminance = 0;
float uva         = 0;
float uvb         = 0;
float uvIndex     = 0;
 float battery_voltage=0;
int errenv = 0;
int errlora = 0;
int msgcnt = 0;

void loop() {
  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    temperature = 0;
    humidity    = 0;
    pressure    = 0;
    illuminance = 0;
    uva         = 0;
    uvb         = 0;
    uvIndex     = 0;
    errenv++;
  } else {
    // read all the sensor values
    temperature = ENV.readTemperature();
    humidity    = ENV.readHumidity();
    pressure    = ENV.readPressure();
    illuminance = ENV.readIlluminance();
    uva         = ENV.readUVA();
    uvb         = ENV.readUVB();
    uvIndex     = ENV.readUVIndex();
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

  Serial.print("UVA         = ");
  Serial.println(uva);

  Serial.print("UVB         = ");
  Serial.println(uvb);

  Serial.print("UV Index    = ");
  Serial.println(uvIndex);

  Serial.print("ENV Error   = ");
  Serial.println(errenv);

  Serial.print("Lora Error  = ");
  Serial.println(errlora);

  Serial.print("MSG counter = ");
  Serial.println(msgcnt);
  
  //battery_voltage = analogRead(ADC_BATTERY);
 //  Serial.print("voltage   = ");
 //  Serial.println(battery_voltage);
   
  // Prepare Cayenne LPP
  lpp.reset();
  lpp.addTemperature(1, temperature);
  lpp.addBarometricPressure(1, pressure);
  lpp.addRelativeHumidity(1, humidity);
  lpp.addLuminosity(1, uvIndex);
  lpp.addDigitalOutput(1, errenv);
  lpp.addAnalogOutput(1, errlora);
  lpp.addDigitalInput(1, msgcnt);

  Serial.print("Sending Lora Package! Size=");
  Serial.println(lpp.getSize());

  modem.beginPacket();
  modem.write(lpp.getBuffer(), lpp.getSize());

  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent correctly!");
    msgcnt++;
  } else {
    Serial.println(err);
    errlora++;
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
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
    int cmd = rcv[0] >> 4;
    cmd = cmd + (rcv[0] & 0xF);
    int para = 0;
    Serial.println("");
    Serial.print("cmd=");
    Serial.println(cmd);  

    if (cmd == 10) // reboot
    {
      Serial.println("Received command to reboot");
      NVIC_SystemReset();

    }
    if (cmd = 11) // change sleep time
    {
      para = (rcv[1] >> 4) * 16;        //AA = 170 
      para = para + (rcv[1] & 0xF);
      Serial.print("para=");
      Serial.println(para);

      Serial.print("Received command to change deep sleep time(10000*para) to ");
      Serial.println(10000 * para);
      sleeptime= 10000*para;

    }

  }
  Serial.println();

  Serial.print("DeepSleep (in milli sec) for ");
  Serial.println(sleeptime);
  USBDevice.detach();
  delay(5000);
  Serial.println("Serial Detached");
  LowPower.deepSleep(sleeptime);
  //  modem.sleep();
  // wait 1 second to print get serial back again
  USBDevice.attach();
  delay(5000);
  Serial.println("");
  Serial.println("Awake");
  return;
}
