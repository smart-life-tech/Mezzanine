# 🚧 Forklift Ultrasonic Warning System

**Raspberry Pi 5 + ESP32-C6 + SR04 Sensors**
**Real-time forklift fork-height detection with audio alert**

---

## 📘 Overview

This system prevents forklifts from striking a mezzanine or overhead structure by continuously monitoring fork height using one or two HC-SR04 ultrasonic sensors.
The sensors connect to an ESP32-C6 located at the mezzanine, which sends real-time distance data to a Raspberry Pi 5 over the network. When the fork height crosses a defined threshold, the Pi triggers a loud warning through a USB audio interface, 12V amplifier, and horn speaker.

The entire system is housed in a structured, well-laid-out control enclosure at the workbench, while the sensing hardware sits remotely at the mezzanine end via PoE.

---

## 📦 System Components

### **Raspberry Pi (Control Box – Workbench Side)**

* Raspberry Pi 5
* PoE HAT (for Pi power + network)
* Geekworm X1200 UPS + RTC battery
* USB → 3.5mm audio adapter
* 12V / 80W amplifier
* Horn speaker
* Big momentary pause button (2 min pause)
* Small J2 soft-power button
* 12V fan + dust filter
* PoE splitter (for ESP downlink)
* Cat6 network input

### **ESP Side (Mezzanine – Sensor End)**

* ESP32-C6 module
* One or two HC-SR04 ultrasonic sensors
* 54V → 5V PoE splitter
* Cat6 PoE uplink from Pi
* Steel L-brackets (sensor mounting)

---

## 🧩 System Architecture

**Flow:**
`SR04 → ESP32 → UDP Network → Raspberry Pi 5 → Audio Alert → Horn`

* ESP32 reads sensors continuously (100 ms cycle)
* Sends packet: `D1:xxx,D2:xxx\n`
* Pi receives data, checks threshold, triggers alert WAV
* Pause button disables alerts for 2 minutes
* J2 button controls soft start/shutdown
* UPS ensures clean power handling
* Fan keeps enclosure cool

---

## 📁 Project Structure

```
/forklift-system
│
├── firmware/
│   ├── esp32_sr04_udp.ino        # ESP32 firmware
│   └── wiring_notes.md
│
├── raspberry-pi/
│   ├── forklift_alert.py         # Main Pi-side alert logic
│   ├── config.json               # Thresholds & settings
│   ├── alert.wav                 # Warning audio file
│   └── forklift.service          # systemd autostart file
│
├── enclosure/
│   ├── layout.md                 # Internal layout notes
│   ├── panel_templates.pdf       # Front/Rear/Top cutouts
│   ├── mounting_grid.pdf         # Standoff map
│   └── wiring_diagram.pdf        # Visual wiring schematic
│
└── README.md                     # This file
```

---

## ⚙️ Installation

### **1. Raspberry Pi Setup**

```bash
sudo apt update
sudo apt install -y python3-gpiozero python3-pip alsa-utils
```

Copy files into:

```
/home/pi/forklift/
    forklift_alert.py
    config.json
    alert.wav
```

Test manually:

```bash
python3 forklift_alert.py
```

### **Enable Autostart**

```bash
sudo cp forklift.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable forklift
sudo systemctl start forklift
```

---

## 🌐 ESP32 Setup

1. Open `esp32_sr04_udp.ino` in Arduino IDE / PlatformIO
2. Update:

   * WiFi SSID/PASSWORD
   * Raspberry Pi IP
   * Number of sensors
3. Flash ESP32-C6
4. Place the ESP and SR04s at the mezzanine, powered by PoE splitter

---

## 🛠 Configuration

Edit `/home/pi/forklift/config.json`:

```json
{
  "udp_listen_port": 5005,
  "distance_threshold_cm": 80,
  "pause_duration_seconds": 120,
  "alert_wav_path": "/home/pi/forklift/alert.wav",
  "pause_button_gpio": 17,
  "min_interval_between_alerts_sec": 1.0
}
```

---

## 🧱 Layout (Physical Placement)

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

## 🚨 Operation

* System runs continuously.
* If SR04 detects forks below threshold → **Immediate horn alert**.
* Press big button → **system paused for 2 minutes**.
* Press small button → **clean Pi shutdown/start**.
* UPS protects against power loss.

---

## 📄 License

Closed, project-specific — distribution only for client.

---

If you'd like the README **branded, with your name, a logo banner, or a version for GitHub**, I can format it accordingly.
