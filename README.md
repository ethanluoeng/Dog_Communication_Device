# Dog Communication Device

<img src="Pictures/IMG_5929.jpg" width="400"> &nbsp; &nbsp; <img src="Pictures/IMG_5897.JPG" width="400">

## Intro
Oh, the guilt I'd feel when my dog is confined inside; he lies next to the screen door, his wide longing eyes gazing out at the outdoors - the guilt is too much! I am building this project because I want my dog to be able to hangout freely in our large backyard even when everyone else is busy. 

So far, I've built a wearable device for dogs that can play audio recordings to them from up to 100 meters away. Whatever command you want to tell your dog, you press a button on your phone and your dog will hear it and act accordingly (if they're smart enough). This also has safety benefits, such as telling your dog to stand still when they are lost. 

Lastly, this project gives me an excuse to spend more quality time bothering my dog, which is time well spent.

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
