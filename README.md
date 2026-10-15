# Dog Communication Device

<img src="Pictures/IMG_5929.jpg" width="400"> &nbsp; &nbsp; <img src="Pictures/IMG_5897.JPG" width="400">

## Intro
Oh, the guilt I feel when my dog is stuck inside! He lies by the screen door, gazing out at the backyard with wide, longing eyes... I’m building this project so he can hang out freely in our large backyard, even when everyone is busy.

So far, I’ve built a wearable device for dogs that play audio recordings to them from up to 100 meters away. Press a button on your phone, your dog hears the command and (if they’re smart enough), acts on it. It also has safety benefits, such as telling your dog to stand still if they are lost.

## Overview
The ESP32-S3 does three things simultaneously:
- Creates its own WiFi network (SoftAP) that your phone connects to. (You do not need internet or a router)
- Serves a HTML webpage.
- Plays a WAV file through the speaker when a button is pressed.

## Features
- Play audio clips to your dog from far away without internet, using esp32 built-in wifi module
- Play those clips from even even further using HaLow radio (in progress)

## Hardware
ESP32-S3, MAX98357A I2S Class-D amplifier, 3 W speaker, and 3.7 V LiPo Battery

## Software
Wrote the firmware in C on ESP-IDF with PlatformIO/VSCode, using separate FreeRTOS tasks to self-host a Wi-Fi SoftAP and HTTP server and to execute I2S audio playback.

## License
© 2026 ethanluoeng. All rights reserved.
