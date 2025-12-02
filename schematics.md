* **A. Raspberry Pi side (bench / control box)**
* **B. ESP32 + SR04 side (mezzanine / sensor end)**


* **Pause button:** Pi GPIO17
* **SR04 #1:** TRIG → ESP GPIO2, ECHO → ESP GPIO5
* **SR04 #2:** TRIG → ESP GPIO4, ECHO → ESP GPIO6


---

## A. Raspberry Pi Side – Step-by-Step Wiring

### 1. Mount and power the Pi + PoE HAT + UPS

1. Mount the **Raspberry Pi 5** on standoffs inside the box.
2. Plug the **PoE HAT** onto the Pi’s GPIO header (stacked on top as designed).
3. Mount the **Geekworm X1200 UPS** close to the Pi.
4. Connect the UPS to the Pi using the supplied cable (usually USB-C or 5V/GND header as per Geekworm’s manual).
5. Insert the **RTC coin cell** into the UPS board.

> Network/power:
>
> * Connect the Cat6 from your **PoE injector/switch** to the **PoE HAT Ethernet port**.
> * That gives the Pi both **power and network**.

---

### 2. USB audio from Pi to amplifier

1. Plug the **USB → 3.5mm audio adapter** into one of the Pi’s USB ports.
2. Plug a **3.5mm male-male audio cable** into the USB adapter’s headphone jack.
3. Mount your **12V 80W amp** in the box (volume knob facing towards the panel).
4. Plug the **other end of the 3.5mm cable** into the amp’s audio input (LINE IN / AUX IN).

---

### 3. Amplifier power and horn wiring

1. Provide **12V DC** to the amplifier’s power terminals (respect polarity: +12V and GND).

   * This 12V can be from a dedicated 12V PSU or a 12V rail inside your box.
2. Connect the **horn speaker** wires to the amplifier’s **speaker output terminals** (L+/L− or mono output as per amp).

---

### 4. Big pause button to Pi GPIO

We’ll use **GPIO17** for the pause button (matches the Python script):

1. On the big white **momentary button**, identify the two switch contacts (NO – normally open, and COM).
2. Run a wire from **COM** → **Pi GND** (any ground pin on the Pi GPIO header).
3. Run a wire from **NO** → **Pi GPIO17** (BCM numbering; pin 11 on the header).

The Python code uses `pull_up=True`, so wiring is:

* Button **open** → GPIO17 pulled high internally
* Button **pressed** → GPIO17 shorted to GND → button press detected

---

### 5. Small power button to Pi J2 header

This is the **Pi soft power** button for clean startup/shutdown.

1. Locate the **J2 header** on the Raspberry Pi 5 (2-pin power header).
2. Wire the **small momentary button** across J2’s two pins (no polarity).

   * One button terminal → J2 pin 1
   * Other button terminal → J2 pin 2

A short press will tell the Pi firmware to power on or initiate shutdown (depending on state).

---

### 6. 12V fan and dust filter

1. Mount the **12V fan** over the fan opening, with the **airflow direction** set to either:

   * **Inlet** (pulling clean air in through the filter)
   * Or **exhaust** (pushing hot air out) — just be consistent with your design.
2. Wire the **fan + (red)** to the **+12V rail**.
3. Wire the **fan − (black)** to the **12V GND**.

If you want it always on, that’s it.
(If you later decide to switch it with a MOSFET from a GPIO, we can add that.)

---

## B. ESP32 + SR04 Side – Step-by-Step Wiring

This is the **mezzanine end**, near the forklift.

### 1. PoE splitter to ESP and sensors

1. Connect a **Cat6 cable** from your PoE switch/injector out to the **PoE splitter** at the mezzanine.

2. On the PoE splitter:

   * Plug the **Ethernet output** into the ESP’s Ethernet (if using Ethernet)
   * Use the **5V output** (barrel/DC wires) as the 5V supply.

3. Wire splitter **5V+** to:

   * ESP board **5V/VIN**
   * SR04 **VCC**

4. Wire splitter **GND** to:

   * ESP **GND**
   * SR04 **GND**

> Important: ESP, SR04, and Pi must share a **common ground via the network/PoE system**, but practically, powering ESP + sensors from the splitter’s 5V and all GNDs tied together is enough at this end.

---

### 2. SR04 #1 wiring

We’ll match the firmware’s default pin choices:

* **TRIG1 → ESP GPIO2**
* **ECHO1 → ESP GPIO5** (through divider)

**Steps:**

1. SR04 #1 **VCC** → 5V from PoE splitter.

2. SR04 #1 **GND** → ESP GND.

3. SR04 #1 **TRIG** → **ESP GPIO2** (wire directly).

4. For **ECHO** (5V output → needs to go to 3.3V input):

   * Build a simple **resistor divider**:

     * Connect a **2kΩ resistor** from **ECHO** to ESP GPIO5.
     * Connect a **1kΩ resistor** from **ESP GPIO5** to GND.
   * Then wire **SR04 ECHO** → top of divider (junction of ECHO and 2k).

This drops 5V to roughly 3.3V at the ESP input.

---

### 3. SR04 #2 wiring (if used)

Firmware mapping:

* **TRIG2 → ESP GPIO4**
* **ECHO2 → ESP GPIO6** (via divider)

**Steps:**

1. SR04 #2 **VCC** → 5V from PoE splitter (same 5V rail).
2. SR04 #2 **GND** → ESP GND (same GND rail).
3. SR04 #2 **TRIG** → **ESP GPIO4** directly.
4. For **ECHO2**:

   * Connect a **2kΩ resistor** from **ECHO2** to **ESP GPIO6**.
   * Connect a **1kΩ resistor** from **ESP GPIO6** to GND.
   * SR04 ECHO2 goes to the top of this divider.

---

### 4. ESP32-C6 power and network

1. ESP **5V/VIN** → 5V from PoE splitter.
2. ESP **GND** → GND from PoE splitter.
3. For **network**:

   * If using **Wi-Fi**: nothing to wire, just configure SSID/password in code.
   * If using **Ethernet**: connect the splitter’s Ethernet port to the ESP’s Ethernet-capable board/switch as needed.

---

### 5. Final sanity check (ESP side)

* All **VCC pins** (ESP + SR04s) on the **same 5V rail**.
* All **GND pins** tied together.
* TRIG pins go **direct** from ESP GPIO to SR04 TRIG.
* ECHO pins go **through resistor dividers** into ESP GPIOs.
* No ECHO pin should ever be wired straight 5V → ESP GPIO.

---

## C. End-to-End Signal Flow Recap

1. **SR04 sensors** measure distance → analog echo pulse.
2. **ESP32-C6** sends TRIG pulses, reads ECHO pulses, converts to cm.
3. ESP sends data over **UDP** (Wi-Fi/Ethernet) to the **Raspberry Pi 5**.
4. Pi receives values, compares to **distance_threshold_cm**.
5. If forks “too high” → Pi plays **alert.wav** → USB audio → amp → horn.
6. **Big button** on Pi → pauses alerts for 2 minutes.
7. **Small button on J2** → starts or cleanly shuts down the Pi.
8. UPS keeps Pi up long enough for clean shutdowns and protects against dips.

---

If you tell me **exactly which ESP board you have** (e.g. “ESP32-C6 DevKitM-1” etc.), I can map the GPIO names to **physical pin numbers** for you, so you literally just follow: “top-left pin → TRIG1”, “second pin → GND”, etc.
