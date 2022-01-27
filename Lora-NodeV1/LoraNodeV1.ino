// Hello LoRa - for NodeLoraV1

/************************** Configuration ***********************************/
#include <TinyLoRa.h>
#include <SPI.h>
#include <LowPower.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
Adafruit_BME680 bme; // I2C
#include <CayenneLPP.h>
CayenneLPP lpp(51);


// Network Session Key (MSB)
uint8_t NwkSkey[16] = { 0x28, 0x6B, 0xFB, 0xF8, 0x5F, 0xA2, 0xB3, 0x64, 0x10, 0xAB, 0xE0, 0x63, 0xCF, 0x34, 0x8E, 0x71 };
//286BFBF85FA2B36410ABE063CF348E75
//286bfbf85fa2b36410abe063cf348e70

// Application Session Key (MSB)
uint8_t AppSkey[16] = { 0x60, 0xA4, 0x94, 0x20, 0x56, 0x8C, 0x50, 0xF2, 0xFB, 0x6E, 0x8A, 0xF7, 0x61, 0xDC, 0x94, 0xA1 };
//60A49420568C50F2FB6E8AF761DC94AF
//60a49420568c50f2fb6e8af761dc94a0

// Device Address (MSB)
uint8_t DevAddr[4] = { 0x26, 0x01, 0x36, 0xA1 };

/************************** Example Begins Here ***********************************/
// Data Packet to Send to TTN
unsigned char loraData[11] = {"hello LoRa"};

// How many times data transfer should occur, in seconds
const unsigned int sendInterval = 30;

// Pinout for Adafruit Feather 32u4 LoRa
//TinyLoRa lora = TinyLoRa(7, 8, 4);

// Pinout for Adafruit Feather M0 LoRa
//TinyLoRa lora = TinyLoRa(3, 8, 4);

//TinyLoRa (int8_t rfm_dio0, int8_t rfm_nss, int8_t rfm_rst)
TinyLoRa lora = TinyLoRa(2, 10, 9);


void setup()
{
  delay(15000);
  Serial.begin(115200);
  while (! Serial);
    delay(5000);
  // Initialize pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
   delay(5000);
  // Initialize LoRa
  Serial.print("Starting LoRa...");
  // define multi-channel sending
  lora.setChannel(MULTI);
  // set datarate
  lora.setDatarate(SF7BW125);
  if(!lora.begin())
  {
    Serial.println("Failed");
    Serial.println("Check your radio");

;
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);
  digitalWrite(LED_BUILTIN, LOW);
    while(true);
  }


  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
  }else{

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
  }
  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");

  Serial.print("Gas = ");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" KOhms");
  }//bem680
  // Optional set transmit power. If not set default is +17 dBm.
  // Valid options are: -80, 1 to 17, 20 (dBm).
  // For safe operation in 20dBm: your antenna must be 3:1 VWSR or better
  // and respect the 1% duty cycle.

  // lora.setPower(17);

  Serial.println("OK");
    Serial.print("DevAddr:");
  // Serial.println((char)DevAddr);
  for (int i = 0; i < sizeof(DevAddr); i++)
    printHex(DevAddr[i]);
  Serial.println();
/*
  Serial.print("NwkSkey:");
  for (int i = 0; i < sizeof(NwkSkey); i++)
    printHex(NwkSkey[i]);
  Serial.println();

  Serial.print("AppSkey:");
  for (int i = 0; i < sizeof(AppSkey); i++)
    printHex(AppSkey[i]);
  Serial.println();

  //Serial.println(getHexString(AppSkey));
  */
}


String getHexString(uint8_t* num)
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

void printHex(uint8_t num) {
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

unsigned int counter=0;
void loop()
{
  
  
  Serial.println("Sending LoRa Data (LPP) ...");
  //lora.sendData(loraData, sizeof(loraData), lora.frameCounter);
  // Optionally set the Frame Port (1 to 255)
  // uint8_t framePort = 1;
  //lora.sendData(loraData, sizeof(loraData), lora.frameCounter, framePort);
  Serial.print("Frame Counter: ");Serial.println(lora.frameCounter);
  float t=0;
  float h=0;
  float p=0;
  float g=0; 
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");

  }else{
    t = bme.temperature;
    h = bme.humidity;
    p = bme.pressure / 100.0;
    g = bme.gas_resistance / 1000.0;
    }

  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" *C");

  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.println(" %");

    lpp.reset();
    lpp.addTemperature(1, t);
    lpp.addRelativeHumidity(1, h);
    lpp.addBarometricPressure(1,p);
    lpp.addAnalogInput(1, lora.frameCounter);
    lpp.addAnalogOutput(1, g);
 
    Serial.print("Sending: ");
    lora.sendData(lpp.getBuffer(), lpp.getSize(), lora.frameCounter);
    for (int i = 0; i < sizeof(lpp.getBuffer()); i++)
      printHex(lpp.getBuffer()[i]);
    Serial.println();
     Serial.print("Packet Size =");
     Serial.println(lpp.getSize());

    lora.frameCounter++;
  // blink LED to indicate packet sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
  digitalWrite(LED_BUILTIN, LOW);
 delay(5000);
  Serial.println("delaying...");
 delay(1000);
  // ATmega328P, ATmega168
// 4 hours = 60x60x4 = 14400 s
// 14400 s / 8 s = 1800
unsigned int sleepCounter;
for (sleepCounter = 80; sleepCounter > 0; sleepCounter--)
{
  //Serial.println("powerDown ="+sleepCounter);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
}
counter++;

 Serial.println("A wake");

 if(counter >10)
 {
   Serial.println("Reset");
 resetFunc();
 }
 
}
