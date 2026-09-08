# Dog Communication Device

<img src="Pictures/IMG_5929.jpg" width="400"> &nbsp; &nbsp; <img src="Pictures/IMG_5897.JPG" width="400">

## Intro


## Overview


## Features
- Play audio clips to your dog from far away without internet, using esp32 built-in wifi module
- Play those clips from even even further using HaLow radio (in progress)

## Hardware
ESP32-S3, MAX98357A I2S Class-D amplifier, 3 W speaker, and 3.7 V LiPo Battery

## Software
Wrote the firmware in C on ESP-IDF with PlatformIO/VSCode, using separate FreeRTOS tasks to self-host a Wi-Fi SoftAP and HTTP server and to execute I2S audio playback.

## Getting Started

### Prerequisites

### Installation

### Usage


## License
© 2026 ethanluoeng. All rights reserved.
