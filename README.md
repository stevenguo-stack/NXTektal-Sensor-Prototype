# NXTektal Sensor Prototype

Sensor prototype developed for NXTektal's robotic platform.

This repository contains the Arduino firmware used to test and integrate a VL53L4CX Time-of-Flight (ToF) distance sensor with a Heltec Wireless Tracker development board.

## Current Prototype

The current prototype can:

- Initialize the VL53L4CX ToF sensor
- Communicate with the sensor through I2C
- Continuously monitor measurement readiness
- Detect objects
- Measure object distance in millimeters
- Report sensor measurement status through Serial Monitor
- Restart measurements continuously

## Hardware

### Main Controller
- Heltec Wireless Tracker

### Distance Sensor
- STMicroelectronics VL53L4CX
- Time-of-Flight (ToF) ranging sensor

## Pin Configuration

Current firmware configuration:

| Function | Heltec GPIO |
|---|---:|
| SDA | GPIO 45 |
| SCL | GPIO 46 |
| XSHUT | GPIO 44 |
| GND | GND |
| Power | Sensor-compatible supply |

> Always disconnect power before changing jumper-wire connections.

## Communication

The sensor communicates with the Heltec board using I2C.

Current configuration:

```cpp
#define SDA_PIN 45
#define SCL_PIN 46
#define XSHUT_PIN 44
