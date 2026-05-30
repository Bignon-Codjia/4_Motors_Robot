#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP32Servo.h> 

// Creation of instances to control each motor independently
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Definition of the ESP32 output pins (PWM). 
// These pins will generate the square wave signal that indicates the angle to the servo.
const int pinS1 = 13;
const int pinS2 = 12;
const int pinS3 = 14;
const int pinS4 = 26;

// Pin 2 corresponds to the built-in blue LED on most ESP32 boards
const int pinLED = 2;

// Definition of the data "mold". It is crucial that the memory footprint 
// (the number and type of variables) is strictly identical between 
// the Transmitter and the Receiver for the packet to be decoded correctly.
typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

// Instantiation of the structure to store the received values
struct_message armData;

// --- CALLBACK FUNCTION ---
// This function does not run in the loop(). It is triggered asynchronously 
// (hardware interrupt) by the Wi-Fi module as soon as a packet is validated.
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  
  // Memory transfer: we raw-copy the bytes (incomingData) 
  // into our formatted structure (armData) to be able to read the variables.
  memcpy(&armData, incomingData, sizeof(armData));
  
  // 1. Visual debugging on the Serial Monitor
  Serial.print("\nPacket received (");
  Serial.print(len);
  Serial.print(" bytes) -> ");
  Serial.print("S1: "); Serial.print(armData.angle1);
  Serial.print("° | S2: "); Serial.print(armData.angle2);
  Serial.print("° | S3: "); Serial.print(armData.angle3);
  Serial.print("° | S4: "); Serial.println(armData.angle4);

  // 2. Physical visual indicator
  // Read the current state of the pin (0 or 1) and invert this state with '!'. 
  // This makes the LED blink at the exact packet reception frequency.
  digitalWrite(pinLED, !digitalRead(pinLED)); 

  // 3. Hardware control
  // The ESP32 updates the duty cycle of the PWM signals 
  // on the 4 pins to force the servos to reach the new position.
  servo1.write(armData.angle1);
  servo2.write(armData.angle2);
  servo3.write(armData.angle3);
  servo4.write(armData.angle4);
}

void setup() {
  Serial.begin(115200);
  
  // Configuration of the internal LED pin as an output
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW); // Turn off by default

  // --- RADIO CONFIGURATION ---
  // WIFI_STA (Station) mode disables Access Point mode (WIFI_AP).
  // This frees up processor resources and dedicates the antenna to ESP-NOW reception.
  WiFi.mode(WIFI_STA);
  
  // ESP-NOW does not have a channel scanning mechanism. Therefore, we force 
  // the radio hardware to listen exclusively to the specific frequency of channel 1 (2412 MHz).
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // --- HARDWARE TIMER CONFIGURATION ---
  // The ESP32 has PWM generators independent of the central processor.
  // We allocate 4 distinct hardware channels (timers) here to guarantee 
  // a very stable signal without making the servos jitter.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // The standard frequency for classic analog servos is 50 Hz 
  // (one signal sent every 20 milliseconds).
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo3.setPeriodHertz(50);
  servo4.setPeriodHertz(50);

  // Attaching pins to the timers. 
  // The parameters 500 and 2400 are the minimum and maximum pulse widths in microseconds.
  // They define the physical limits of the servos (0 degrees = 500us, 180 degrees = 2400us).
  servo1.attach(pinS1, 500, 2400);
  servo2.attach(pinS2, 500, 2400);
  servo3.attach(pinS3, 500, 2400);
  servo4.attach(pinS4, 500, 2400);

  // Initialization to the safe position to avoid mechanical shocks at startup.
  servo1.write(70);
  servo2.write(70);
  servo3.write(100);
  servo4.write(0);

  // --- STARTING THE PROTOCOL ---
  // Initialization of the ESP-NOW software layer.
  if (esp_now_init() != ESP_OK) {
    Serial.println("Hardware error during ESP-NOW initialization.");
    return; // Stops the setup function if the radio module does not respond
  }
  
  // We tell the network software layer which function to execute 
  // when a new data buffer is full.
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("Initialization complete. Waiting for network packets...");
}

void loop() {
  // The main processor remains idle here.
  // The ESP32 handles Wi-Fi reception on its own hardware core or via interrupts.
}