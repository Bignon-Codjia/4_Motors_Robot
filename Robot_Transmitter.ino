#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// 🔴 REPLACE WITH THE MAC ADDRESS OF THE RECEIVER ESP32 🔴
uint8_t broadcastAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const int pot1 = 32;
const int pot2 = 33;
const int pot3 = 34;
const int pot4 = 35;

typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

struct_message armData;
esp_now_peer_info_t peerInfo;

// --- VARIABLES FOR THE SMOOTHING FILTER ---
// The smoothing factor (between 0.0 and 1.0)
// 0.1 = Very smooth but a bit slow (high inertia)
// 0.5 = More responsive but less filtered
// 1.0 = No smoothing (jitters)
const float SMOOTHING = 0.5; 

// Variables to store the filtered values (as float for precision)
float filteredVal1 = 0;
float filteredVal2 = 0;
float filteredVal3 = 0;
float filteredVal4 = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  
  esp_now_add_peer(&peerInfo);
  
  // Initialize the filters with the first reading to avoid a sudden jump at startup
  filteredVal1 = analogRead(pot1);
  filteredVal2 = analogRead(pot2);
  filteredVal3 = analogRead(pot3);
  filteredVal4 = analogRead(pot4);
}

void loop() {
  // 1. Raw reading of the potentiometers
  int raw1 = analogRead(pot1);
  int raw2 = analogRead(pot2);
  int raw3 = analogRead(pot3);
  int raw4 = analogRead(pot4);

  // 2. Application of the low-pass filter (Exponential smoothing)
  filteredVal1 = (SMOOTHING * raw1) + ((1.0 - SMOOTHING) * filteredVal1);
  filteredVal2 = (SMOOTHING * raw2) + ((1.0 - SMOOTHING) * filteredVal2);
  filteredVal3 = (SMOOTHING * raw3) + ((1.0 - SMOOTHING) * filteredVal3);
  filteredVal4 = (SMOOTHING * raw4) + ((1.0 - SMOOTHING) * filteredVal4);

  // 3. Mapping of the filtered values to the arm angles
  // We convert the float (filteredVal) to an int at the time of mapping()
  armData.angle1 = map((int)filteredVal1, 0, 4095, 10, 90);
  armData.angle2 = map((int)filteredVal2, 0, 4095, 0, 180);
  armData.angle3 = map((int)filteredVal3, 0, 4095, 0, 170);
  armData.angle4 = map((int)filteredVal4, 0, 4095, 0, 180);

  // 4. Sending the data
  esp_now_send(broadcastAddress, (uint8_t *) &armData, sizeof(armData));
  
  // Necessary pause to let the ADC and Wi-Fi breathe
  delay(50); 
}