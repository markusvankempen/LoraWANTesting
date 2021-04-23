/*
*
* This is for the TTGO with a SIM 7000G
* 
* I had a hard time get this to work. Information etc hare on various githubs
* https://github.com/vshymanskyy/TinyGSM/issues/448
* https://m2msupport.net/m2msupport/how-to-use-test-nb-iot-with-sim7000-modules/
* 
*  The sequence below finally helped ... i worked for Twilio,holgram and bell 
*  
*  mvk@ca.ibm.com / markus@vankempen.org - 2021-Apr-06
*  
*  
//Set the APN for the wireless connection. Please check the APN for your network
AT+CSTT="super" // twilio super SIM
WAIT=1
//Bring up Connection...
AT+CIICR
WAIT=3
//Get the local IP address
AT+CIFSR
WAIT=2
//Get the GNSS location information
AT+CIPPING="www.google.com"

******************************************************************************/
// Select your modem:
// #define TINY_GSM_MODEM_SIM868
#define TINY_GSM_MODEM_SIM7000  
// #define TINY_GSM_MODEM_SIM7600

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600

#include <TinyGsmClient.h>
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
#define uS_TO_S_FACTOR          1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP           60          /* Time ESP32 will go to sleep (in seconds) */

#define PIN_TX                  27
#define PIN_RX                  26
#define UART_BAUD               115200
#define PWR_PIN                 4
#define LED_PIN                 12
#define POWER_PIN               25

bool reply = false;

#include <ArduinoHttpClient.h>
#include <PubSubClient.h>

// Server details
const char server[] ="thinklab1239.mybluemix.net";
const char resource[] = "/sim";
const int  port = 80;
TinyGsmClient client(modem);
HttpClient http(client, server, port);
PubSubClient mqtt(client);


void modem_on()
{
    // Set-up modem  power pin
    Serial.println("\nStarting Up Modem...");
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(300);
    digitalWrite(PWR_PIN, LOW);
    delay(10000);

    int i = 10;
    Serial.println("\nTesting Modem Response...\n");
    Serial.println("****");
    while (i) {
        SerialAT.println("AT");
        delay(500);
        if (SerialAT.available()) {
            String r = SerialAT.readString();
            Serial.println(r);
            if ( r.indexOf("OK") >= 0 ) {
                reply = true;
                break;;
            }
        }
        delay(500);
        i--;
    }
    Serial.println("****\n");
}
 String res;
void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);
    delay(100);
   Serial.println("----------------------  START --------------------------");
    // Onboard LED light, it can be used freely
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // POWER_PIN : This pin controls the power supply of the SIM7600
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    // PWR_PIN ： This Pin is the PWR-KEY of the SIM7600
    // The time of active low level impulse of PWRKEY pin to power on module , type 500 ms
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(500);
    digitalWrite(PWR_PIN, LOW);

    delay(1000);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    DBG("Wait...");
    int retry = 5;
    while (!reply && retry--){
        modem_on();
    }

    delay(1000);
    Serial.println("=====SetAPN=====");
    modem.sendAT("+CSTT=\"super\"");
   // modem.sendAT("+CSTT=\"hologram\"");
    modem.waitResponse(1000L, res); 
   Serial.println(res);
      delay(500);

SerialAT.println("WAIT=1");
   
   delay(500);
    Serial.println("=====Startup=====");
    modem.sendAT("+CIICR");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(500);
SerialAT.println("WAIT=3");
      delay(500);
    Serial.println("=====Modem IP=====");
    modem.sendAT("+CIFSR");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);


    Serial.println("=====Modem IP=====");
    modem.sendAT("+CIFSR");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

    
      int i = 10;
  while (i) {
    SerialAT.println("AT");
    delay(500);
    if (SerialAT.available()) {
      String r = SerialAT.readString();
      Serial.println(r);
      if ( r.indexOf("OK") >= 0 ) break;;
    }
    delay(500);
    i--;
  }
  reply=true;

      Serial.println("=====info=====");
    modem.sendAT("+CGDCONT?");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

//AT+CGDCONT=1,"IP","hologram","0.0.0.0",0,0,0,0

      Serial.println("=====Bare=====");
    modem.sendAT("+SAPBR=3,1,\"APN\",\"super\"");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);


          Serial.println("=====Connection test=====");
    modem.sendAT("+SAPBR=2,1");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

      Serial.println("=====Connect=====");
    modem.sendAT("+SAPBR=1,1");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

           Serial.println("====HTTP init=====");
    modem.sendAT("+HTTPINIT");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

    Serial.println("====HTTP url=====");
    modem.sendAT("+HTTPPARA=\"URL\",\"http://thinklab1239.mybluemix.net/sim?A=1001\"");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

        Serial.println("====HTTP get=====");
    modem.sendAT("+HTTPACTION=0");
    modem.waitResponse(1000L, res); 
    Serial.println(res);
    delay(1000);

SerialMon.print(F("Connecting to "));
  SerialMon.print(server);
  if (!client.connect(server, port)) {
    SerialMon.println(" fail");
    //delay(10000);
   // return;
  }else{
  SerialMon.println(" success");
  }
   /*
  // Make a HTTP GET request:
  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  uint32_t timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      SerialMon.print(c);
      timeout = millis();
    }
  }
  SerialMon.println();

  // Shutdown

  client.stop();
  SerialMon.println(F("Server disconnected"));
reply=true;

  
    delay(1000);
         reply = true;
   // AT+CIPPING="www.google.com"
  
Serial.println("making POST request");
  String postData = "imei=32442&data=1";

  http.beginRequest();
  http.post("/");
  http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
  http.sendHeader("Content-Length", postData.length());
  http.sendHeader("X-Custom-Header", "custom-header-value");
  http.beginBody();
  http.print(postData);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  // Shutdown

  http.stop();
  SerialMon.println(F("Server disconnected"));
*/
       
    if (reply) {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consiquinces..."));
        Serial.println(F("***********************************************************\n"));
    } else {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
        Serial.println(F("***********************************************************\n"));
    }

  mqtt.setServer("snpweather.xyz", 1883);
  
}

void reconnect(){

  while (!mqtt.connected())
  {
    Serial.println("Attempting to connect the MQTT");

    if (mqtt.connect("", "", NULL))
    {
      Serial.println("Connected to the MQTT");
//      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      Serial.println("Failed!");
    //  digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Retrying in 5 seconds");
      delay(5000);
    }
    
    
  }
  
}
void loop()
{
   //if (!mqtt.connected())
  {
  //  reconnect();
  
  }

  //mqtt.loop();
  
    while (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    while (Serial.available()) {
        SerialAT.write(Serial.read());
    }
}
