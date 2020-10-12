// 
// Tutorial Link: https://learn.adafruit.com/the-things-network-for-feather/using-a-feather-32u4
//
// Added lpp and Oled functions 
/************************** Configuration ***********************************/
#include <TinyLoRa.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <CayenneLPP.h>
CayenneLPP lpp(51);

Adafruit_SSD1306 display = Adafruit_SSD1306();

// Visit your thethingsnetwork.org device console
// to create an account, or if you need your session keys.

// Network Session Key (MSB)
uint8_t NwkSkey[16] = { 0x28, 0x6B, 0xFB, 0xF8, 0x5F, 0xA2, 0xB3, 0x64, 0x10, 0xAB, 0xE0, 0x63, 0xCF, 0x34, 0x8E, 0x75 }; // Use UR KEYS

// Application Session Key (MSB)
uint8_t AppSkey[16] = { 0x60, 0xA4, 0x94, 0x20, 0x56, 0x8C, 0x50, 0xF2, 0xFB, 0x6E, 0x8A, 0xF7, 0x61, 0xDC, 0x94, 0xAF }; // Use UR KEYS

// Device Address (MSB)
uint8_t DevAddr[4] = { 0x26, 0x01, 0x1D, 0xF0 };

/************************** Example Begins Here ***********************************/
// Data Packet to Send to TTN
unsigned char loraData[11] = {"hello LoRa"};

// How many times data transfer should occur, in seconds
const unsigned int sendInterval = 30;

// Pinout for Adafruit Feather 32u4 LoRa
//TinyLoRa lora = TinyLoRa(7, 8, 4);

// Pinout for Adafruit Feather M0 LoRa
TinyLoRa lora = TinyLoRa(3, 8, 4);



void say(String s, String t, String u, String v) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(s);
  display.println(t);
  display.println(u);
  display.println(v);
  display.display();
}
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup()
{
  delay(2000);
  Serial.begin(9600);
  //while (! Serial);

  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)

  Serial.println(F("Starting"));
  display.clearDisplay();
  say( "Starting Lora","V1", "", "");
    delay(1000);  
  // Initialize pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);
  
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
      say( "Error","Lora start", "Failed", "We wait 5sec");
    delay(10000);
      say( "Reboot in 5 Sec","", "", "We had an error");
          delay(10000);
    NVIC_SystemReset();
  }

  // Optional set transmit power. If not set default is +17 dBm.
  // Valid options are: -80, 1 to 17, 20 (dBm).
  // For safe operation in 20dBm: your antenna must be 3:1 VWSR or better
  // and respect the 1% duty cycle.

  // lora.setPower(17);

  Serial.println("OK");
     say( "","OK", "", "");
       delay(1000);
}

void loop()
{
  Serial.println("Sending LoRa Data...");
   display.clearDisplay();
  //llp
          
           lpp.reset();
           lpp.addTemperature(1, 21);
           lpp.addAnalogInput(1, lora.frameCounter);
            
//            LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
 
            // send the 6 bytes payload to LoRaWAN port 7
         //   LMIC_setTxData2(7, lpp.getBuffer(), lpp.getSize(), 0);
             
  
  lora.sendData(lpp.getBuffer(), lpp.getSize(), lora.frameCounter);
  // Optionally set the Frame Port (1 to 255)
  // uint8_t framePort = 1;
  say( "Sending LoRa Data:","", "", "");
  // lora.sendData(loraData, sizeof(loraData), lora.frameCounter, framePort);
  Serial.print("Frame Counter: ");
  Serial.println(lora.frameCounter);
     display.clearDisplay();
   
  say( "","Frame Counter:"+String(lora.frameCounter), "","");
  lora.frameCounter++;
  delay(1000);
  // blink LED to indicate packet sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.println("delaying...");
   say( "","", "Next msg in "+String(sendInterval),"seconds");
  delay(sendInterval * 1000);
}
