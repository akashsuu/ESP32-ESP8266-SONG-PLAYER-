# USB-Powered Spotify Remote

This project is a permanently USB-powered ESP32 remote for Windows media
controls. It sends commands by NRF24L01 PA+LNA radio to an ESP8266 NodeMCU
receiver, which writes them over USB serial to the included Python tray app.
The app converts the commands to Windows media keys for Spotify, the Spotify
web player, VLC, YouTube Music, and other media applications.

```
ESP32 remote -> NRF24L01 PA+LNA -> NRF24L01 PA+LNA -> ESP8266 -> USB serial -> Python -> Windows media keys
```

The remote has no battery circuitry and does not enter deep sleep. It remains
powered by its USB cable while in use.

## Features

- ESP32-WROOM-DA remote with a 128x64 I2C SSD1306 OLED.
- Six active-low buttons: Next, Previous, Play/Pause, Volume Up, Volume Down,
  and Mute.
- Non-blocking debounce, long-press About screen, and volume hold-repeat.
- NRF24 auto-ACK, CRC-16, retries, packet counter, checksum, device ID,
  heartbeat, reconnect, and estimated link quality from TX/ACK results.
- No RSSI calls: the OLED's Link percentage is an ACK-success estimate.
- ESP8266 receiver and Python controller remain unchanged and compatible.

## ESP32 Remote wiring

All grounds must be connected together. Wire each button between its GPIO and
GND; the firmware uses `INPUT_PULLUP`.

| Device | ESP32 pin | Notes |
|---|---:|---|
| OLED SDA | GPIO21 | I2C data |
| OLED SCL | GPIO22 | I2C clock |
| OLED VCC | GPIO2 or 3.3 V | GPIO2 is the configured display-power pin |
| NRF24 CE | GPIO4 | |
| NRF24 CSN | GPIO5 | |
| NRF24 SCK | GPIO18 | VSPI clock |
| NRF24 MOSI | GPIO23 | VSPI MOSI |
| NRF24 MISO | GPIO19 | VSPI MISO |
| Next | GPIO32 | button to GND |
| Previous | GPIO33 | button to GND |
| Play/Pause | GPIO25 | button to GND; hold for About |
| Volume Up | GPIO26 | button to GND; hold to repeat |
| Volume Down | GPIO27 | button to GND; hold to repeat |
| Mute | GPIO14 | button to GND |

Power the NRF24L01 PA+LNA from **3.3 V only**, never 5 V. Put a 10-100 uF
capacitor directly across its VCC and GND pins. The PA+LNA module can have high
current spikes, so use a stable 3.3 V source and short connections.

## Arduino Web Editor upload

1. Create/open a sketch named `remote` and upload every file from `Remote/` as
   a matching sketch tab. Do not add `battery.cpp` or `battery.h`.
2. Select **ESP32 Dev Module** for the ESP32-WROOM-DA remote.
3. Install RF24 (TMRh20), Adafruit SSD1306, Adafruit GFX, and Adafruit BusIO.
4. Select the ESP32 Arduino core used by the Web Editor (the remote supports
   the project's 2.0.17 core) and upload.
5. Upload `Receiver/receiver.ino` to a **NodeMCU 1.0 (ESP-12E Module)** with
   `nrf_rx.cpp`, `nrf_rx.h`, `config.h`, and `protocol.h` in the same sketch,
   and RF24 installed.
6. Confirm `DEVICE_ID`, `ENCRYPTION_KEY`, RF channel, and receiver address
   match in `Remote/config.h` and `Receiver/config.h` before flashing.

The OLED shows boot progress, receiver search/connection state, estimated link
quality, USB-powered status, the last command, button feedback, and About.

## Python application

On Windows, install Python 3.9+ and run:

```bat
cd Python
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

The application retains serial auto-detection, reconnect, system tray,
logging, and media-key control. The receiver commands are `NEXT`, `PREVIOUS`,
`PLAY`, `VOLUP`, `VOLDOWN`, and `MUTE`.

## Folder structure

```
SpotifyRemote/
├── Remote/       ESP32 USB-powered remote firmware
├── Receiver/     ESP8266 NRF24-to-serial receiver firmware (.ino + .cpp module)
├── Python/       Windows tray media-key controller
└── docs/         diagrams and diagram generator
```

`Remote/` contains only `remote.ino`, `globals.h`, `config.h`, `protocol.h`,
`nrf_comm.h/.cpp`, `buttons.h/.cpp`, and `ui.h/.cpp`.
