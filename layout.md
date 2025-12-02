No wiring, no functional explanation — **just the physical layout of components** for inside the enclosure and at each end of the system.

You can paste this directly into documentation or send to the client.

---

# 🔧 **Forklift Warning System – Component Layout (MD Format)**

## **📦 Enclosure (Workbench / Raspberry Pi Side)**

### **Top-Down Internal Layout**

```
+------------------------------------------------------------+
|                                                            |
|   [ 12V FAN + FILTER ]   (top panel)                       |
|                                                            |
|  +-------------------+     +-----------------------------+ |
|  | Raspberry Pi 5    |     |  12V Amplifier (80W)        | |
|  | + PoE HAT         |     |  (Volume knob facing front) | |
|  +-------------------+     +-----------------------------+ |
|                                                            |
|  +-------------------+                                     |
|  | Geekworm X1200    |   (UPS under or beside Pi)         |
|  +-------------------+                                     |
|                                                            |
|        [ USB→3.5mm Adapter ]  → Short audio lead → Amp     |
|                                                            |
|  [ PoE Splitter for ESP link ]  (rear left of enclosure)   |
|                                                            |
|  Cable Routing:                                            |
|    - Left: Network + PoE entry                             |
|    - Center: Logic wiring (Pi, UPS)                        |
|    - Right: 12V wiring (Amp, Fan)                          |
+------------------------------------------------------------+
```

### **Front Panel**

```
+--------------------------------------------------+
|  [ BIG BUTTON – PAUSE ]   [ SMALL BUTTON – POWER ]  |
|                                                    |
|                 (AMPLIFIER KNOB)                  |
+--------------------------------------------------+
```

### **Rear Panel**

```
+--------------------------------------------------+
|  [ ETH / PoE Jack ]       [ FAN EXHAUST VENTS ]  |
+--------------------------------------------------+
```

---

## **📡 Sensor End (Mezzanine / ESP Side) Layout**

### **ESP + Sensor Assembly Layout**

```
+--------------------------------------------+
|  PoE Splitter (54V→5V)                     |
|    - Mounted to bracket or wall            |
|                                            |
|  ESP32-C6 Board                            |
|    - Mounted above or beside splitter      |
|                                            |
|  SR04 Sensor #1                            |
|  SR04 Sensor #2 (optional)                 |
|    - Placed in L-shape orientation         |
|    - Mounted to steel brackets / L-intons  |
+--------------------------------------------+
```

---

## **📏 Recommended Spacing / Positioning**

### **Inside enclosure:**

* Pi + PoE HAT on left
* UPS under or beside Pi
* Amplifier on right with front access
* Fan on top or rear
* Buttons on front panel
* PoE splitter near Ethernet entry
* Keep **12V amp area** physically separate from **5V logic area**
* Use vertical cable channels where possible

### **At mezzanine:**

* ESP and splitter kept together
* Sensors mounted on rigid steel L-brackets
* Sensors at known fixed angles (forming the L-shape detection area)
* Cat6 cable secured with strain relief

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
