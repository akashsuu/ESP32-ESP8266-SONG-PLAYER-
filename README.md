# Wireless Spotify Remote - ESP32 + ESP8266 + NRF24L01 + OLED

A **professional, production-ready wireless remote** that controls Spotify (desktop or web player), YouTube Music, VLC and Windows Media Player on Windows — **without any browser extension, Spotify API, cloud service or internet dependency**.

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐      ┌─────────────────┐
│  ESP32 Remote │  ⇄  │ NRF24L01 PA  │ ⇄ ⇄ │ NRF24L01 PA  │  ⇄  │ ESP8266 Receiver │
│  OLED + Buttons│ SPI │ + LNA (TX)  │ RF  │ + LNA (RX)   │ SPI │ NodeMCU USB → PC │
└──────────────┘      └──────────────┘      └──────────────┘      └──────┬──────────┘
                                                                          │ USB Serial 115200
                                                              ┌───────────▼───────────┐
                                                              │  Python App (tray)     │
                                                              │  → Windows media keys   │
                                                              └───────────┬───────────┘
                                                                          ▼
                                                          Spotify · YouTube Music ·
                                                                  VLC · WMP
```

- **Remote**: ESP32-WROOM-DA + NRF24L01 PA+LNA + 0.96" SSD1306 OLED + 6 buttons + optional LiPo battery.
- **Receiver**: ESP8266 NodeMCU (ESP-12E) + NRF24L01 PA+LNA, USB-powered (CH340), streams commands to a Windows Python app over 115200 baud serial.
- **PC app**: auto-detects the receiver, runs in the system tray, maps commands to Windows multimedia keys.

---

## Table of Contents

1. [Features](#features)
2. [Hardware List](#hardware-list)
3. [Wiring Diagrams & Pin Mapping](#wiring-diagrams--pin-mapping)
4. [Power Recommendations](#power-recommendations)
5. [Wireless Protocol](#wireless-protocol)
6. [OLED Screens](#oled-screens)
7. [Library Installation (Arduino)](#library-installation-arduino)
8. [Flashing the Firmware](#flashing-the-firmware)
9. [Python Installation](#python-installation)
10. [Running the System](#running-the-system)
11. [Windows Python App](#windows-python-app)
12. [Folder Structure](#folder-structure)
13. [Troubleshooting Guide](#troubleshooting-guide)
14. [FAQ](#faq)
15. [Future Improvements](#future-improvements)
16. [License & Credits](#license--credits)

---

## Features

### Hardware
- 6 push buttons with **software debounce (25 ms)**, **short press**, **long press** and **hold-to-repeat** (volume).
- 0.96" SSD1306 OLED (128×64) with **8 screens**: Boot, Home, Button Animation, Searching, Connected, Error, Battery, About, Sleep.
- **Battery monitoring** (voltage, percentage, low-battery warning, battery icon), automatic **deep sleep** after 60 s idle, **wake on any button**.
- NRF24L01 **PA+LNA** link: channel **108** (2.508 GHz), **250 kbps**, **auto-ACK**, **CRC-16**, **retries**, **PA max (remote) / PA high (receiver)**, **dynamic payloads**, ACK payloads for receiver status.

### Reliability
- Heartbeat every 5 s, link timeout detection (15 s), automatic reconnect, packet retries, duplicate/corrupt/unknown-device rejection, link quality estimation (TX/ACK success → percent + 5 bars), receiver watchdog, packet acknowledgements.

### Security
- Device pairing by **device ID**, **XOR stream encryption** with a configurable shared key, unknown devices rejected.

### Windows Python app
- pyserial with **auto COM detection**, **auto reconnect**, system tray icon (pystray + Pillow), balloon notifications, hidden startup, **Start with Windows** (registry, no admin), settings window with **dark mode**, COM port selection, Reconnect button, daily rotating logs (`logs/daily.log`).

---

## Hardware List

| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | ESP32-WROOM-DA dev board | 1 | Remote |
| 2 | ESP8266 NodeMCU (ESP-12E) dev board | 1 | Receiver (USB dongle) |
| 3 | NRF24L01 **PA+LNA** module (SMA antenna) | 2 | High-power long-range variant |
| 4 | 0.96" SSD1306 OLED, I2C (128×64) | 1 | Remote only |
| 5 | Tactile push buttons | 6 | Remote |
| 6 | 1S LiPo 3.7 V (400–2000 mAh) *optional* | 1 | Remote battery |
| 7 | AMS1117-3.3 LDO regulator *recommended* | 1 | Dedicated NRF supply (remote) |
| 8 | 10 µF electrolytic + 100 nF ceramic caps | 4+ | At regulator and at NRF VCC |
| 9 | 10–100 µF electrolytic cap *recommended* | 1 | Across receiver NRF VCC/GND |
| 10 | Resistors 100 kΩ ×2 (battery divider) | 2 | Remote |
| 11 | USB cables / data | 1 | Receiver ↔ PC |
| 12 | P-channel MOSFET (e.g. AO3401) *optional* | 1 | Cut NRF power in deep sleep |

> The ESP32-WROOM-DA is the **dual-antenna** variant of the ESP32-WROOM module; no special handling is required — it behaves like any ESP32.

---

## Wiring Diagrams & Pin Mapping

Full graphical diagrams are in [`docs/`](docs/):

| Diagram | Description |
|---------|-------------|
| [`docs/Circuit.png`](docs/Circuit.png) | Functional wiring diagram |
| [`docs/Flowchart.png`](docs/Flowchart.png) | Remote + receiver firmware flowcharts |
| [`docs/Architecture.png`](docs/Architecture.png) | System architecture overview |

### Remote: ESP32 ↔ NRF24L01 PA+LNA

The PA+LNA module works over the ESP32 **VSPI** bus. CE/CSN are configurable in `Remote/config.h`.

| NRF24 pin | ESP32 GPIO | Notes |
|-----------|------------|-------|
| VCC | **AMS1117-3.3 OUT** (dedicated 3.3 V rail) | Peak current ~250 mA — never feed from the board 3V3 pin alone |
| GND | GND (common rail) | Short, thick connection |
| CSN | **GPIO5** | Chip select |
| CE | **GPIO4** | Chip enable |
| SCK | **GPIO18** | VSPI clock |
| MOSI | **GPIO23** | Master out |
| MISO | **GPIO19** | Master in |
| IRQ | — (optional) | Leave unconnected |

Place **10 µF + 100 nF** capacitors right at the NRF VCC/GND pins.

### Receiver: ESP8266 NodeMCU ↔ NRF24L01 PA+LNA

The ESP8266 SPI bus is **hardware-fixed** to D5/D6/D7 (SCK/MISO/MOSI); only CE/CSN are configurable in `Receiver/config.h`. The NodeMCU's internal pulldown on D8 (GPIO15) makes it boot-safe as CSN. Feed the NRF from the NodeMCU **3V3** pin and place a **10–100 µF electrolytic cap** across the NRF VCC/GND to handle PA current spikes.

| NRF24 pin | NodeMCU pin | GPIO | Notes |
|-----------|-------------|------|-------|
| VCC | **3V3** | — | + 10–100 µF cap across VCC/GND |
| GND | GND | — | Common ground |
| CE | **D2** | GPIO4 | Chip enable |
| CSN | **D8** | GPIO15 | Chip select (boot-safe pulldown) |
| SCK | **D5** | GPIO14 | Fixed SPI clock |
| MOSI | **D7** | GPIO13 | Fixed SPI master out |
| MISO | **D6** | GPIO12 | Fixed SPI master in |
| IRQ | — | — | Leave unconnected |

### ESP32 ↔ OLED SSD1306 (remote)

| OLED pin | ESP32 GPIO | Notes |
|----------|------------|-------|
| VCC | 3V3 (or `OLED_POWER_PIN` GPIO2 for true off in sleep) | |
| GND | GND | |
| SDA | **GPIO21** | I2C data |
| SCL | **GPIO22** | I2C clock |

I2C address **0x3C**, bus clock 400 kHz.

### ESP32 ↔ Buttons (remote)

Active-low with the ESP32 **internal pull-ups** (`INPUT_PULLUP`) — wire each button from its GPIO to GND. All pins are **RTC-capable** so they can wake the ESP32 from deep sleep.

| Button | ESP32 GPIO |
|--------|------------|
| Next Track | **GPIO32** |
| Previous Track | **GPIO33** |
| Play / Pause | **GPIO25** |
| Volume + | **GPIO26** |
| Volume – | **GPIO27** |
| Mute | **GPIO14** |

### Battery (remote, optional)

| Battery | Component | ESP32 GPIO |
|---------|-----------|------------|
| + | 100 kΩ (R1) | **GPIO36** (ADC1_CH6) |
| + ─ R1 ─ GPIO36 ─ R2 (100 kΩ) ─ GND | divider 2:1 | |
| – | | GND rail |

`BAT_DIVIDER_RATIO = 2.0` matches the 100 k/100 k divider. If no battery is connected (USB only), the reading falls below `BAT_MIN_PLAUSIBLE_MV` and the display shows **"Power USB"** instead of a percentage.

---

## Power Recommendations

1. **NRF24L01 PA+LNA needs a clean 3.3 V supply.** The PA amplifier draws spikes up to ~250 mA. On the **remote**, feed it from an **AMS1117-3.3** whose input comes from 5 V, with **10 µF electrolytic + 100 nF ceramic** directly at the module's VCC/GND. On the **receiver**, the NodeMCU 3V3 regulator is sufficient at PA_HIGH with a **10–100 µF electrolytic cap** across the NRF VCC/GND. This prevents brownouts and resets.
2. **Grounding**: one common ground rail for USB 5 V, ESP32, NodeMCU, NRF, OLED, buttons and battery. Keep the NRF GND trace short.
3. **Remote on battery**: the dev board's 3.3 V regulator powers the ESP32 + OLED (fine). If you add the AMS1117 rail for the NRF, it can share the battery too.
4. **Deep sleep**: the remote powers the OLED off (`OLED_POWER_PIN`) and puts the NRF into power-down. For minimum quiescent current, add the optional **P-channel MOSFET** between 3.3 V and NRF VCC, controlled by `NRF_POWER_PIN` (config.h). ESP32 deep sleep then draws only a few µA.
5. **Receiver PA level**: `Receiver/config.h` uses **PA_HIGH** by default (safe for the NodeMCU's USB-supplied regulator). The remote keeps **PA_MAX** — each side's TX power is independent.

---

## Wireless Protocol

### Radio settings (`config.h` on both sides)

| Setting | Value |
|---------|-------|
| Channel | 108 (2.508 GHz) |
| Data rate | 250 kbps |
| TX power | PA_MAX (remote) / PA_HIGH (receiver) |
| CRC | 16-bit |
| Auto-ACK | on |
| Retries | 5 × 250 µs, 15 attempts |
| Payloads | dynamic (20 B packets + 4 B ACK payloads) |

### Packet layout — 20 bytes, little-endian

| Offset | Size | Field | Purpose |
|--------|------|-------|---------|
| 0 | 1 | magic (0x5A) | cheap garbage filter |
| 1 | 2 | deviceId | pairing (rejects unknown devices) |
| 3 | 2 | packetNumber | duplicate rejection window |
| 5 | 4 | timestampMs | remote uptime; reboot detection |
| 9 | 1 | command | NEXT / PREVIOUS / PLAY / VOLUP / VOLDOWN / MUTE / HEARTBEAT / CONNECT / STATUS |
| 10 | 1 | flags | encrypted / heartbeat |
| 11 | 2 | crc16 | CRC-16/XMODEM over bytes 0-10 + 13-18 |
| 13 | 2 | fwVersion | firmware version (or battery+link-quality in STATUS) |
| 15 | 4 | counter | global monotonic counter (replay protection) |
| 19 | 1 | checksum | XOR of bytes 0-18 |

Bytes **5..18 are XOR-obfuscated** with a keystream derived from the packet number and the shared 16-byte key (`ENCRYPTION_KEY` in both `config.h` files). Receiver validation order: magic → deviceId → decrypt → checksum → CRC → counter/timestamp monotonicity → duplicate window → dispatch.

**Receiver rejects:** duplicate packets, corrupted packets (bad checksum/CRC), replays (stale counter), unknown devices (wrong deviceId).

### Serial protocol (receiver → PC, 115200 baud)

| Line | Meaning |
|------|---------|
| `READY` | boot acknowledgement |
| `INFO:fw=1.0.0,dev=A1B2` | identity |
| `LINK:UP` / `LINK:DOWN` | radio link state change |
| `NEXT/PREVIOUS/PLAY/VOLUP/VOLDOWN/MUTE` | media command (bare line) |
| `STATUS:batt=94,vol=4112,quality=96` | battery + link quality report |
| `HEARTBEAT` | link alive (every 5 s) |
| `PONG` | reply to PC `PING` |
| `WARN:...` | diagnostics |

> **Backward compatible:** the PC app also accepts the legacy `CMD:NEXT...` prefixed form from the old ESP32 receiver firmware.

### Media key mapping (PC app)

| Command | Key sent |
|---------|----------|
| NEXT | Media Next Track |
| PREVIOUS | Media Previous Track |
| PLAY | Media Play/Pause |
| VOLUP | Volume Up |
| VOLDOWN | Volume Down |
| MUTE | Volume Mute |

---

## OLED Screens

| Screen | When | Content |
|--------|------|---------|
| Boot | power-up | title, ESP32-WROOM-DA, "Initializing…", FW version, animated progress bar |
| Home | connected | device name, Connected, signal bars, battery %, last button, time since last command, FW |
| Button animation | every press (1 s) | large icon + label (Next / Previous / Play / Vol+ / Vol– / Mute) |
| Searching | link down, never connected | "Searching… Looking for Receiver" with animated dots |
| Connected splash | first 2 s after link up | "Receiver Connected — Ready" |
| Error | link lost | "Connection Lost — Retrying…" with blinking bar |
| Battery | **long-press Mute** (4 s) | %, voltage, low-battery warning |
| About | **long-press Play/Pause** (4 s) | device, RF module, FW, author |
| Sleep | 60 s idle | "Sleeping… Press Any Button", then OLED off + deep sleep |

---

## Library Installation (Arduino)

**Boards**: install **esp32 by Espressif Systems** and **esp8266 by ESP8266 Community** via Boards Manager (File → Preferences → Additional boards manager URLs):

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Use **ESP32 Arduino core 2.0.17** (stable; also compatible with 3.x) and the **ESP8266 core 3.x**.

**Libraries** (Sketch → Include Library → Manage Libraries…):

| Library | Author | Version |
|---------|--------|---------|
| **RF24** | TMRh20 | ≥ 1.4.0 |
| **Adafruit SSD1306** | Adafruit | latest (remote only) |
| **Adafruit GFX** | Adafruit | latest (remote only) |
| **Adafruit BusIO** | Adafruit | latest (remote only) |

---

## Flashing the Firmware

### Remote (`Remote/` folder — open `remote.ino`)

```
Board:        ESP32 Dev Module
Flash size:   4 MB (or larger)
Flash mode:   QIO
Upload speed: 115200
Partition:    Default 4MB (no OTA)
```

> Opening `remote.ino` loads all tabs (`buttons.ino`, `nrf_comm.ino`, `ui.ino`, `battery.ino`) automatically — they compile together as one sketch.

### Receiver (`Receiver/` folder — open `receiver.ino`)

```
Board:        NodeMCU 1.0 (ESP-12E Module)
Flash size:   4M (1M SPIFFS)
Flash mode:   DIO
Upload speed: 115200
```

> Opening `receiver.ino` loads `nrf_rx.ino` automatically. No additional libraries beyond RF24 (SPI is built into the ESP8266 core).
>
> The USB-serial chip on most NodeMCU boards is a **CH340** — install its driver if Windows shows an unknown device.

### Pairing (do this once, before deployment)

1. Open `Remote/config.h` and `Receiver/config.h`.
2. Make sure `DEVICE_ID` (default `0xA1B2`) and `ENCRYPTION_KEY` (16 bytes) are **identical in both files**.
3. Flash remote, flash receiver, done. Any other device with a different ID/key is ignored by the receiver.

---

## Python Installation

Requires **Python 3.9+** on Windows.

```bat
cd Python
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

**Note on permissions**: the `keyboard` library uses Windows `SendInput`, which works from a normal user session. If media keys ever fail, run the app once as Administrator.

---

## Running the System

1. Plug the **receiver** into the PC (any USB port). A new COM port appears (CH340 on the NodeMCU; CP210x and others also detected).
2. Start the Python app:
   ```bat
   venv\Scripts\activate
   python main.py
   ```
   The tray icon appears and the settings window opens.
3. In the settings window, pick a COM port (or leave **AUTO** — the app auto-detects the receiver) and press **Save & Apply**.
4. The tray tooltip should read `Connected`. The remote shows **Searching…** for a moment, then **Receiver Connected → Home**.
5. Press buttons — media commands hit the active media app.

### Hidden start (no window, no console)

```bat
venv\Scripts\pythonw.exe main.py --hidden
```
or enable **"Start hidden"** / **"Start with Windows"** in the settings window (stored in HKCU registry, no admin needed).

### Force a specific port

```bat
python main.py --port COM5
```

---

## Windows Python App

`Python/` — modular, thread-based:

| Module | Responsibility |
|--------|----------------|
| `main.py` | wiring, message dispatch, settings window (dark mode), single-instance guard |
| `serial_manager.py` | COM detection, auto-reconnect, line protocol, PING/PONG |
| `media_controller.py` | media key mapping + execution |
| `tray.py` | pystray icon, menu, balloon notifications |
| `settings.py` | config.json + "Start with Windows" registry |
| `logger.py` | daily rotating log `logs/daily.log` (7 days) |

The log records timestamps, received packets, signal quality, errors, reconnects and executed commands. Set `"log_level": "DEBUG"` in `config.json` to log every heartbeat.

---

## Folder Structure

```
SpotifyRemote/
├── Remote/                 ESP32 remote firmware (open remote.ino)
│   ├── remote.ino          main loop, link state machine, sleep
│   ├── nrf_comm.h/.cpp     NRF24 TX, packet build, encryption, link quality
│   ├── buttons.h/.cpp      debounce, long press, hold repeat
│   ├── ui.h/.cpp           OLED screens, icons, animations
│   ├── battery.h/.cpp      battery ADC + deep sleep
│   ├── config.h            pins, timing, identity, key
│   └── protocol.h          shared packet/protocol definition
├── Receiver/               ESP8266 receiver firmware (open receiver.ino)
│   ├── receiver.ino        main loop, watchdog, serial out
│   ├── nrf_rx.ino          RX, validation, dedupe, ACK payloads
│   ├── config.h            NodeMCU pins, timing, identity, key
│   └── protocol.h          shared packet/protocol definition
├── Python/                 Windows desktop application
│   ├── main.py             entry point + settings window
│   ├── serial_manager.py   COM auto-detect + reconnect
│   ├── media_controller.py media keys
│   ├── tray.py             system tray + notifications
│   ├── settings.py         config.json + autostart
│   ├── logger.py           daily log rotation
│   ├── config.json         user configuration
│   └── requirements.txt
├── logs/                   daily.log lives here (created automatically)
├── docs/
│   ├── Circuit.png         wiring diagram
│   ├── Flowchart.png       firmware flowcharts
│   ├── Architecture.png    system architecture
│   └── generate_diagrams.py  diagram generator (Pillow)
└── README.md
```

---

## Troubleshooting Guide

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Python: "No receiver found" | driver missing / cable charge-only | Install the CH340 driver (NodeMCU) or CP210x; try another cable; set the port manually in settings |
| Receiver appears as an unknown device in Windows | CH340 driver missing | Download the CH340/CH341 driver from the WCH website and install it |
| Remote stuck on Searching… | channel/address mismatch, too far | flash both `config.h` with same `RF_CHANNEL` + `RECEIVER_ADDRESS`; keep < 30 m; check antenna connections |
| Link dies indoors | PA+LNA needs a clear line of sight | 250 kbps already selected; avoid metal enclosures; raise antennas |
| OLED stays blank | wrong I2C pins/address | check `OLED_SDA_PIN/SCL_PIN` (21/22), address 0x3C |
| Battery shows "Power USB" | no battery or wrong divider | check `BAT_DIVIDER_RATIO`, or connect the LiPo |
| Remote resets when pressing buttons | NRF power spikes | add the AMS1117 rail + 10 µF/100 nF at the NRF (see Power Recommendations) |
| Media keys don't affect Spotify | wrong window focus / permissions | click the Spotify window first; run the app as Administrator once |
| Two receivers answer | both receivers in range | they share the same device ID — the remote pairs with the first ACK; use unique IDs per pair |
| Commands lost during hold | link down | commands are queued and retried every 120 ms until ACKed (see `updateLink`) |
| `keyboard` import error | dependency missing | `pip install keyboard`; on Windows it must not be blocked by policies |

---

## FAQ

**Q: Does the remote need internet?**
A: No. The RF link, USB serial and media keys are all local. Only Spotify itself needs its own connection.

**Q: Which Spotify works?**
A: The Windows **desktop app** and the **web player** in Chrome/Edge/Firefox/Brave/Opera — both respond to media keys once focused. Also YouTube Music, VLC, WMP.

**Q: How far does it reach?**
A: PA+LNA + 250 kbps + channel 108 gives roughly **30–100 m** line of sight; walls reduce it. Lower the data rate or raise antennas for more range.

**Q: How long does the battery last?**
A: Active: ~30–80 mA (OLED + RF). Deep sleep: a few µA. A 400 mAh LiPo lasts days with normal use; the MOSFET mod cuts the NRF completely in sleep.

**Q: How do I change the encryption key?**
A: Edit `ENCRYPTION_KEY` in **both** `config.h` files identically and reflash both. Old remotes are then rejected.

**Q: Why channel 108?**
A: 2.508 GHz sits above the crowded 2.4 GHz Wi-Fi band, avoiding interference from access points.

**Q: Can I use two receivers (living room + bedroom)?**
A: Yes in principle — flash each receiver with its own `DEVICE_ID` and set the remote's `DEVICE_ID` accordingly. See Future Improvements for true multi-receiver support.

---

## Future Improvements

The architecture (modular tabs, enum commands, versioned protocol, PC app with a message bus) is designed for easy extension:

- **Media metadata** — receiver → PC `STATUS` line → PC → OLED via a `CMD:SHOW_TITLE` channel (RF link is bidirectional through ACK payloads).
- **Rotary encoder volume** — plug into the button scanner, emit `VOLUP`/`VOLDOWN` pulses.
- **Touch buttons / haptics / RGB LED** — extend the `ButtonDef` table and add an effect layer.
- **Multiple paired receivers** — device ID routing + link quality selection in the remote.
- **OTA over Wi-Fi** — the ESP32 module already has Wi-Fi; add a bootloader partition and a web server.
- **Web configuration** — host a captive portal on the remote for key/pairing setup.
- **Wi-Fi fallback** — detect a lost NRF link and switch the receiver to an ESP-NOW/TCP bridge.
- **Macro buttons** — new command IDs (e.g. `CMD_LAUNCH_SPOTIFY`) handled by the Python app.
- **Battery charging status** — TP4056 charger with a charge-complete GPIO.

---

## License & Credits

- **License**: MIT — see the repository license file. Use freely, including commercially.
- **Firmware**: built on [RF24](https://github.com/nRF24/RF24) (TMRh20), [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306), [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library).
- **Desktop app**: [pyserial](https://github.com/pyserial/pyserial), [keyboard](https://github.com/boppreh/keyboard), [pystray](https://github.com/moses-palmer/pystray), [Pillow](https://python-pillow.org), [psutil](https://github.com/giampaolo/psutil).
- **Hardware**: ESP32-WROOM-DA (Espressif) remote, ESP8266 NodeMCU (Espressif) receiver, NRF24L01 PA+LNA, SSD1306 OLED — stock modules, no custom boards required.
- Diagrams generated with `docs/generate_diagrams.py` (Pillow).

---

*No cloud. No Spotify API. No browser extension. Just hardware and Windows media keys.*
