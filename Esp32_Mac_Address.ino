#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Gives the serial port a moment to stabilize

  // Resets and forcefully activates the Wi-Fi
  WiFi.disconnect(true);
  delay(100); 
  WiFi.mode(WIFI_STA); 
  delay(100); // Gives the antenna time to activate

  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // The program runs in a continuous loop here, 
  // even if there is nothing to do for the moment.
}