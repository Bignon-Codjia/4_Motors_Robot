# 4 Motors ESP32 Wireless Robot Arm Controller (ESP-NOW)

![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif)
![Protocol: ESP-NOW](https://img.shields.io/badge/Protocol-ESP--NOW-red?style=for-the-badge)
![Language: C++](https://img.shields.io/badge/Language-C++-green?style=for-the-badge&logo=c%2B%2B)

An ultra-responsive, low-latency wireless remote control system designed for a 4-Degree-of-Freedom (4-DOF) robotic arm. Utilizing Espressif's proprietary **ESP-NOW protocol**, this setup eliminates the need for local Wi-Fi routers, establishing a direct, high-speed connection between an analog control rig and the robot arm's servos.

---

## Key Features

- **Zero-Router Latency:** Employs peer-to-peer ESP-NOW protocol over a fixed 2.4 GHz radio frequency for instant control execution.
- **Jitter-Free Performance:** Real-time signal smoothing using an Exponential Moving Average (EMA) low-pass filter on the analog inputs.
- **Independent Hardware Timing:** Leverages separate hardware channels via `ESP32Servo` to supply ultra-stable PWM signals to 4 independent motors simultaneously.
- **Asynchronous Execution:** Intercepts incoming network packets using hardware interrupts, decoupling signal processing from standard MCU loop limitations.

---

## Hardware Mapping

### 1. Transmitter Unit (Control Rig)
Reads 4 linear potentiometers to determine desired arm positions.

| Component | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **Potentiometer 1** | GPIO 32 | Joint Angle 1 Control |
| **Potentiometer 2** | GPIO 33 | Joint Angle 2 Control |
| **Potentiometer 3** | GPIO 34 | Joint Angle 3 Control |
| **Potentiometer 4** | GPIO 35 | Joint Angle 4 Control |

### 2. Receiver Unit (Robot Arm)
Translates incoming packets into direct structural coordinates.

| Component | ESP32 Pin | Timer Channel | Target Arm Limits |
| :--- | :--- | :--- | :--- |
| **Servo 1** | GPIO 13 | Timer 0 | 10° to 90° |
| **Servo 2** | GPIO 12 | Timer 1 | 0° to 180° |
| **Servo 3** | GPIO 14 | Timer 2 | 0° to 170° |
| **Servo 4** | GPIO 26 | Timer 3 | 0° to 180° |
| **Status LED** | GPIO 2 | N/A | Toggles with each data packet |

---

## Step-by-Step Installation & Setup

### Step 1: Extract the Receiver MAC Address
ESP-NOW demands explicit MAC addressing for structural targets.
1. Flash `Esp32_Mac_Address.ino` onto your **Receiver ESP32**.
2. Open the Serial Monitor (115200 baud) and copy the printed MAC Address.

### Step 2: Configure and Deploy the Transmitter
1. Open `Robot_Transmitter.ino`.
2. Locate the peer array and substitute your copied target MAC addresses:
   ```cpp
   uint8_t broadcastAddress[] = {0x00, 0x06, 0x00, 0x00, 0x00, 0x00};
3. Upload the code to your **Transmitter ESP32**.

### Step 3: Deploy the Receiver

1. Upload `Robot_Receiver.ino` to your **Receiver ESP32**.
2. Keep the unit connected to a stable external 5V/2A power supply capable of sustaining mechanical currents.


## 🛠 How the Code Works (Deep Dive)

### 1. The Shared Data Mold (Struct)

Both the transmitter and receiver share the exact same struct. This is crucial. When sending data over radio, the ESP32 sends a stream of raw bytes. By defining a strict memory footprint on both sides, the receiver knows exactly how to reassemble those bytes back into integer variables.

```cpp
typedef struct struct_message {
  int angle1;
  int angle2;
  int angle3;
  int angle4;
} struct_message;

```

### 2. The Transmitter: Exponential Smoothing Filter

Analog potentiometers can be noisy. If we send raw ADC (Analog-to-Digital Converter) data directly to the servos, the robotic arm will jitter and shake. To fix this, the code uses a software low-pass filter:

```cpp
filteredVal1 = (SMOOTHING * raw1) + ((1.0 - SMOOTHING) * filteredVal1);

```

* If `SMOOTHING` is `0.1`, the arm moves very smoothly but feels a bit sluggish (high inertia).


* If `SMOOTHING` is `0.5`, it provides a perfect balance of fast reaction time while filtering out electrical noise.



### 3. The Transmitter: Safety Mapping

The ESP32's ADC reads values from 0 to 4095. Before sending, these are mapped to safe physical angles for the robotic arm. This prevents the servos from trying to push past their physical limits and stripping their plastic or metal gears.

```cpp
armData.angle1 = map((int)filteredVal1, 0, 4095, 10, 90); // Constrained mapping

```

### 4. The Receiver: Hardware PWM Timers

The standard way of generating PWM (Pulse Width Modulation) in software can cause servos to twitch if the processor is busy handling Wi-Fi data. This project solves that by allocating the ESP32's internal hardware timers.

```cpp
ESP32PWM::allocateTimer(0); // ... Allocates dedicated channels
servo1.attach(pinS1, 500, 2400);

```

This offloads the signal generation to dedicated hardware, ensuring a perfectly stable 50Hz square wave is sent to the servos, completely independent of the main CPU loop.

### 5. The Receiver: Asynchronous Callback

The receiver's `loop()` function is completely empty. The magic happens in the `OnDataRecv` callback function.

```cpp
esp_now_register_recv_cb(OnDataRecv);

```

When a valid packet hits the Wi-Fi antenna, it triggers a hardware interrupt. The ESP32 pauses whatever it is doing, instantly copies the memory (`memcpy`) , writes the new angles to the servos , blinks the onboard LED to confirm reception, and goes back to sleep. This ensures zero-lag performance.

---

