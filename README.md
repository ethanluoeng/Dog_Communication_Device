# Dog Communication Device

<img src="Pictures/IMG_5929.jpg" width="400"> &nbsp; &nbsp; <img src="Pictures/IMG_5897.JPG" width="400">

## Intro
Oh, the guilt I feel when my dog is stuck inside! He lies by the screen door, gazing out at the backyard with wide, longing eyes... I’m building this project so he can hang out freely in our large backyard, even when everyone is busy.

So far, I’ve built a wearable device for dogs that play audio recordings to them from up to 100 meters away. Press a button on your phone, your dog hears the command and (if they’re smart enough), acts on it. It also has safety benefits, such as telling your dog to stand still if they are lost.

## How It Works

The firmware runs on ESP-IDF using separate FreeRTOS tasks:

- **Wi-Fi SoftAP:** the ESP32 creates its own Wi-Fi network that your phone connects to, and hosts the web page at `192.168.4.1`.
- **HTTP server:** serves the web page. Each button tap sends a clip request.
- **Queue:** the HTTP handler pushes the request into a FreeRTOS queue. Then, it returns immediately, keeping the web page responsive while audio plays.
- **Audio task:** waits on the queue for requests, looks up the embedded WAV, and streams the PCM samples over I2S to the amplifier.

## Project Structure

```
AudioPlayer+Wifi/
├── .gitignore
├── .vscode/                       # VSCode/PlatformIO settings
├── .pio/                          # build output, auto-generated
├── audio/                         # embedded WAV clips
│   ├── audio_1.wav
│   ├── audio_2.wav
│   ├── audio_3.wav
│   ├── audio_4.wav
│   ├── audio_5.wav
│   └── audio_6.wav
├── include/
├── lib/
├── managed_components/
│   └── joltwallet__littlefs/      # LittleFS component, downloaded automatically
├── src/
│   ├── CMakeLists.txt             # component registration and WAV embedding
│   └── main.c                     # Wi-Fi, HTTP server, and audio task
├── test/
├── CMakeLists.txt                 # top-level ESP-IDF project file
├── dependencies.lock
├── partitions.csv                 # custom partition table for 16 MB flash
├── platformio.ini                 # board overrides and embedded files
├── sdkconfig.defaults
└── sdkconfig.esp32-s3-devkitc-1   # generated ESP-IDF config
```

## Bill of Materials

| Part | Qty | Approx. cost (USD) | Purchase Link | Notes |
|---|---|---|---|---|
| ESP32-S3-DevKitC-1 | 1 | $10–15 | https://www.amazon.ca/dp/B0CKXJLP4B | 16 MB flash, 8 MB octal PSRAM. Clones like **N16R8** work fine |
| MAX98357A I2S amplifier breakout | 1 | $3–6 | https://www.amazon.ca/dp/B0F1LLQ6WJ | Class-D, 3 W into 4 Ω |
| 3 W 4 Ω speaker | 1 | $2–4 | https://www.amazon.ca/dp/B01LN8ONG4 ||
| Jumper wires / perfboard | – | – | |
| 3.7 V LiPo battery | 1 | $20 | https://www.amazon.ca/dp/B0F8MFGZZC | Interchangeable with other batteries |

## Wiring and Schematics

| MAX98357A | ESP32-S3 |
|---|---|
| BCLK | GPIO 5 |
| LRC | GPIO 6 |
| DIN | GPIO 7 |
| VIN | 5V |
| GND | GND |

## Getting Started

### Prerequisites
- [VSCode](https://code.visualstudio.com/)
- [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- ESP-IDF 6.0.1 (PlatformIO installs it automatically through the `espressif32` platform)

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/ethanluoeng/Dog_Communication_Device.git
   cd Dog_Communication_Device
   ```

2. **Record and add your own audio clips**
   - Use [Audacity](https://www.audacityteam.org/) or ffmpeg to convert your audio files to 16000Hz, 16-bit, mono .wav files.
   - Put the file in `audio/`.
   - Register it in **both** places:
      - `platformio.ini`, by adding it under `board_build.embed_files`
      - `src/CMakeLists.txt`, by adding it to `EMBED_FILES` in `idf_component_register(...)`
   - In `src/main.c`, declare the embedded symbols:

3. **Build & Flash on ESP32**:
   ```bash
   pio run                  
   pio run -t upload
   ```

### Usage
- Power on the device.
- On your phone, connect to the Wi-Fi network **`<SSID>`** (password: **`<password>`**). To change these, edit `<WIFI_SSID / WIFI_PASS>` in `src/main.c`.
- Open a browser and go to **http://192.168.4.1**.
- Tap a button to play that clip through the speaker.
