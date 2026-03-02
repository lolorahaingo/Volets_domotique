<p align="center">
  <img src="https://img.shields.io/badge/MCU-Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" />
  <img src="https://img.shields.io/badge/Protocol-Morse%20over%20IR-8B0000?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" />
  <img src="https://img.shields.io/badge/Status-Complete-brightgreen?style=for-the-badge" />
</p>

<h1 align="center">Volets Domotique</h1>

<p align="center">
  <i>A DIY motorized shutter controller that uses Morse code over infrared as its communication protocol — making it immune to standard RF replay attacks.</i>
</p>

---

## The Idea

Commercial motorized shutters use simple RF protocols (like Somfy RTS) that can be intercepted and replayed with cheap SDR hardware. During a vacation, I built a fully custom alternative from scratch: a shutter controller that speaks **Morse code over infrared** — a protocol that no off-the-shelf device can clone or replay.

The system also features **automated scheduling** with gradual opening/closing, **position tracking**, and a **dual-mode remote** that doubles as a clock adjustment tool.

---

## Why Morse Code?

This is the core design choice that makes the project unique:

```
 COMMERCIAL SHUTTERS                    THIS PROJECT
 ───────────────────                    ────────────────────────
 RF 433 MHz                             Infrared 38 kHz
 Fixed rolling codes                    Morse-encoded words
 Vulnerable to SDR replay               No standard decoder exists
 Proprietary protocol                   Custom alphabet-based protocol
 Single command per button              Full words as commands
```

Instead of sending a binary command like `0xA3F1` for "up", the remote literally spells out the **word "up"** in Morse code using infrared pulses:

```
 Command "up":

 u = ..-       p = .--.

 IR LED:  ┃ ┃   ┃ ┃   ━━━┃   ┃ ┃   ━━━┃   ━━━┃   ┃ ┃
          .  .    -        .    -     -    .
          ╰── u ──╯        ╰────── p ──────╯

 Timing:
   dot   = 5ms pulse
   dash  = 15ms pulse (3x dot)
   gap between symbols = 5ms
   gap between letters = 15ms
   gap between words   = 35ms
```

**Why is this secure?** An attacker with a standard 433 MHz replay device sees nothing — the signal is infrared. An attacker with an IR receiver sees a 38 kHz modulated signal, but without knowing it's Morse code, the timing pattern looks like noise. And even if they figure out the encoding, they'd need to build a custom Morse transmitter to replay it.

---

## System Architecture

The system is split into two independent Arduino-based devices:

```
                    INFRARED (38 kHz, Morse-encoded)
                    ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
                                    
  ┌─────────────────────┐                    ┌──────────────────────────────┐
  │     REMOTE           │                    │          DECODER              │
  │     (Arduino)        │     "up"           │          (Arduino)            │
  │                      │    "down"          │                              │
  │  [BTN UP]  ─┐       │    "stop"          │   IR Receiver ──► Morse      │
  │  [BTN DOWN]─┤       │    "min"           │   Decoder     ──► Command    │
  │  [BTN STOP]─┤──► IR LED ─ ─ ─ ─ ─ ─ ─ ─►   Parser      ──► Relay     │
  │  [BTN MODE]─┘       │    "hour"          │                    Control   │
  │                      │                    │                              │
  │  Mode: SHUTTER       │                    │   ┌────────┐  ┌──────────┐  │
  │    UP / DOWN / STOP  │                    │   │ CLOCK  │  │ POSITION │  │
  │  Mode: CLOCK         │                    │   │ 24h    │  │ TRACKER  │  │
  │    +1 MIN / +1 HOUR  │                    │   │ on LCD │  │ 0-15000  │  │
  │                      │                    │   └────┬───┘  └────┬─────┘  │
  └─────────────────────┘                    │        │           │        │
                                              │        ▼           ▼        │
                                              │   ┌────────────────────┐    │
                                              │   │   AUTO SCHEDULER   │    │
                                              │   │                    │    │
                                              │   │  08:00 → crack open│    │
                                              │   │  08:20 → half open │    │
                                              │   │  08:35 → full open │    │
                                              │   │  21:00 → half close│    │
                                              │   │  21:30 → mostly    │    │
                                              │   │  22:00 → full close│    │
                                              │   └────────────────────┘    │
                                              │             │               │
                                              │             ▼               │
                                              │   ┌────────────────────┐    │
                                              │   │   3x RELAY MODULE  │    │
                                              │   │                    │    │
                                              │   │  Relay D (descend) │    │
                                              │   │  Relay S (sense)   │    │
                                              │   │  Relay M (mount)   │    │
                                              │   └────────┬───────────┘    │
                                              └────────────┼────────────────┘
                                                           │
                                                           ▼
                                                  ┌────────────────┐
                                                  │  SHUTTER MOTOR │
                                                  │  (230V AC)     │
                                                  └────────────────┘
```

---

## The Remote

A minimal 4-button Arduino device with an IR LED modulated at **38 kHz** (standard IR carrier frequency, compatible with any IR receiver module).

```
 ┌─────────────────────────────────┐
 │          REMOTE                 │
 │                                 │
 │   [MODE] ← toggle SHUTTER/CLOCK│
 │                                 │
 │   SHUTTER mode:    CLOCK mode:  │
 │   [▲] → "up"      [▲] → "min"  │
 │   [▼] → "down"    [▼] → "hour" │
 │   [■] → "stop"    [■] → (none) │
 │                                 │
 │          (( IR LED ))  ─────────┤──► 38 kHz Morse
 └─────────────────────────────────┘
```

### How a word is transmitted

```
 envoie("down")
    │
    ├── 'd' → signalDe('d') → "-.. "
    │         dash() dot() dot() finLettre()
    │
    ├── 'o' → signalDe('o') → "---"
    │         dash() dash() dash() finLettre()
    │
    ├── 'w' → signalDe('w') → ".--"
    │         dot() dash() dash() finLettre()
    │
    └── 'n' → signalDe('n') → "-."
              dash() dot() finLettre()
```

Each `dot()` and `dash()` modulates the IR LED at 38 kHz using `tone()`, making the signal invisible to the naked eye but readable by any standard IR receiver.

---

## The Decoder

The decoder is the brain of the system. It runs three concurrent state machines:

### 1. Morse Decoder (`Decode` class)

Real-time Morse decoding from the IR receiver using timing analysis:

```
 IR Receiver (A0)
      │
      ▼
 Signal LOW? ──► Measure pulse duration
      │
      ├── ≤ 5ms  → DOT  (.)
      ├── > 5ms  → DASH (-)
      │
      ▼
 Silence ≥ 15ms? → Letter complete → lookup in dictionary
 Silence ≥ 35ms? → Word complete → send to command parser
```

The decoder builds words **letter by letter** and matches them against commands using substring matching (`correspond()`), allowing the system to react as soon as a valid command word is recognized.

### 2. Position Tracker (`Volet` class)

The system tracks shutter position in real-time without any physical sensor:

```
 Position scale: 0 (fully closed) ──────── 15000 (fully open)

 Method: Time-based integration
   - While motor runs UP:    position += elapsed_ms
   - While motor runs DOWN:  position -= elapsed_ms
   - On STOP: freeze position

 This enables the auto-scheduler to move to ANY intermediate position,
 not just "fully open" or "fully closed".
```

### 3. Automated Schedule (`modeAuto`)

Gradual opening in the morning, gradual closing in the evening:

```
 ┌──────────────────────────────────────────────────────────────┐
 │                                                              │
 │  06:00          08:00   08:20   08:35                        │
 │    ║              ║       ║       ║                          │
 │    ║   CLOSED     ║ crack ║ half  ║  FULLY OPEN             │
 │    ║══════════════╬═══════╬═══════╬══════════════            │
 │                   ▲       ▲       ▲                          │
 │                 1100    7500   15000                          │
 │                                                              │
 │                              21:00   21:30   22:00           │
 │                                ║       ║       ║            │
 │                     OPEN       ║ half  ║ most  ║  CLOSED    │
 │                   ═════════════╬═══════╬═══════╬═══         │
 │                                ▲       ▲       ▲            │
 │                              7500    4000      0            │
 └──────────────────────────────────────────────────────────────┘

 Morning: gradual opening over 35 minutes (gentle wake-up)
 Evening: gradual closing over 1 hour (natural transition)
```

The schedule can be **overridden at any time** by sending a manual command — the auto mode disengages until the next scheduled event.

### 4. Internal Clock (`Clock` class)

A software clock displayed on a 16x2 LCD, adjustable via the remote's CLOCK mode (+1 min / +1 hour). The clock is initialized at 23:00 (`pendule = 82800` seconds) and counts up using `millis()`.

---

## Relay Logic

The shutter motor requires **polarity reversal** to change direction. Three relays handle this:

```
 Command    Relay D    Relay S    Relay M    Motor
 ─────────────────────────────────────────────────
 DOWN       LOW        LOW        HIGH       ↓ Descend
 UP         HIGH       HIGH       LOW        ↑ Mount
 STOP       HIGH       HIGH       HIGH       ⏹ Off
 ─────────────────────────────────────────────────

 Safety: STOP is the default state (all relays HIGH = normally open)
         Position limits (0 and 15000) auto-trigger STOP
```

---

## Commands Reference

| Command | Morse | Mode | Action |
|---------|-------|------|--------|
| `up` | `..- .--.` | Shutter | Raise the shutter |
| `down` | `-.. --- .-- -.` | Shutter | Lower the shutter |
| `stop` | `... - --- .--.` | Shutter | Stop the motor |
| `min` | `-- .. -.` | Clock | Add 1 minute to the clock |
| `hour` | `.... --- ..- .-.` | Clock | Add 1 hour to the clock |

---

## Hardware

| Component | Role | Qty |
|-----------|------|-----|
| Arduino (x2) | Remote controller + Decoder/relay controller | 2 |
| IR LED (38 kHz) | Morse transmitter on remote | 1 |
| IR Receiver module (e.g. TSOP4838) | Morse receiver on decoder | 1 |
| 3-channel relay module | Motor direction control (230V AC switching) | 1 |
| 16x2 LCD display | Clock display | 1 |
| Push buttons | Remote controls (UP, DOWN, STOP, MODE) | 4 |
| Resistors | Pull-ups for buttons | 4 |

---

## Project Structure

```
Volets_domotique/
├── telecommandePrototype/
│   └── telecommandePrototype.ino    # Remote: buttons → Morse → IR LED
└── decodeurPrototype/
    └── decodeurPrototype.ino        # Decoder: IR → Morse → relays + clock + auto
```

---

## What Makes This Interesting

| Aspect | Detail |
|--------|--------|
| **Security by obscurity (but real)** | No commercial device speaks Morse over IR. A burglar with an RF jammer or rolling code grabber is useless here. |
| **Zero dependencies** | No WiFi, no cloud, no app, no Bluetooth. Works during power outages (Arduino on battery), internet outages, and RF jamming. |
| **Position tracking without sensors** | Time-based integration gives continuous position awareness — enabling intermediate positions, not just open/close. |
| **Gradual automation** | The morning schedule opens shutters over 35 minutes, simulating a natural wake-up. Not just "bang, fully open at 8am". |
| **Dual-mode remote** | Same 3 buttons serve two functions (shutter control + clock adjustment) via a mode toggle — minimal hardware, maximum utility. |
| **Real-time Morse decoding** | The decoder processes IR pulses in real-time with timing-based dot/dash discrimination — a non-trivial embedded signal processing problem. |

---

<p align="center">
  <i>A vacation project that turned a security concern into an engineering challenge — and a custom protocol that no off-the-shelf device can crack.</i>
</p>
