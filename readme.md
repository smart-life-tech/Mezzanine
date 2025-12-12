# 🚧 Forklift Ultrasonic Warning System

**Raspberry Pi 5 + ESP32-PoE + SR04 Sensors**
**Real-time forklift fork-height detection with audio alert**

---

## 📘 Overview

This system prevents forklifts from striking a mezzanine or overhead structure by continuously monitoring fork height using one or two HC-SR04 ultrasonic sensors.
The sensors connect to an ESP32-PoE (Olimex Ethernet module) located at the mezzanine, which sends real-time distance data to a Raspberry Pi 5 over the network. When the fork height crosses a defined threshold, the Pi triggers a loud warning through a USB audio interface, 12V amplifier, and horn speaker.

The entire system is housed in a structured, well-laid-out control enclosure at the workbench, while the sensing hardware sits remotely at the mezzanine end via PoE.

### 🔋 **Power Architecture**

Both the **Raspberry Pi 5** and the **ESP32-PoE module** are powered entirely from **PoE infrastructure**, with a **PoE switch** providing centralized power distribution:

- **Network Topology:** Pi 5 ↔ PoE Switch ↔ ESP32-PoE (NO ROUTER)

- **Pi Stack (Workbench):** The Pi is configured as a vertical stack:
  - **Bottom:** Power supply (PoE HAT or dedicated PSU)
  - **Middle:** Raspberry Pi 5 board
  - **Top:** Cooling fan (12V)
  - The stack connects to PoE switch via single Cat6 cable

- **Mezzanine (ESP Side):** **Olimex ESP32-PoE** board with integrated Ethernet and PoE receives power directly from PoE switch via Cat6 cable. A **3.3V regulator** on the ESP32-PoE board provides **3.3V for both HC-SR04 ultrasonic sensors**.

- **Network Configuration:** Static IPs (NO DHCP, NO ROUTER):
  - Pi: 192.168.10.1/24
  - ESP32 #1: 192.168.10.20
  - ESP32 #2: 192.168.10.21 (future)
  - Communication: UDP port 5005

- **Common Ground:** All components (Pi, ESP, sensors) share **common ground via PoE network**, ensuring signal integrity.

- **Audio Output:** Configurable in config.json:
  - USB audio adapter → Amplifier → Horn speaker
  - GPIO 12/13 PWM audio (Pi 5 native)
  - Onboard 3.5mm headphone jack

---

## 📦 System Components

### **Raspberry Pi Stack (Workbench Side)**

* **Pi Stack Components:**
  - Raspberry Pi 5 board
  - PoE HAT (power from switch)
  - 12V cooling fan (top mount)
  - Stack mounting hardware
* **Network:** Cat6 to PoE switch (static IP: 192.168.10.1)
* **Audio Output:** (configurable in config.json)
  - Option 1: USB → 3.5mm audio adapter → Amplifier/Speaker
  - Option 2: GPIO 12/13 PWM audio (Pi 5 native)
  - Option 3: Onboard 3.5mm headphone jack
* **Controls:**
  - GPIO17: Pause button (momentary, 2 min pause)
  - J2 header: Soft-power button (optional)

### **ESP32-PoE Side (Mezzanine – Sensor End)**

* **Olimex ESP32-PoE** (Ethernet-only, no WiFi)
  - Integrated LAN8720 PHY
  - Static IP: 192.168.10.20
  - Powered via PoE from switch
* **Sensors:** 1-2× HC-SR04 ultrasonic sensors
  - Powered from ESP32's 3.3V rail (NO voltage dividers needed)
  - GPIO: 14, 15, 16, 32
* **Network:** Cat6 from PoE switch
* **Mounting:** Steel L-brackets for rigid sensor positioning

---

## 🧩 System Architecture

### **Complete Data and Power Flow**

```
MEZZANINE END                    PoE SWITCH              WORKBENCH END
(Sensor & Detection)             (NO ROUTER)            (Control & Alerting)

    SR04 Sensors                                         Pi Stack
         │                                               ┌─────────┐
         ├─ TRIG (GPIO14/16) ──┐                        │   Fan   │ (Top)
         ├─ ECHO (GPIO15/32) ──┤                        ├─────────┤
         └─ 3.3V/GND ──┐        │                       │   Pi 5  │ (Middle)
                       │        │                       ├─────────┤
                  ESP32-PoE ←───┼──────────────────────→│ PoE PSU │ (Bottom)
                  (Olimex)      │     Cat6 PoE          └─────────┘
                192.168.10.20   │                      192.168.10.1
                       │        │                           │
                       └────────┴───── PoE Switch ─────────┘
                                      (48V Power)
                                   
    Power: 3.3V from ESP32          Network: UDP port 5005     Audio: USB/GPIO/Jack
    Data: UDP packets              Static IPs (no DHCP)         Control: GPIO17
                               └──── CAT6 ────────────┘
                               (Data + Power)

    GPS: GPIO14, GPIO16 (TRIG), GPIO15, GPIO32 (ECHO)
    Network: UDP Port 5005 (distance packets)
    Audio: Pi USB → 3.5mm → Amp → Horn (Threshold triggered)
    Control: GPIO17 (pause button), J2 (power button)
```

### **Operational Sequence**

1. **Sensor Reading** (100ms cycle):
   - ESP32 continuously polls SR04 #1 and SR04 #2
   - Measures echo pulse duration, converts to distance (cm)
   - Formats packet: `D1:xxx.x,D2:yyy.y\n`

2. **Network Transmission**:
   - ESP sends UDP packet to Pi's port 5005
   - 10 readings per second for real-time response
   - Pi receives and parses distance values

3. **Threshold Detection**:
   - Pi compares minimum distance to configured threshold (default 80cm)
   - If distance < threshold → trigger alert

4. **Audio Alert**:
   - Pi plays horn WAV file through USB audio adapter
   - Sound passes through 12V amplifier to horn speaker
   - Alert repeats every 1 second (configurable min interval)

5. **Pause Control**:
   - User presses big button → pause button callback fires
   - Pi disables alerts for 120 seconds (configurable)
   - LED or log shows pause status
   - After timeout, normal monitoring resumes

6. **Power Management**:
   - UPS detects power loss, maintains Pi operation
   - RTC keeps time during outage
   - Pi can cleanly shutdown via J2 soft power button
   - System auto-recovers on power restore

---

## 📁 Project Structure

```
Mezzanine/
│
├── esp32/                                    # ESP32 Firmware Project (PlatformIO)
│   ├── platformio.ini                        # Board config: esp32-poe, Arduino framework
│   ├── src/
│   │   └── main.cpp                          # Sensor reading + UDP transmission
│   ├── include/
│   │   └── README
│   ├── lib/
│   │   └── README
│   ├── test/
│   │   └── README
│   └── .gitignore
│
├── raspberrypi/                              # Raspberry Pi 5 Application
│   ├── app.py                                # Main alert system (UDP listener, threshold, audio)
│   ├── config.json                           # Configuration (threshold, GPIO, audio path)
│   ├── alert.wav                             # Horn warning sound file (place your WAV here)
│   ├── service/
│   │   └── forklift.service                  # Systemd unit for autostart on boot
│   └── README.md                             # Detailed Pi setup instructions
│
├── layout.md                                 # Physical component placement (enclosure & mezzanine)
├── schematics.md                             # Wiring guide (GPIO, power, audio, buttons)
├── structure.md                              # Directory organization reference
└── README.md                                 # This file (project overview)

---

## ⚙️ Installation & Deployment

### **1. ESP32-PoE Firmware Setup**

#### Prerequisites
- PlatformIO IDE or CLI installed
- USB cable for flashing the ESP32-PoE
- Visual Studio Code with PlatformIO extension (recommended)

#### Steps

1. **Clone/Navigate to `esp32/` folder**:
   ```bash
   cd esp32
   ```

2. **Update WiFi Credentials** in `src/main.cpp`:
   ```cpp
   const char* ssid = "YOUR_SSID";          // Your WiFi network
   const char* password = "YOUR_PASSWORD";  // WiFi password
   const char* udp_target_ip = "192.168.1.100";  // Your Pi's IP
   ```

3. **Verify Board & Framework** in `platformio.ini`:
   - Board: `esp32-poe`
   - Platform: `espressif32`
   - Framework: `arduino`

4. **Build and Flash**:
   ```bash
   pio run -t upload
   ```

5. **Monitor Serial Output** (verify operation):
   ```bash
   pio device monitor --baud 115200
   ```

6. **Voltage Divider Setup** ⚠️:
   - SR04 ECHO outputs 5V, ESP GPIO accepts 3.3V max
   - **Wiring (per schematics.md)**:
     - 3.3V regulator: PoE splitter 5V → 3.3V (for ESP and sensors)
     - SR04 sensors: Connected to 3.3V supply (direct logic levels)
   - This divider scales 5V → 3.3V safely

7. **Mount at Mezzanine**:
   - Secure ESP32 and PoE splitter to wall bracket
   - Mount SR04 sensors on rigid L-brackets for accuracy
   - Connect Cat6 cable from PoE injector to splitter
   - Verify 5V power LED on splitter

---

### **2. Raspberry Pi 5 Setup**

#### Prerequisites
- Fresh Raspberry Pi OS (Bookworm) installation
- SSH access or HDMI terminal
- PoE HAT installed and powered
- USB audio adapter plugged in

#### Dependencies Installation

```bash
sudo apt update
sudo apt install -y python3 python3-pip alsa-utils
pip3 install gpiozero
```

#### File Organization

Create the forklift directory and copy application files:

```bash
sudo mkdir -p /home/pi/forklift
sudo chown pi:pi /home/pi/forklift
cd /home/pi/forklift
```

Copy these files from the `raspberrypi/` folder:
- `app.py` → main Python application
- `config.json` → configuration file
- `alert.wav` → horn warning audio (place your WAV file here)

```bash
# From your development machine:
scp app.py pi@<pi-ip>:/home/pi/forklift/
scp config.json pi@<pi-ip>:/home/pi/forklift/
scp alert.wav pi@<pi-ip>:/home/pi/forklift/
```

#### Test Manually

```bash
cd /home/pi/forklift
python3 app.py
```

You should see:
```
============================================================
Forklift Ultrasonic Warning System - Raspberry Pi
============================================================

[System] Configuration:
  UDP Port: 5005
  Threshold: 80 cm
  Pause Duration: 120 sec
  Alert File: /home/pi/forklift/alert.wav
  Pause GPIO: 17

[System] Starting UDP listener...
[UDP] Listening on port 5005
[System] System ready. Monitoring for forks...
```

#### Configure Autostart with Systemd

Copy the service file:

```bash
sudo cp raspberrypi/service/forklift.service /etc/systemd/system/forklift.service
sudo systemctl daemon-reload
sudo systemctl enable forklift
sudo systemctl start forklift
```

Check status:

```bash
sudo systemctl status forklift
journalctl -u forklift -f  # Follow logs in real-time
```

---

### **3. Hardware Configuration & Testing**

#### GPIO Wiring (Raspberry Pi)

- **Pause Button (GPIO17)**:
  - Button COM → Pi GND
  - Button NO → Pi GPIO17
  - Pull-up enabled in `gpiozero` code
  - Pressing grounds GPIO17 momentarily

- **Soft Power Button (J2 Header)**:
  - Connect momentary switch across J2 pins
  - Handled by Pi firmware (no custom code needed)

#### USB Audio Adapter Setup

```bash
# List audio devices
aplay -L

# If USB audio not showing, check dmesg
dmesg | grep -i usb

# Set USB adapter as default
sudo nano /etc/asound.conf
# Add or verify: defaults.ctl.card 1 (or appropriate USB card number)
```

#### Test Audio Playback

```bash
# Download or create a test WAV file
aplay /home/pi/forklift/alert.wav

# Adjust volume if needed
alsamixer
```

#### Verify Network & UDP

```bash
# Check Pi IP (should be 192.168.10.1)
ip addr show eth0

# Verify UDP port open and listening
sudo netstat -tuln | grep 5005

# Monitor incoming UDP packets
sudo tcpdump -i eth0 -n udp port 5005

# Test from development machine (manual UDP send)
python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(b'D1:45.5,D2:60.2\n', ('192.168.10.1', 5005))
"
```

---

## 🛠 Configuration & Tuning

### **Raspberry Pi Configuration** (`config.json`)

Edit `/home/pi/forklift/config.json` to customize system behavior:

```json
{
  "description": "Forklift Ultrasonic Warning System - Configuration",
  "udp_listen_port": 5005,
  "distance_threshold_cm": 80,
  "pause_duration_seconds": 120,
  "alert_wav_path": "/home/pi/forklift/alert.wav",
  "pause_button_gpio": 17,
  "min_interval_between_alerts_sec": 1.0,
  "audio_output": "usb"
}
```

#### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `udp_listen_port` | 5005 | UDP port Pi listens on for distance packets from ESP |
| `distance_threshold_cm` | 80 | **Alert threshold** — forks below this cm distance trigger horn |
| `pause_duration_seconds` | 120 | How long (seconds) the pause button silences alerts |
| `alert_wav_path` | `/home/pi/forklift/alert.wav` | Full path to horn warning audio file |
| `pause_button_gpio` | 17 | BCM GPIO pin number for pause button input |
| `min_interval_between_alerts_sec` | 1.0 | Minimum seconds between consecutive alerts (prevents spam) |
| `audio_output` | "usb" | Audio output: "usb" (USB adapter), "gpio_pwm" (GPIO 12/13), "headphone_jack" (3.5mm jack) |

#### Tuning Tips

- **Threshold Distance**: Set based on your mezzanine clearance. Typical: 60–100 cm.
  - Test with a ruler/tape measure to find safe distance
  - Account for sensor mounting angle and SR04 accuracy (±2–3 cm)

- **Pause Duration**: 120 sec (2 min) is typical. Increase if needed for repeated checks.

- **Alert Interval**: 1.0 sec prevents alert loop while ensuring multiple warnings. Decrease for urgent applications.

- **GPIO Pin**: If modifying button wiring, update this value. Standard: GPIO17.

### **ESP32-PoE Configuration** (`src/main.cpp`)

Edit the configuration section for NO-ROUTER static IP setup:

```cpp
// Raspberry Pi UDP target
const char *udp_target_ip = "192.168.10.1";       // Pi static IP
const uint16_t udp_target_port = 5005;            // UDP port

// Static IP Configuration (NO-ROUTER setup)
IPAddress local_IP(192, 168, 10, 20);             // ESP32 static IP (change to .21 for board #2)
IPAddress gateway(192, 168, 10, 1);               // Pi acts as gateway
IPAddress subnet(255, 255, 255, 0);               // Subnet mask

// Olimex ESP32-PoE Ethernet pins
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

// Measurement cycle timing
const unsigned long measurement_interval_ms = 100; // 10 readings/sec

// Sensor configuration
const uint8_t num_sensors = 2;                     // 1 or 2 sensors

// GPIO PIN DEFINITIONS
const uint8_t SR04_1_TRIG = 14;                   // Trigger pin for sensor 1
const uint8_t SR04_1_ECHO = 15;                   // Echo pin for sensor 1
const uint8_t SR04_2_TRIG = 16;                   // Trigger pin for sensor 2
const uint8_t SR04_2_ECHO = 32;                   // Echo pin for sensor 2
```

#### Key Settings

- **Network**: Ethernet-only (NO WiFi) via Olimex ESP32-PoE
- **Static IPs**: Pi = 192.168.10.1, ESP32 = 192.168.10.20 (no DHCP/router)
- **Pi IP Address**: Must be `192.168.10.1` to match NO-ROUTER setup
- **Measurement Interval**: 100 ms = 10 measurements per second (standard)
- **Number of Sensors**: Set to `1` if using only sensor #1, or `2` for dual sensors
- **GPIO Pins**: 14, 15, 16, 32 (updated from original 2, 4, 5, 6)

---

## 🧱 Physical Layout & Enclosure Design

### **Workbench Control Box (Raspberry Pi Side)**

#### **Internal Arrangement**

The control box houses all Pi-side electronics in a logical, service-friendly layout:

```
TOP VIEW (looking down at box interior)

┌─────────────────────────────────────────────────┐
│  [ FAN + FILTER ]  (exhaust vents to rear)      │
│                                                  │
│  ┌────────────────┐      ┌──────────────────┐   │
│  │ Raspberry Pi 5 │      │ 12V Amplifier    │   │
│  │ + PoE HAT      │      │ (80W, knob →)    │   │
│  │ (left side)    │      │ (right side)     │   │
│  └────────────────┘      └──────────────────┘   │
│                                                  │
│  ┌────────────────┐                             │
│  │ Geekworm UPS   │ (under or beside Pi)        │
│  │ X1200 + RTC    │                             │
│  └────────────────┘                             │
│                                                  │
│  [ USB→3.5mm Adapter ] short cable → Amp input  │
│                                                  │
│  [PoE Splitter for ESP] (lower left, near eth)  │
│                                                  │
│  CABLE ROUTING:                                 │
│    Left Zone:    PoE/Ethernet in & out           │
│    Center Zone:  5V logic (Pi, UPS, USB adapter)│
│    Right Zone:   12V power (Amp, Fan)           │
│                                                  │
└─────────────────────────────────────────────────┘
```

#### **Front Panel Cutouts**

```
┌─────────────────────────────────────────┐
│                                         │
│  [ BIG PAUSE BUTTON ]  [ VOLUME KNOB ]  │
│                                         │
│  [ SMALL J2 POWER BUTTON ]              │
│                                         │
│  (Optional: LED status indicator)       │
│                                         │
└─────────────────────────────────────────┘
```

#### **Rear Panel**

```
┌─────────────────────────────────────────┐
│                                         │
│  [Ethernet/PoE Input] [FAN EXHAUST VENTS] │
│                                         │
│  (Power cable entry if using PSU)       │
│                                         │
└─────────────────────────────────────────┘
```

#### **Component Spacing & Ventilation**

- **Pi + PoE HAT**: Mount on left side, stacked vertically on standoffs
- **UPS Board**: Position beneath or beside Pi, close to 5V supply
- **Amplifier**: Mount on right side with front-facing volume knob
- **Fan**: Install on top or rear, with intake filter
- **PoE Splitter**: Place near Ethernet input, preferably lower area
- **USB Adapter & Audio Cable**: Keep short, routed through center, away from 12V lines
- **12V Amplifier & Fan Wiring**: Route on right side, separated from 5V logic
- **Cable Management**: Use vertical channels, tie wraps, no sharp bends

---

### **Mezzanine Sensor Installation**

#### **Physical Layout**

At the mezzanine, sensors must be rigidly mounted for accuracy:

```
MEZZANINE END (near forklift approach)

┌──────────────────────────────────────┐
│                                      │
│  PoE Splitter (wall-mounted)         │
│    ├─ Ethernet in (from Pi side)    │
│    ├─ 5V out → ESP32 & sensors      │
│    └─ GND out → common ground       │
│                                      │
│  ESP32-PoE Module                    │
│    ├─ Mounted on bracket below       │
│    └─ GPIO: 14,16 (TRIG), 15,32 (ECHO)
│                                      │
│  SR04 Sensor #1                      │
│    ├─ Mounted on steel L-bracket #1  │
│    ├─ Facing downward at angle       │
│    └─ TRIG→GPIO14, ECHO→GPIO15 (÷)  │
│                                      │
│  SR04 Sensor #2 (optional)           │
│    ├─ Mounted on steel L-bracket #2  │
│    ├─ Positioned adjacent to #1      │
│    └─ TRIG→GPIO16, ECHO→GPIO32 (÷)  │
│                                      │
│  Voltage Dividers (for ECHO pins)    │
│    ├─ 2kΩ + 1kΩ resistors × 2       │
│    └─ Mounted near ESP for short     │
│        distance between sensor & divider
│                                      │
│  All wiring: Secure with ties,       │
│  no loose cables near forks          │
│                                      │
└──────────────────────────────────────┘
```

#### **Sensor Mounting Recommendations**

1. **Rigid Mounting**: Use steel L-brackets or custom aluminum mounts to prevent vibration
2. **Angle Setup**: Sensors typically mount at 30–45° downward to face approaching forks
3. **Spacing**: If using 2 sensors, mount ~20–30 cm apart horizontally or vertically
4. **Height**: Position sensors 1.5–2.5 meters above ground (typical forklift approach height)
5. **Clearance**: Ensure no obstacles block sensor ultrasonic "cone" (roughly ±15° from center)
6. **Weather Protection** (optional): If mezzanine is outdoors, cover with a simple rain shield

#### **Wiring at Mezzanine**

- All sensor and ESP power from PoE splitter's 5V output
- Common GND star point near splitter
- ECHO lines pass through voltage dividers (2k/1k) before reaching ESP GPIO
- Cat6 cable routed clearly with strain relief at splitter
- Cables secured to avoid snagging on moving equipment

### **Control Box Layout**

* Left: Raspberry Pi + PoE HAT
* Under/side: UPS
* Right side: Amplifier
* Top panel: Fan + filter
* Front panel: Big pause button, small J2 power button, volume knob
* Rear panel: Ethernet/PoE input + ventilation
* Inside bottom: PoE splitter for ESP path
* Cable left: Network
* Cable center: Logic
* Cable right: 12V

### **Mezzanine Layout**

* ESP32 + PoE splitter local mount
* 1–2 SR04 sensors in L-shape
* Fixed rigid brackets for accuracy

---

## 🚨 Operation & Control

### **Normal Operation**

**System Startup:**
1. Power applied via PoE HAT to Raspberry Pi
2. Pi boots, systemd automatically starts `forklift` service
3. ESP32 connects to WiFi and begins sensor polling
4. Pi listens on UDP port 5005 for distance packets
5. Status appears in logs (accessible via `journalctl -u forklift`)

**Continuous Monitoring:**
- ESP sends distance packets every 100 ms (10/sec)
- Pi receives and parses `D1:xx.x,D2:yy.y` packets
- Distance compared against 80 cm threshold (configurable)
- If distance < threshold → **horn alarm sounds immediately**
- Alert repeats every ~1 second while condition persists

**Alarm Ceases When:**
- Fork moves away (distance > threshold)
- Pause button pressed (silence for 2 minutes)
- Manual pause via configuration change

### **User Controls**

#### **Large Pause Button** (front panel)
- **Single Press**: Silences all alerts for 2 minutes (120 seconds)
- **Visual Feedback**: Monitor logs to see pause status
- **Multiple Presses**: Each press resets the 2-minute timer
- **Use Case**: Supervisor needs to move forks without repeated alarms, e.g., for inspection

#### **Small J2 Power Button** (front panel)
- **Short Press**: Initiates clean Pi shutdown
- **During Shutdown**: Systemd gracefully stops services
- **Power Off**: After ~30 sec, system fully powers down
- **Press Again to Boot**: Short press again to power on (firmware dependent)
- **Use Case**: Safe end-of-shift shutdown, maintenance mode

#### **System Monitoring** (via SSH)

Check system status in real-time:

```bash
# View live logs with color highlighting
journalctl -u forklift -f

# Sample output:
# [Alert] Horn triggered at 14:32:45
# [OK] D1= 45.2cm D2= 67.8cm Min= 45.2cm [Data OK]
# [Button] PAUSE button pressed
# [Pause] Alerts paused for 2.0 minutes until 14:34:45
```

### **Power Loss & UPS Behavior**

**During Power Outage:**
1. PoE HAT detects loss of input power
2. **Geekworm X1200 UPS** instantly switches to battery
3. Pi continues running uninterrupted (typically 5–10 min battery life)
4. RTC on UPS maintains accurate time during outage
5. ESP32 also loses power (no battery at mezzanine), stops sending packets
6. Pi may log "Data STALE" warning in monitor output

**On Power Restore:**
1. UPS detects AC return, begins charging battery
2. PoE HAT re-supplies power to Pi
3. Pi resumes normal operation (no reboot needed if battery lasted)
4. ESP32 reboots, reconnects to WiFi, resumes sensor polling
5. System automatically resumesAlertMonitoring

**Recommended UPS Settings:**
- Set low-battery alarm threshold: ~10% (for early warning)
- Configure graceful shutdown at 5% (if desired)
- Battery capacity: ~10Ah (Geekworm standard) = ~5 min runtime at 5W

### **Maintenance & Troubleshooting**

#### **Audio Not Working**

```bash
# Check if USB audio adapter is detected
aplay -L
# Look for: USB Audio (or similar)

# List ALSA cards
cat /proc/asound/cards

# Test audio playback manually
aplay /home/pi/forklift/alert.wav

# Check system logs
journalctl -u forklift | grep -i audio
```

#### **No Distance Data**

```bash
# Verify ESP32 can reach Pi
# From Pi, check UDP listening:
sudo netstat -tuln | grep 5005

# Monitor UDP traffic (if netcat installed)
timeout 10 nc -u -l 5005

# Check ESP serial output via USB:
# (Connect ESP to Windows/Mac with USB cable)
# Open serial monitor at 115200 baud
# Verify: "UDP listener started..." and distance readings
```

#### **Threshold Not Triggering**

```bash
# Check current distance readings in logs
journalctl -u forklift -f

# Manually test alert playback
aplay /home/pi/forklift/alert.wav

# Verify threshold in config
cat /home/pi/forklift/config.json | grep distance_threshold

# Lower threshold temporarily to test
nano /home/pi/forklift/config.json
# Change "distance_threshold_cm": 80 to "distance_threshold_cm": 200
# Save, then restart service:
sudo systemctl restart forklift
```

#### **Pause Button Not Working**

```bash
# Verify GPIO17 configuration
cat /home/pi/forklift/config.json | grep pause_button_gpio

# Check user permissions for GPIO
groups pi
# Should include "gpio" group

# Test GPIO manually (if gpiozero installed)
python3 -c "from gpiozero import Button; b = Button(17); print('Button status:', b.is_pressed)"

# Check wiring: ensure button COM → GND, NO → GPIO17
```

## 📚 Additional Documentation

- **`schematics.md`**: Detailed step-by-step wiring guide for all components
- **`layout.md`**: Physical component placement and enclosure layout diagrams
- **`structure.md`**: Project directory organization reference

## 🔒 System Safety & Compliance

### **Design Safeguards**

1. **Voltage Dividers**: All 5V sensor signals properly stepped down to 3.3V for ESP GPIO
2. **Common Ground**: All circuits share ground via PoE network for signal integrity
3. **PoE Isolation**: Power distribution isolated at splitter, protecting mezzanine equipment
4. **Over-Current Protection**: Use appropriate breakers/fuses on 12V amplifier circuit
5. **Cable Strain Relief**: All connectors secured against accidental disconnection
6. **Sensor Mounting**: Rigid brackets prevent vibration-induced false readings

### **Operational Safety**

- **Always test with a tape measure** before deploying thresholds
- **Never rely solely on this system** for heavy equipment safety
- **Use in conjunction with** proper OSHA forklift training and procedures
- **Regular audits**: Check sensor alignment monthly
- **Replace batteries annually** in UPS board (Geekworm standard)

## 🔧 Quick Reference

### **Essential Commands (Raspberry Pi)**

```bash
# View live system status
journalctl -u forklift -f

# Restart the alert system
sudo systemctl restart forklift

# Check if service is running
sudo systemctl status forklift

# Stop the service (manual mode)
sudo systemctl stop forklift

# View last 50 lines of logs
journalctl -u forklift -n 50

# Check UDP listening on port 5005
sudo netstat -tuln | grep 5005
```

### **Essential Commands (ESP32)**

```bash
# Build firmware
pio run -t build

# Upload to ESP
pio run -t upload

# Monitor serial output
pio device monitor --baud 115200

# Clean build (if issues occur)
pio run -t clean
```

## 📋 Deployment Checklist

- [ ] **ESP32 Firmware**: WiFi SSID/password updated
- [ ] **ESP32 Firmware**: Pi IP address verified
- [ ] **Voltage Dividers**: 2k/1k resistors soldered on SR04 ECHO lines
- [ ] **Raspberry Pi**: Dependencies installed (`gpiozero`, `alsa-utils`)
- [ ] **Raspberry Pi**: Files copied to `/home/pi/forklift/`
- [ ] **Audio File**: `alert.wav` placed in `/home/pi/forklift/`
- [ ] **GPIO Wiring**: Pause button connected to GPIO17 and GND
- [ ] **J2 Header**: Power button wired across J2 pins
- [ ] **12V Wiring**: Amplifier and fan on 12V rail with proper polarity
- [ ] **PoE Cable**: Cat6 PoE running from injection point to mezzanine splitter
- [ ] **Sensors**: SR04s mounted rigidly on L-brackets at correct angle
- [ ] **Systemd Service**: `forklift.service` copied and enabled
- [ ] **Manual Test**: Run `python3 app.py` and verify UDP reception
- [ ] **Threshold Tuning**: Measure actual distances and set threshold appropriately
- [ ] **Audio Test**: Play `alert.wav` and verify horn volume
- [ ] **Reboot Test**: Power cycle Pi and confirm autostart works

---

## 📄 License & Usage

This project is designed for **client-specific deployment** only.  
Distribution, modification, or reuse requires explicit written permission.

**Version**: 1.0  
**Last Updated**: December 2024  
**Status**: Production Ready

---

For technical support, refer to inline code comments in:
- `esp32/src/main.cpp` — ESP32 sensor & networking details
- `raspberrypi/app.py` — Raspberry Pi alert system architecture
- `schematics.md` — Complete wiring diagrams and GPIO mapping
