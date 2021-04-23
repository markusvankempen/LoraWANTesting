#include "Arduino.h"
#include <ESP8266WiFi.h>
 
//Deeps/light sleep test
//https://diyprojects.io/esp8266-deep-sleep-mode-test-wake-pir-motion-detector/
// Important is a cable between D0 and RST


#define FPM_SLEEP_MAX_TIME           0xFFFFFFF
 
// Required for LIGHT_SLEEP_T delay mode
extern "C" {
#include "user_interface.h"
}
 
const char* ssid = "";
const char* password = "";
 
//The setup function is called once at startup of the sketch
void setup() {
  Serial1.begin(115200);
    Serial.begin(115200);
  while(!Serial) { }
 
  Serial.println();
  Serial.println("Start device in normal mode!");
 
  WiFi.mode(WIFI_STA);
 
  WiFi.begin(ssid, password);
  Serial1.println("");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(5000);
  
}
void callback() {
  Serial1.println("Callback");
  Serial.flush();
}
void WiFiOn() {

  wifi_fpm_do_wakeup();
  wifi_fpm_close();

  Serial.println("Reconnecting");
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
}


void WiFiOff() {

  Serial.println("diconnecting client and wifi");
  // client.disconnect();
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  // wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);

}

 
void lightsleep(int deepsleeptime) {
  
  Serial.println(F("\nSleep, for "));
  Serial.println(deepsleeptime);
  WiFiOff();
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
 
  extern os_timer_t *timer_list;
  timer_list = nullptr;  // stop (but don't disable) the 4 OS timers
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  //gpio_pin_wakeup_enable(GPIO_ID_PIN(0), GPIO_PIN_INTR_LOLEVEL);  // GPIO wakeup (optional)
  // only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
  // wifi_fpm_set_wakeup_cb(wakeupCallback); // set wakeup callback
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_open();
  //000000
  // The deepsleeptime can not be more than 99 it seems
  wifi_fpm_do_sleep(deepsleeptime * 1000000); // Sleep range = 10000 ~ 268,435,454 uS (0xFFFFFFE, 2^28-1)
   Serial.println("Delay");
   delay((deepsleeptime * 1000) + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep

  Serial.println(F("Woke up!"));  // the interrupt callback hits before this is executed
  // only fire wifi up if HB or temp change otherwise to to sleep

  WiFiOn();

} 

 
void loop() {
    Serial.println("ESP.deepSleep");
  ESP.deepSleep(10 * 1000000);
      Serial.println("Enter light sleep mode");
 
      lightsleep(10);
      Serial.println("Sleep one more time");
      lightsleep(10);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

   Serial.println("ESP.deepSleep");
  ESP.deepSleep(10 * 1000000);

}
