# Smart Guide System for the Visually Impaired

## 📌 Project Overview

The **Smart Guide System** is an IoT-based assistive technology designed to help visually impaired individuals navigate their surroundings safely and independently. The system combines **camera-based computer vision, sensors, GPS tracking, voice feedback, and emergency communication** to provide real-time environmental information and alerts.

The system uses a **Raspberry Pi Zero 2W** for image processing and an **ESP32** for sensor interfacing and communication.

## 🎯 Objectives

* Assist visually impaired individuals during navigation.
* Detect traffic signals and provide voice alerts.
* Detect obstacles and unsafe areas such as ditches or pits.
* Provide location information using GPS.
* Detect falls using an MPU6050 sensor.
* Provide emergency SMS functionality.
* Deliver important alerts through voice feedback.

## 🏗️ System Architecture

```text
                 ┌──────────────────────┐
                 │      Camera          │
                 └──────────┬───────────┘
                            │
                            ▼
                 ┌──────────────────────┐
                 │ Raspberry Pi Zero 2W │
                 │  Image Processing    │
                 │  Computer Vision     │
                 └──────────┬───────────┘
                            │
                       Serial / UART
                            │
                            ▼
                 ┌──────────────────────┐
                 │        ESP32         │
                 │ Sensor Processing &  │
                 │ Communication        │
                 └───────┬───────┬──────┘
                         │       │
             ┌───────────┘       └────────────┐
             ▼                                ▼
        ┌─────────┐                     ┌─────────┐
        │ MPU6050 │                     │ GPS     │
        │ Fall    │                     │ Neo-6M  │
        │Detection│                     │Tracking │
        └─────────┘                     └─────────┘
             │
             ▼
        ┌─────────┐
        │ VL53L0X │
        │ Distance│
        │ Sensor  │
        └─────────┘

        ESP32 ──────────────► SIM800L
                               │
                               ▼
                         Emergency SMS

                 Raspberry Pi
                       │
                       ▼
                Voice Feedback
                       │
                       ▼
                Wired / Bluetooth
                    Earphone
```

## 🔧 Hardware Components

| Component            | Purpose                                     |
| -------------------- | ------------------------------------------- |
| Raspberry Pi Zero 2W | Image processing and main vision processing |
| ESP32                | Sensor interfacing and communication        |
| Camera               | Capturing real-time environmental images    |
| MPU6050              | Fall detection                              |
| VL53L0X              | Distance and obstacle detection             |
| Neo-6M GPS           | Location tracking and navigation            |
| SIM800L              | Emergency SMS communication                 |
| Earphone / Headset   | Voice feedback                              |
| Power Supply         | System power                                |

## 💻 Software & Technologies

* **Python**
* **Embedded C**
* **OpenCV**
* **NumPy**
* **Picamera2**
* **Raspberry Pi OS**
* **Arduino IDE**
* **ESP32**
* **UART Communication**
* **GPS**
* **SIM800L AT Commands**

## 🚀 Key Features

### 1. Traffic Light Detection

The camera captures the surrounding environment and the Raspberry Pi processes the image to identify traffic light conditions.

The system provides voice notifications such as:

* Red light detected
* Green light detected

### 2. Obstacle and Ditch Detection

The **VL53L0X Time-of-Flight sensor** measures the distance between the user and nearby objects.

The system can provide alerts when an obstacle or unsafe area is detected.

### 3. Fall Detection

The **MPU6050 accelerometer and gyroscope** are used to monitor sudden changes in motion and identify possible falls.

When a fall condition is detected, the system can trigger an emergency response.

### 4. GPS Tracking

The **Neo-6M GPS module** provides the user's geographical coordinates.

The system can use predefined locations and provide voice-based location notifications.

### 5. Emergency SMS

The **SIM800L GSM module** provides emergency communication.

When an emergency condition such as a fall is detected, the system can send an SMS containing relevant information to a predefined contact.

### 6. Voice Feedback

Important system events are converted into audio feedback so that the user can receive information without depending on a visual display.

## 🔄 Working Principle

1. The camera continuously captures images of the user's surroundings.
2. The Raspberry Pi Zero 2W processes the images using computer vision techniques.
3. Traffic lights and other relevant environmental conditions are identified.
4. The ESP32 continuously monitors connected sensors.
5. The VL53L0X detects nearby obstacles and distance variations.
6. The MPU6050 monitors motion for fall detection.
7. The Neo-6M provides GPS coordinates.
8. Sensor information is exchanged between the ESP32 and Raspberry Pi.
9. Important events are converted into voice alerts.
10. In an emergency condition, the ESP32 communicates with the SIM800L to send an SMS.


## 📡 Communication

The system uses communication between the Raspberry Pi and ESP32 to exchange sensor and system information.

```text
Raspberry Pi Zero 2W
        │
        │ UART
        ▼
      ESP32
        │
        ├── MPU6050
        ├── VL53L0X
        ├── Neo-6M GPS
        └── SIM800L
```

## 📍 Applications

* Assistive navigation for visually impaired individuals
* Obstacle awareness
* Outdoor navigation assistance
* Emergency fall notification
* Location monitoring
* Smart wearable assistive systems

## 🔮 Future Enhancements

* Advanced AI-based object detection
* Improved road and obstacle classification
* Offline voice recognition
* More accurate navigation assistance
* Cloud-based location monitoring
* Mobile application integration
* Improved wearable design and power optimization

## 👨‍💻 Project Type

**IoT | Embedded Systems | Computer Vision | Assistive Technology**

## 📜 License

This project is developed for **educational and research purposes**. You may modify and improve the project with appropriate attribution.

