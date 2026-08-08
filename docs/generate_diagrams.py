#!/usr/bin/env python3
"""
generate_diagrams.py - Renders the project documentation diagrams:

    docs/Architecture.png   system architecture overview
    docs/Flowchart.png      firmware behaviour flowcharts (remote + receiver)
    docs/Circuit.png        functional wiring / circuit diagram

Run:  python generate_diagrams.py
Output goes next to this script (docs/). Only requires Pillow.
"""

import math
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- palette
BG      = (13, 17, 23)
PANEL   = (22, 27, 34)
PANEL2  = (17, 22, 29)
BORDER  = (48, 54, 61)
TEXT    = (201, 209, 217)
MUTED   = (139, 148, 158)
GREEN   = (29, 185, 84)
BLUE    = (88, 166, 255)
AMBER   = (210, 153, 34)
RED     = (248, 81, 73)
WIRE    = (139, 148, 158)
POWER   = (46, 160, 67)

FONTS = {}


def font(size, bold=False):
    key = (size, bold)
    if key not in FONTS:
        for name in (("arialbd.ttf" if bold else "arial.ttf"),
                     ("segoeuib.ttf" if bold else "segoeui.ttf"),
                     "consola.ttf"):
            path = os.path.join(r"C:\Windows\Fonts", name)
            if os.path.exists(path):
                try:
                    FONTS[key] = ImageFont.truetype(path, size)
                    break
                except Exception:
                    continue
        else:
            FONTS[key] = ImageFont.load_default()
    return FONTS[key]


def wrap(text, width_chars):
    """Word-wrap text into lines of at most width_chars characters."""
    words, lines, cur = text.split(), [], ""
    for w in words:
        if cur and len(cur) + 1 + len(w) > width_chars:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + " " + w).strip()
    if cur:
        lines.append(cur)
    return lines


def centered(draw, cx, y, text, fg=TEXT, size=15, bold=False):
    f = font(size, bold)
    for ln in text.split("\n"):
        draw.text((cx - draw.textlength(ln, font=f) / 2, y), ln, font=f, fill=fg)
        y += size + 4


def box(draw, x, y, w, h, title=None, lines=None, fill=PANEL,
        border=BORDER, radius=10, title_color=GREEN, tsize=16):
    draw.rounded_rectangle([x, y, x + w, y + h], radius=radius,
                           fill=fill, outline=border, width=2)
    yy = y + 9
    if title:
        centered(draw, x + w / 2, yy, title, title_color, tsize, True)
        yy += tsize + 7
    for line in (lines or []):
        centered(draw, x + w / 2, yy, line, TEXT, 13)
        yy += 18
    return (x, y, x + w, y + h)


def _head(draw, x, y, ang, color):
    for a in (ang + 2.62, ang - 2.62):
        draw.line([x, y, x + 12 * math.cos(a), y + 12 * math.sin(a)],
                  fill=color, width=2)


def arrow(draw, p1, p2, label=None, color=TEXT, lx=None, ly=None):
    x1, y1 = p1
    x2, y2 = p2
    draw.line([x1, y1, x2, y2], fill=color, width=2)
    _head(draw, x2, y2, math.atan2(y2 - y1, x2 - x1), color)
    if label:
        f = font(12, True)
        draw.text((lx if lx is not None else (x1 + x2) / 2 + 6,
                   ly if ly is not None else (y1 + y2) / 2 - 20),
                  label, font=f, fill=GREEN)


def polyarrow(draw, pts, label=None, color=TEXT, lx=None, ly=None):
    """Polyline ending in an arrowhead; pts = [(x, y), ...]."""
    draw.line(pts, fill=color, width=2)
    p1, p2 = pts[-2], pts[-1]
    _head(draw, p2[0], p2[1], math.atan2(p2[1] - p1[1], p2[0] - p1[0]), color)
    if label:
        f = font(12, True)
        mid = pts[len(pts) // 2]
        draw.text((lx if lx is not None else mid[0] + 8,
                   ly if ly is not None else mid[1] - 8),
                  label, font=f, fill=GREEN)


def diamond(draw, cx, cy, w, h, text):
    pts = [(cx, cy - h), (cx + w, cy), (cx, cy + h), (cx - w, cy)]
    draw.polygon(pts, fill=PANEL2, outline=BORDER)
    lines = text.split("\n")
    yy = cy - (len(lines) * 14) / 2 + 2
    for ln in lines:
        centered(draw, cx, yy, ln, TEXT, 12)
        yy += 14


def new_canvas(w, h, title=None):
    img = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(img)
    if title:
        centered(d, w / 2, 18, title, TEXT, 24, True)
    return img, d


# =====================================================================
# 1. Architecture diagram
# =====================================================================

def render_architecture():
    W, H = 1180, 470
    img, d = new_canvas(W, H, "System Architecture - Wireless Spotify Remote")
    bw, bh = 172, 104
    y = 220
    boxes = [
        ("ESP32 Remote", "ESP32-WROOM-DA\n6 buttons · OLED\nbattery · sleep"),
        ("NRF24L01 PA+LNA", "TX side\n2.508 GHz\n250 kbps · auto-ACK"),
        ("NRF24L01 PA+LNA", "RX side\nPA + LNA\nSMA antenna"),
        ("ESP8266 Receiver", "NodeMCU ESP-12E\nUSB serial\n115200 baud"),
        ("Python App", "tray icon · logging\nauto reconnect\nmedia keys"),
        ("Windows Media Keys", "Spotify · YouTube\nVLC · WMP\nbrowser players"),
    ]
    xs = [26 + i * (bw + 16) for i in range(6)]
    for (t, lines), x in zip(boxes, xs):
        box(d, x, y, bw, bh, t, wrap(lines, 20), tsize=15)

    labels = ["SPI", "RF LINK", "RF LINK", "USB SERIAL", "KEYBOARD EVENTS"]
    colors = [WIRE, GREEN, GREEN, BLUE, AMBER]
    for i in range(5):
        x1 = xs[i] + bw
        x2 = xs[i + 1]
        arrow(d, (x1, y + bh / 2), (x2, y + bh / 2),
              labels[i], color=colors[i], ly=y + bh / 2 - 28)

    caps = ["Remote device\n(battery powered)",
            "Wireless link\nencrypted · CRC-16 · retries",
            "NodeMCU receiver\ndongle (USB powered)",
            "PC software\n(Windows 10/11)",
            "Any media app\nno integration needed"]
    for x, c in zip(xs, caps):
        for j, ln in enumerate(wrap(c, 22)):
            centered(d, x + bw / 2, y + bh + 22 + j * 16, ln, MUTED, 12)

    centered(d, W / 2, H - 34,
             "No cloud  |  No Spotify API  |  No browser extension  |  No internet dependency",
             MUTED, 13, True)
    img.save(os.path.join(HERE, "Architecture.png"))


# =====================================================================
# 2. Flowchart diagram (remote + receiver columns)
# =====================================================================

def render_flowchart():
    W, H = 1160, 1100
    img, d = new_canvas(W, H, "Firmware Behaviour - Remote (left) / Receiver (right)")
    L = 290        # remote column center
    R = 870        # receiver column center
    BW = 420       # remote box width

    def cbox(cx, y, w, h, title=None, lines=None, **kw):
        box(d, cx - w / 2, y, w, h, title, lines, **kw)
        return y + h

    # ------------------------------- remote ------------------------------
    centered(d, L, 58, "REMOTE  (ESP32-WROOM-DA)", BLUE, 15, True)
    y = cbox(L, 84, 300, 40, None, ["Boot / wake (EXT1)"])
    y = cbox(L, y + 24, BW, 44, None, ["Init: NRF24 · OLED · buttons · battery ADC"])
    y = cbox(L, y + 24, BW, 44, None, ["Searching: send CMD_CONNECT every 500 ms"])
    ack_cy = y + 58
    diamond(d, L, ack_cy, 150, 45, "ACK\nreceived?")
    y = cbox(L, ack_cy + 68, BW, 48, "CONNECTED", ["Heartbeat every 5 s · link quality",
                                                   "estimate · read ACK payload"])
    loop_top = y
    y = cbox(L, y + 20, BW, 48, None, ["Scan buttons (debounce · long press · repeat)",
                                       "OLED 12 fps · battery every 10 s"])
    btn_cy = y + 30
    diamond(d, L, btn_cy, 120, 44, "Button\npressed?")
    y = cbox(L, btn_cy + 92, BW, 48, None, ["Connection Lost · Error screen\nretry CMD_CONNECT"])
    idle_cy = y + 55
    diamond(d, L, idle_cy, 150, 45, "Idle > 60 s?")
    y = cbox(L, idle_cy + 68, BW, 48, "SLEEP", ["OLED off · NRF power down",
                                                "deep sleep (EXT1 wake)"])

    # main vertical chain (x = L)
    arrow(d, (L, 124), (L, 148))
    arrow(d, (L, 192), (L, 216))
    arrow(d, (L, 260), (L, ack_cy - 45))
    arrow(d, (L, ack_cy + 45), (L, 390), "yes", GREEN, ly=400)
    arrow(d, (L, 438), (L, 462))
    arrow(d, (L, 510), (L, btn_cy - 44))
    arrow(d, (L, btn_cy + 44), (L, btn_cy + 92), "no", RED, ly=btn_cy + 60)
    arrow(d, (L, btn_cy + 140), (L, idle_cy - 45))
    arrow(d, (L, idle_cy + 45), (L, 795), "yes", GREEN, ly=idle_cy + 56)

    # command box (yes branch, right of button diamond)
    cmd_box = box(d, 460, btn_cy - 18, 190, 48, None,
                  ["Send command packet",
                   "button animation (1 s)"])
    polyarrow(d, [(410, btn_cy), (440, btn_cy), (440, btn_cy - 18 + 24), (460, btn_cy - 18 + 24)],
              "yes", GREEN, ly=btn_cy - 30)
    # command -> loop back (right side)
    polyarrow(d, [(650, btn_cy + 6), (560, btn_cy + 6), (560, 486), (470, 494)], color=GREEN)

    # error -> searching loop (left side)
    polyarrow(d, [(80, btn_cy + 116), (40, btn_cy + 116), (40, 238), (80, 238)], color=AMBER)
    # idle "no" -> loop box (left side)
    arrow(d, (L - 150, idle_cy), (70, idle_cy), "no", RED)
    polyarrow(d, [(70, idle_cy), (70, 486), (80, 486)], color=RED)
    # sleep wake -> loop box (right side)
    polyarrow(d, [(500, 843), (600, 843), (600, 478), (470, 478)], color=GREEN)

    centered(d, L, 1072, "millis() timers only · no delay() · 1 KB OLED framebuffer",
             MUTED, 12)

    # ----------------------------- receiver ------------------------------
    centered(d, R, 58, "RECEIVER  (ESP8266 NodeMCU)", BLUE, 15, True)
    RW = 380
    y = cbox(R, 84, 300, 40, None, ["Boot"])
    y = cbox(R, y + 24, RW, 44, None, ["Init: NRF24 RX · Serial 115200"])
    wait_y = cbox(R, y + 24, RW, 44, None, ["Wait for packet\nradio.available()"])
    y = cbox(R, wait_y + 20, RW, 48, None, ["Validate: magic · deviceId · checksum",
                                            "CRC-16 · counter · duplicate window"])
    val_cy = y + 60
    diamond(d, R, val_cy, 150, 45, "Valid &\nunique?")
    # "no" -> drop box (left)
    drop_box = box(d, 640, val_cy + 44, 200, 44, None, ["Drop packet (silently)"])
    # "yes" -> dispatch
    y = cbox(R, val_cy + 68, RW, 44, None, ["Dispatch by command"])
    y = cbox(R, y + 20, RW, 48, None, ["Media cmd -> bare 'NEXT'...",
                                       "Heartbeat -> 'LINK:UP' + ACK payload"])
    y = cbox(R, y + 28, RW, 56, "WATCHDOG", ["No packet for 15 s -> 'LINK:DOWN'",
                                             "link re-acquired automatically"])

    arrow(d, (R, 124), (R, 148))
    arrow(d, (R, 192), (R, 216))
    arrow(d, (R, wait_y), (R, wait_y + 20))
    arrow(d, (R, wait_y + 68), (R, val_cy - 45))
    arrow(d, (R, val_cy + 45), (R, val_cy + 68), "yes", GREEN)
    arrow(d, (R, val_cy + 112), (R, val_cy + 132))
    arrow(d, (720, val_cy), (720, drop_box[1]), "no", RED)
    # drop -> wait loop (left)
    polyarrow(d, [(640, val_cy + 66), (600, val_cy + 66), (600, 248), (680, 248)], color=RED)
    # action -> wait loop (right)
    polyarrow(d, [(1060, 544), (1090, 544), (1090, 238), (1060, 238)], color=GREEN)
    # watchdog -> wait loop (left, outer)
    polyarrow(d, [(680, val_cy + 208), (570, val_cy + 208), (570, 228), (680, 228)], color=AMBER)

    centered(d, R, 1072, "polling loop · interrupt-free · validation pipeline drops junk",
             MUTED, 12)

    img.save(os.path.join(HERE, "Flowchart.png"))


# =====================================================================
# 3. Circuit / wiring diagram
# =====================================================================

def pin_lines(d, x, y, items, size=12, fg=TEXT):
    f = font(size)
    for text in items:
        d.text((x, y), text, font=f, fill=fg)
        y += 17


def wire(d, pts, label=None, color=WIRE, lx=None, ly=None):
    d.line(pts, fill=color, width=2)
    if label:
        f = font(12, True)
        d.text((lx if lx is not None else (pts[0][0] + pts[-1][0]) / 2 + 8,
                ly if ly is not None else (pts[0][1] + pts[-1][1]) / 2 - 8),
               label, font=f, fill=color)


def render_circuit():
    W, H = 1620, 1020
    img, d = new_canvas(W, H, "Wiring Diagram - Wireless Spotify Remote (functional view)")
    d.line([60, 130, 1560, 130], fill=POWER, width=3)     # 5V rail
    d.line([60, 950, 1560, 950], fill=MUTED, width=3)     # GND rail
    centered(d, 1565, 118, "5V", POWER, 13, True)
    centered(d, 1565, 944, "GND", MUTED, 13, True)

    # ----- NodeMCU receiver (dongle) -------------------------------------
    box(d, 60, 170, 300, 230, "ESP8266 NodeMCU RECEIVER",
        ["dongle at the PC, USB (CH340)",
         "D2 -> NRF CE      D8 -> NRF CSN",
         "D5 -> NRF SCK     D7 -> NRF MOSI",
         "D6 <- NRF MISO    IRQ -> n/c",
         "3V3 -> NRF VCC* (10-100 uF cap)",
         "GND -> GND rail"],
        tsize=15)
    wire(d, [(210, 170), (210, 130)], "USB 5V", POWER)
    wire(d, [(180, 400), (180, 450), (390, 450), (390, 950)], None, MUTED)

    # ----- ESP32 remote --------------------------------------------------
    esp = box(d, 430, 300, 380, 500, "ESP32 REMOTE (WROOM-DA)",
              lines=[], tsize=15)
    pin_lines(d, esp[0] + 24, esp[1] + 40, [
        "VIN/5V   <-  5V rail       3V3  -> OLED VCC",
        "GND      ->  GND rail      3V3  -> NRF VCC*",
        "GPIO21   <-> OLED SDA      GND  -> GND rail",
        "GPIO22   <-> OLED SCL",
        "GPIO18   ->  NRF SCK",
        "GPIO19   <-  NRF MISO",
        "GPIO23   ->  NRF MOSI",
        "GPIO4    ->  NRF CE",
        "GPIO5    ->  NRF CSN",
        "GPIO36   <-  battery divider",
        "GPIO32   <-  BTN NEXT",
        "GPIO33   <-  BTN PREV",
        "GPIO25   <-  BTN PLAY",
        "GPIO26   <-  BTN VOL+",
        "GPIO27   <-  BTN VOL-",
        "GPIO14   <-  BTN MUTE",
    ], 13)

    wire(d, [(430, 320), (390, 320), (390, 130)], "5V", POWER)

    # ----- NRF module --------------------------------------------------
    nrf = box(d, 1010, 170, 480, 300, "NRF24L01 PA+LNA",
              lines=[], tsize=15)
    pin_lines(d, nrf[0] + 24, nrf[1] + 40, [
        "VCC   <-  AMS1117 OUT (3.3 V)",
        "GND   ->  GND rail",
        "CE    <-  GPIO4",
        "CSN   <-  GPIO5",
        "SCK   <-  GPIO18",
        "MOSI  <-  GPIO23",
        "MISO  ->  GPIO19",
        "IRQ   ->  (optional, unused)",
        "10 uF + 100 nF capacitors at VCC",
    ], 13)
    # antenna icon
    d.ellipse([1450, 200, 1478, 228], outline=GREEN, width=2)
    d.line([1458, 214, 1446, 232], fill=GREEN, width=2)
    d.line([1446, 232, 1464, 232], fill=GREEN, width=2)
    centered(d, 1464, 236, "SMA", MUTED, 11)

    # SPI + control bundle from ESP32 to NRF
    wire(d, [(810, 420), (860, 420), (860, 300), (1010, 300)],
         "SCK · MISO · MOSI · CE · CSN", lx=872, ly=348)

    # ----- AMS1117 regulator -------------------------------------------
    box(d, 1010, 520, 480, 150, "AMS1117-3.3 (dedicated)", [
        "IN  <-  5V rail",
        "OUT ->  NRF VCC  (250 mA peak)",
        "GND ->  GND rail",
        "caps: 10 uF electrolytic + 100 nF ceramic",
    ], tsize=15)
    wire(d, [(1130, 520), (1130, 130)], "5V", POWER)
    wire(d, [(1290, 670), (1290, 950)], None, MUTED)
    wire(d, [(1290, 470), (1290, 430)], "3.3V", POWER)

    # ----- OLED ---------------------------------------------------------
    oled = box(d, 60, 360, 300, 260, '0.96" OLED SSD1306', [
        "VCC  <-  3V3",
        "GND  ->  GND rail",
        "SDA  <-> GPIO21",
        "SCL  <-> GPIO22",
        "(I2C, address 0x3C)",
    ], tsize=15)
    wire(d, [(360, 460), (400, 460), (400, 380), (430, 380)],
         "SDA(21) · SCL(22)", lx=370, ly=408)
    wire(d, [(180, 620), (180, 950)], None, MUTED)
    wire(d, [(120, 360), (120, 130)], "3V3", POWER)

    # ----- buttons ------------------------------------------------------
    btn = box(d, 60, 700, 300, 210, "6 x push buttons", [
        "active low (LOW = pressed)",
        "internal INPUT_PULLUP",
        "GPIO32 NEXT    GPIO33 PREV",
        "GPIO25 PLAY    GPIO26 VOL+",
        "GPIO27 VOL-    GPIO14 MUTE",
        "each button: pin -> GND",
    ], tsize=15)
    wire(d, [(360, 760), (400, 760), (400, 740), (430, 740)],
         "GPIO32/33/25/26/27/14", lx=372, ly=704)
    wire(d, [(210, 910), (210, 950)], None, MUTED)

    # ----- battery ------------------------------------------------------
    box(d, 1010, 700, 480, 210, "LiPo 1S 3.7 V (optional)", [
        "+  --R1 100k--  -- GPIO36 (ADC)",
        "|                  |",
        "-----------------R2 100k-- GND",
        "GND  ->  GND rail",
        "divider ratio 2:1 -> BAT_DIVIDER_RATIO=2.0",
    ], tsize=15)
    wire(d, [(1250, 700), (1250, 660), (880, 660), (880, 780), (810, 780)],
         "BATTERY MV -> GPIO36", lx=900, ly=640)
    wire(d, [(1100, 910), (1100, 950)], None, MUTED)

    centered(d, W / 2, 980,
             "* Remote NRF: dedicated AMS1117-3.3 (PA_MAX)  |  Receiver NRF: NodeMCU 3V3 + 10-100 uF cap (PA_HIGH)",
             AMBER, 13, True)
    img.save(os.path.join(HERE, "Circuit.png"))


if __name__ == "__main__":
    render_architecture()
    render_flowchart()
    render_circuit()
    print("Diagrams written to", HERE)
