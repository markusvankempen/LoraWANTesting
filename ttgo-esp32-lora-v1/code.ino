// MIT License
// https://github.com/gonzalocasas/arduino-uno-dragino-lorawan/blob/master/LICENSE
// Based on examples from https://github.com/matthijskooijman/arduino-lmic
// Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

#include <Arduino.h>
#include "lmic.h"
#include <hal/hal.h>
#include <SPI.h>
#include <SSD1306.h>
#include <EEPROM.h>
#include <CayenneLPP.h>
CayenneLPP lpp(51);

// Added by Nick
//#include "ArduinoLowPower.h"
#define SERIAL_PORT_USBVIRTUAL 0

#define STATE_IDLE 0
#define STATE_OK_TO_SLEEP 1
#define STATE_TIME_TO_SEND 2
#define STATE_SENDING 3
uint8_t State = STATE_IDLE;

#define LEDPIN 2
#define OTHERLED 21

#define LMIC_DEBUG_LEVEL 1


#define OLED_I2C_ADDR 0x3C
#define OLED_RESET 16
#define OLED_SDA 4
#define OLED_SCL 15

unsigned int counter = 0;
char downlink[30];

// Structure to Store in EEPROM
struct {
  long deepsleep = 0;
  float temperature  = 0;
  int msg = 0;
} myeprom;


SSD1306 display (OLED_I2C_ADDR, OLED_SDA, OLED_SCL);

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes.

// Copy the value from Device EUI from the TTN console in LSB mode.
static const u1_t PROGMEM DEVEUI[8]= { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// Copy the value from Application EUI from the TTN console in LSB mode
static const u1_t PROGMEM APPEUI[8]= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is. Anyway its in MSB mode.
static const u1_t PROGMEM APPKEY[16] ={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 33, 32}  // Pins for the Heltec ESP32 Lora board/ TTGO Lora32 with 3D metal antenna
};
int frameCounter=0;

void do_send(osjob_t* j){
    // Payload to send (uplink)
    static uint8_t message[] = "Hello OTAA!";

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
       // LMIC_setTxData2(1, message, sizeof(message)-1, 0);
        Serial.println(F("Sending uplink packet..."));
        digitalWrite(LEDPIN, HIGH);
        display.clear();
        display.drawString (0, 0, "Sending/queued uplink packet...");
        display.drawString (0, 30,  "Frame ="+String (frameCounter));
        display.drawString (0, 50, String (++counter));
        display.display ();

    lpp.reset();
    lpp.addAnalogInput(1, frameCounter);   
    float measurement = (float) analogRead(34);
    float battery_voltage = (measurement / 4095.0) * 7.26;  
    lpp.addAnalogOutput(1,battery_voltage);

    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
    Serial.println(F("Packet queued"));
  frameCounter++;
  
    
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
           case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            display.clear();
            display.drawString (0, 0, "EV_TXCOMPLETE event!");

            if (LMIC.txrxFlags & TXRX_ACK) {
              Serial.println(F("Received ack"));
              display.drawString (0, 20, "Received ACK.");
            }

            if (LMIC.dataLen) {
              int i = 0;
              // data received in rx slot after tx
              Serial.print(F("Data Received: "));
              Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
              Serial.println();
              Serial.println(LMIC.rssi);

              display.drawString (0, 9, "Received DATA.");
              for ( i = 0 ; i < LMIC.dataLen ; i++ )
                downlink[i] = LMIC.frame[LMIC.dataBeg+i];
              downlink[i] = 0;
              display.drawString (0, 22, String(downlink));
              display.drawString (0, 32, String(LMIC.rssi));
              display.drawString (64,32, String(LMIC.snr));
            }
            State = STATE_OK_TO_SLEEP;
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            digitalWrite(LEDPIN, LOW);
            display.drawString (0, 50, "messge ="+String (counter));
            display.display ();
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING: -> Joining..."));
            display.drawString(0,16 , "OTAA joining....");
            display.display();
            break;
        case EV_JOINED: {
              Serial.println(F("EV_JOINED"));
              display.clear();
              display.drawString(0 , 0 ,  "Joined!");
              display.display();
              // Disable link check validation (automatically enabled
              // during join, but not supported by TTN at this time).
              LMIC_setLinkCheckMode(0);
            }
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }

}
float VBAT =0;
  
void setup() {
    Serial.begin(115200);
    delay(2500);                      // Give time to the serial monitor to pick up
    Serial.println(F("Starting..."));

   pinMode(21,OUTPUT);
 Serial.println(F("GPIO21  on..."));
   digitalWrite(OTHERLED, HIGH);
   delay(1000);
   digitalWrite(OTHERLED  , LOW);
    
   pinMode(LEDPIN,OUTPUT);
 Serial.println(F("LEDPIN  on..."));
      digitalWrite(LEDPIN, HIGH);
   delay(1000);
   digitalWrite(LEDPIN, LOW);
 
    // Use the Blue pin to signal transmission.
    pinMode(LEDPIN,OUTPUT);

    //Set up and reset the OLED
    pinMode(OLED_RESET, OUTPUT);
    digitalWrite(OLED_RESET, LOW);
    delay(50);
    digitalWrite(OLED_RESET, HIGH);

    /////////// POWER das nicht work
int vbatPin=34;
 pinMode(vbatPin, INPUT);
  // Battery Voltage
   VBAT = (float)(analogRead(vbatPin)) / 4095*2*3.3*1.1;
  /*
  The ADC value is a 12-bit number, so the maximum value is 4095 (counting from 0).
  To convert the ADC integer value to a real voltage you’ll need to divide it by the maximum value of 4095,
  then double it (note above that Adafruit halves the voltage), then multiply that by the reference voltage of the ESP32 which 
  is 3.3V and then vinally, multiply that again by the ADC Reference Voltage of 1100mV.
  */
  Serial.println("VBAT = "+String(VBAT));
  Serial.println("Vbat = "+String(VBAT));


  uint16_t v1 = analogRead(34);
uint16_t v2 = analogRead(14);

float battery_voltage = ((float)v1 / 4095.0) * 2.0 * 3.3 * (1100 / 1000.0);
float other_voltage = ((float)v2 / 4095.0) * 2.0 * 3.3 * (1100 / 1000.0);

String voltage = "PIN34 = "+String(battery_voltage) + "V PIN14 = " + String(other_voltage) + "V";

Serial.println(voltage);
float measurement = (float) analogRead(34);
 battery_voltage = (measurement / 4095.0) * 7.26;

Serial.println("Other voltage = "+String(battery_voltage));

  
    display.init ();
     display.clear ();
    display.flipScreenVertically ();
    display.setFont (ArialMT_Plain_10);

    display.setTextAlignment (TEXT_ALIGN_LEFT);

    display.drawString (0, 0, "Starting....");
    display.display ();
    print_wakeup_reason();
    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
/*
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    */
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.
    LMIC_selectSubBand(1);
    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    //LMIC.dn2Dr = DR_SF9;
    LMIC_setAdrMode (1);

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    //LMIC_setDrTxpow(DR_SF11,14);
   // LMIC_setDrTxpow(DR_SF9,14);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink
  LMIC_setDrTxpow(DR_SF7, 8);

    // Start job
    //do_send(&sendjob);     // Will fire up also the join
    //LMIC_startJoining();
    State = STATE_TIME_TO_SEND;

  EEPROM.begin(512);
  EEPROM.get(0, myeprom);
  Serial.print("myeprom.msg = ");
  Serial.println(myeprom.msg);
  frameCounter=myeprom.msg;
  
    if(myeprom.msg == -1 || myeprom.msg>1000)
    {
      
      Serial.println("Default EEPROM");
      myeprom.msg=0;
      EEPROM.put(0, myeprom);
      EEPROM.commit();
    }
      
 
  
}

void serialBegin(uint32_t baudrate){
  Serial.begin(baudrate);
  time_t timeout = millis();
  while (!Serial) if ((millis() - timeout) < 5000) delay(50); else break;
}

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  900          /* Time ESP32 will go to sleep (in seconds) */

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
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);myeprom.msg=1; break;
    
  }
}

float counterme=0;
int sendcnt=0;
void loop() {
   os_runloop_once(); 

       //Serial.println(F("loop..."));
        digitalWrite(LEDPIN, LOW);
        display.clear();
        display.drawString (0, 0, "Waiting for delivery");
        display.drawString (0, 20, "Messages send ...");
        display.drawString (80, 20, String (counter));
          display.drawString (0, 30, "Counter: "+String (counterme));
        display.display ();

if (State == STATE_OK_TO_SLEEP) {
    Serial.println(F("STATE_OK_TO_SLEEP"));
  //  delay(30000+(TX_INTERVAL*1000));
    //Set timer to 5 seconds
      myeprom.msg++;
      EEPROM.put(0, myeprom);
      EEPROM.commit();
  delay(5000);
  
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +" Seconds");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //Go to sleep now

  esp_deep_sleep_start();
  
    State = STATE_TIME_TO_SEND;
      counterme=0;
    //   ESP.restart();
       sendcnt++;
}
//
// Problem after 5 times seems to hang
//
if (State == STATE_TIME_TO_SEND) {
  Serial.println(F("STATE_TIME_TO_SEND"));
   do_send(&sendjob); 
   State=STATE_IDLE;

}

if(sendcnt>2)
ESP.restart();

counterme++;
if(counterme>500000)
{
  //State=STATE_OK_TO_SLEEP;
  Serial.println(F("Error Resend"));
  counterme=0;
   ESP.restart();
}
//do_send(&sendjob);  

/*
  if (State == STATE_OK_TO_SLEEP) {
    Serial.println(F("STATE_OK_TO_SLEEP"));
    digitalWrite(LEDPIN, LOW);
    Serial.end();
    
    for (uint16_t c = 0; c < 60; c++) {
      // uint32_t so 4294967296 so up to 48 days, days * hours * mins * seconds * millis
      //LowPower.sleep(1 * 1000); // Sleep for 10 mins 

        display.clear();
        display.drawString (0, 0, "Sleep.");
        display.display ();
     delay(1000);
   
      digitalWrite(LEDPIN, HIGH);         // Flash LED
      delay(10);                      // very very briefly
      digitalWrite(LEDPIN, LOW);          // every 5000 milliseconds
    }
    
    //serialBegin(115200);
    Serial.println(F("Awake"));
    State = STATE_TIME_TO_SEND;

    // NOTE: STATE_OK_TO_SLEEP set when TX_COMPLETE is returned onEvent

    // We don't just send here, give the os_runloop_once() a chance to run
    // and we keep the State machine pure

  } else if (State == STATE_TIME_TO_SEND) {
    Serial.println(F("STATE_TIME_TO_SEND"));
    digitalWrite(LEDPIN, HIGH);
    State =  STATE_SENDING;
    
    // Read sensors
    // Build payload
    
    do_send(&sendjob);
    Serial.println(F("STATE_SENDING"));
    
  } else if (State == STATE_SENDING) {
    Serial.println(F("STITLL sending STATE_SENDING"));
    // This section runs for the duration of the radio doing it's thing
    delay(5000);
    counterme++;
    if ((millis() & 256) != 0) digitalWrite(LEDPIN, HIGH); else  digitalWrite(LEDPIN, LOW); // Flash the LED
  }
  */
}
