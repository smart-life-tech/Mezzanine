No wiring, no functional explanation — **just the physical layout of components** for inside the enclosure and at each end of the system.

You can paste this directly into documentation or send to the client.

---

# 🔧 **Forklift Warning System – Component Layout (MD Format)**

## **📦 Pi Stack (Workbench / Raspberry Pi Side)**

### **Vertical Stack Configuration**

```
                    ┌─────────────────────┐
                    │   12V COOLING FAN   │  ← Top Layer
                    │   (exhaust upward)  │
                    ├─────────────────────┤
                    │                     │
                    │  RASPBERRY PI 5     │  ← Middle Layer
                    │  (GPIO facing up)   │
                    │                     │
                    ├─────────────────────┤
                    │   PoE HAT / PSU     │  ← Bottom Layer
                    │   (power input)     │
                    └──────────┬──────────┘
                               │
                          [Cat6 PoE]
                               │
                         PoE Switch
                      (192.168.10.x)


Network Connection:
  • Single Cat6 cable from PoE switch to Pi stack
  • Pi static IP: 192.168.10.1/24
  • No DHCP, no router configuration

Audio Output (configurable in config.json):
  • Option 1: USB audio adapter → Amplifier/Speaker
  • Option 2: GPIO 12/13 PWM audio (Pi 5 native)
  • Option 3: Onboard 3.5mm headphone jack

Control Interface:
  • GPIO17: Pause button (2 min silence)
  • J2 header: Soft power button (optional)
```

### **Control Panel (if using external enclosure)**

```
Front Panel:
+--------------------------------------------------+
|  [ BIG BUTTON – PAUSE ]   [ SMALL BUTTON – POWER ] |
|                                                    |
|        (Optional: Volume control if using amp)     |
+--------------------------------------------------+

Rear/Side Connections:
  • Cat6 Ethernet jack (to PoE switch)
  • USB ports (for audio adapter if used)
  • GPIO header access for pause button
  • Fan exhaust vents
```

---

## **📡 Sensor End (Mezzanine / ESP Side) Layout**

### **ESP32-PoE + Sensor Assembly**

```
                    [Cat6 from PoE Switch]
                              │
                    ┌─────────▼─────────┐
                    │  Olimex ESP32-PoE │
                    │  (Integrated PoE) │
                    │  192.168.10.20    │
                    └──────┬───┬────────┘
                           │   │
                      3.3V │   │ GND
                           │   │
            ┌──────────────┴───┴──────────────┐
            │                                  │
       ┌────▼────┐                       ┌────▼────┐
       │ SR04 #1 │                       │ SR04 #2 │
       │ (VCC)   │                       │ (VCC)   │
       │ TRIG←14 │                       │ TRIG←16 │
       │ ECHO→15 │                       │ ECHO→32 │
       │ (GND)   │                       │ (GND)   │
       └─────────┘                       └─────────┘

Mounting:
  • ESP32-PoE: Wall bracket or mounting plate
  • Sensors: Steel L-brackets for rigid positioning
  • NO voltage dividers (all 3.3V direct connections)
  • Wire length: Keep under 30cm for signal integrity
  
Network:
  • Ethernet-only (no WiFi capability)
  • Static IP: 192.168.10.20
  • UDP destination: 192.168.10.1:5005
```

---

## **📏 Recommended Spacing / Positioning**

### **Pi Stack Assembly:**

* **Vertical Configuration** (bottom to top):
  1. Power supply (PoE HAT or dedicated PSU)
  2. Raspberry Pi 5 board (GPIO header accessible)
  3. Cooling fan (exhaust upward)
* **Mounting**: Use standoffs between layers for airflow
* **Clearance**: 10-15mm between Pi and fan for heat dissipation
* **Cable Management**: Keep Cat6 and USB cables organized
* **Stability**: Secure stack to prevent toppling

### **If using external enclosure:**

* Place Pi stack in center or left side
* Optional amplifier on right side (if using USB audio)
* Front panel buttons with clean wire routing
* Rear panel for Ethernet connection and fan exhaust
* Keep high-voltage/high-current circuits separated from logic

### **At mezzanine:**

* **ESP32-PoE**: Wall-mounted with secure bracket
* **Sensors**: Rigid steel L-brackets at fixed angles
* **Sensor Spacing**: 20-30cm apart for coverage area
* **Wire Runs**: Keep under 30cm, use 22-24 AWG
* **Cat6 Cable**: Secured with strain relief, away from moving equipment
* **NO External Components**: All power integrated in ESP32-PoE board

---

## **🗂 Mounting Grid (Internal Base Plate)**

```
+------------------------------------------------------+
|  o Pi Mount (4x holes)                               |
|                                                      |
|                o UPS Mount (4x holes)                |
|                                                      |
|  o Amplifier Mount (4x holes)                        |
|                                                      |
|                     o PoE Splitter Mount (2–4x)      |
+------------------------------------------------------+
```

---

## **🧩 Component Grouping Summary**

### **Raspberry Pi Side (Workbench Box)**

* Raspberry Pi 5 + PoE HAT
* Geekworm X1200 UPS
* USB → 3.5mm audio adapter
* 12V amplifier
* Enclosure fan + filter
* PoE splitter (for ESP link)
* Big pause button
* Small Pi power button
* Horn speaker output terminals
* Cat6 PoE network input

### **ESP Side (Mezzanine)**

* ESP32-C6
* 1–2 HC-SR04 sensors
* PoE splitter
* Cat6 line from Pi side

---

If you want, I can convert this into a **visual diagram**, **PDF layout**, or **CAD-style placement map** based exactly on your enclosure’s dimensions.
