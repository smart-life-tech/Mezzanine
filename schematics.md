# Forklift Ultrasonic Warning System – Complete Wiring Schematic

**System Overview:**
- **Workbench Pi Stack**: Raspberry Pi 5 + PoE HAT + Fan (vertical stack)
- **Network**: PoE Switch (NO ROUTER) connecting Pi and ESP32
- **Mezzanine Sensor End**: Olimex ESP32-PoE + 2× HC-SR04 ultrasonic sensors
- **Static IPs**: Pi = 192.168.10.1, ESP32 = 192.168.10.20, no DHCP
- **Audio**: Configurable (USB adapter, GPIO 12/13 PWM, or headphone jack)

**Key Pinouts (GPIO Assignment):**
- **Pause button**: Raspberry Pi GPIO17 (Pin 11 on GPIO header)
- **SR04 #1**: TRIG → ESP GPIO14, ECHO → ESP GPIO15 (3.3V direct, no divider)
- **SR04 #2**: TRIG → ESP GPIO16, ECHO → ESP GPIO32 (3.3V direct, no divider)
- **Audio**: USB adapter OR GPIO 12/13 OR headphone jack (config.json setting)

---

## Power Distribution Architecture

This system uses **PoE (Power over Ethernet)** as the primary power backbone, providing clean, centralized distribution to all components:

### **PoE Power Flow Diagram**

```
               PoE Switch (48V)
               (NO ROUTER)
                    │
         ┌──────────┼──────────┐
         │           │           │
    [Cat6 PoE]       │      [Cat6 PoE]
         │           │           │
   Pi Stack         │      ESP32-PoE
  (Workbench)       │     (Mezzanine)
  192.168.10.1      │    192.168.10.20
   5V/3A            5V/2A
      │                │
   Raspberry Pi    ESP32-PoE
   + UPS           + SR04s
      │                │
      └────── Common Ground ──────┘
         (via PoE cable shield)
```

### **Workbench Power Distribution (Pi Stack)**
- **Pi Stack Configuration:**
  - **Bottom Layer**: PoE HAT or dedicated power supply
  - **Middle Layer**: Raspberry Pi 5 board
  - **Top Layer**: 12V cooling fan
- **PoE HAT**: Receives 48V PoE from switch, converts to 5V for Pi
- **Network**: Single Cat6 cable to PoE switch (power + data)
- **Static IP**: 192.168.10.1/24 configured in /etc/dhcpcd.conf
- **Audio Options**: USB adapter, GPIO 12/13 PWM, or 3.5mm jack

### **Mezzanine Power Distribution**
- **Olimex ESP32-PoE**: Integrated PoE (receives 48V directly, no external splitter)
- **3.3V Rail**: Onboard 3.3V regulator powers ESP32 and HC-SR04 sensors
- **Static IP**: 192.168.10.20 configured in ESP32 firmware
- **Common Ground**: All sensors tied to ESP32 GND pin
- **NO Voltage Dividers**: Sensors powered from 3.3V, ECHO outputs are 3.3V logic

---

## A. Raspberry Pi Side – Complete Wiring Guide

### **Section 1: Mounting and PoE Power**

```
PoE Injector (your wall switch/device)
         │
    [Cat6 PoE Cable]
         │
   ┌─────▼───────────┐
   │  PoE HAT        │
   │  (on Pi GPIO)   │
   │ Ethernet Port ◄─┤ Cat6 input
   │                 │
   │ Regulates:      │
   │ 48V → 5V/GND    │
   └────────┬────────┘
            │
      ┌─────▼──────────┐
      │ Raspberry Pi 5 │
      │ (powered by    │
      │  PoE HAT)      │
      └─────┬──────────┘
            │
      ┌─────▼──────────┐
      │ Geekworm X1200 │
      │ UPS + RTC      │
      └────────────────┘
```

**Installation Steps:**

1. **Prepare Standoffs**:
   - Install 4× M2.5 standoffs (15mm height) on left side of enclosure

2. **Mount Raspberry Pi 5**:
   - Align 4 mounting holes with standoffs
   - Screw down with M2.5 screws
   - Ensure GPIO header faces upward and is accessible

3. **Stack PoE HAT**:
   - Align 40-pin PoE HAT connector with Pi GPIO header
   - Gently press down until fully seated
   - Optional: Secure with small standoffs if HAT has mounting holes

4. **Connect Cat6 PoE Cable**:
   - Plug Cat6 from your **PoE injector/switch** into **PoE HAT's Ethernet port**
   - Use proper RJ45 connector with strain relief
   - **No separate power cable needed** — entire Pi powered via PoE

5. **Verify PoE Power**:
   - Check for LED on PoE HAT (indicates power)
   - Pi should boot within 30 seconds
   - Check `/sys/kernel/debug/usb/dwc2/hw_params` or similar for power status

6. **Install UPS**:
   - Mount Geekworm X1200 near Pi
   - Connect via **USB-C port** (preferred) OR
   - Connect via **5V/GND header** (alternative, check manual)
   - Insert **CR2032 coin cell** into RTC slot
   - UPS LED should light (charging indicator)

---

### **Section 2: USB Audio Adapter to Amplifier**

```
Raspberry Pi 5              Audio Adapter         12V Amplifier
              
USB Port #3                                      
    │                                           
    ├─→ [USB-3.5mm Adapter]                    
         (analog audio out)
              │
         [3.5mm Cable]
         (stereo pair)
              │
              ├─→ [Amplifier LINE IN jack]
              │
              └─→ [Left/Right/Ground connections]
```

**Wiring Steps:**

1. **Select USB Port**:
   - Use **USB port 2 or 3** (farther from power circuits)
   - Avoid USB port 1 (closest to power header)

2. **Plug Audio Adapter**:
   - Insert **USB-to-3.5mm adapter** into selected USB port
   - Verify LED lights on adapter (power indication)

3. **Prepare Audio Cable**:
   - Use **3.5mm stereo headphone cable** (male-male connectors)
   - Length: ~30 cm maximum to reduce noise
   - If hum appears, use **shielded audio cable** instead

4. **Route Cable**:
   - Pass through **center cable channel** of enclosure
   - Keep away from 12V amp wiring (minimize EMI)
   - Use velcro straps or split loom for securing
   - Avoid sharp bends (minimum 5cm bend radius)

5. **Connect to Amplifier**:
   - Plug one end into **USB adapter's 3.5mm jack**
   - Plug other end into **amp's LINE IN or AUX IN jack**
   - Ensure connections are tight (no intermittent contact)

6. **Test Audio**:
   ```bash
   # List audio devices
   aplay -L
   
   # Look for USB device in output
   # Test playback
   aplay /home/pi/forklift/alert.wav
   
   # Adjust volume with ALSA mixer
   alsamixer
   # Navigate to USB device, set PCM level to 80-90%
   ```

---

### **Section 3: 12V Amplifier & Horn Speaker Wiring**

```
12V PSU               Amplifier                 Speaker
(Separate)               │
   │                 ┌────┴─────────┐
   │ +12V ──────────→│ POWER+        │
   │                │               │
   │ GND ──────────→│ GND           │
   │                │               │
   │           Audio In ◄─ 3.5mm    │
   │           (from USB)           │
   │                │               │
   │                │ SPEAKER+ ────→├─→ Horn +
   │                │ SPEAKER- ────→└─→ Horn -
   │                └───────────────┘
   │
   │                Horn Speaker
   │                (8Ω passive)
   │                  │
   └──[15A Fuse]─────→├─→ Connection
                      │
                     [Bracket]
```

**Power Supply Setup:**

1. **PSU Selection**:
   - Dedicated **12V PSU** (isolated from Pi's PoE system)
   - Specifications: **12V DC, 15A minimum, 180W**
   - Options:
     - Wall-mounted 12V PSU in enclosure
     - 12V converter from PoE (if available)
     - Separate 12V battery system (UPS-style)

2. **Fusing & Protection**:
   - Install **15A fuse or breaker** between PSU positive and amplifier
   - Location: ~30cm from PSU output
   - Protects circuit from short-circuit or overcurrent

3. **Wire Selection**:
   - **Wire gauge**: 10 AWG minimum (prefer 8 AWG for long runs)
   - **Distance**: Keep PSU-to-amp < 5 meters (minimize voltage drop)
   - Strip ~6mm insulation from each end

4. **Amplifier Power Terminals**:
   - **RED wire** (12V+) → Amp's **+12V terminal**
   - **BLACK wire** (GND) → Amp's **GND terminal**
   - Loosen screw terminal with Phillips screwdriver
   - Insert wire, twist tightly, retighten screw
   - **⚠️ CHECK POLARITY — Reversed connection destroys amp!**

5. **Voltage Verification**:
   ```bash
   # Before connecting anything:
   # Use multimeter in DC voltage mode
   # Red probe: PSU +12V
   # Black probe: PSU GND
   # Reading should be: 12.0 – 12.5V DC
   
   # After amp connection:
   # Measure at amp's power terminals
   # Should read same voltage (minimal drop)
   ```

**Audio Connection:**
- Plug **3.5mm cable from USB adapter** into amp's **LINE IN jack**
- Test with sample audio: `aplay test.wav`
- Adjust amp's **volume knob** to appropriate level (start at 30%, increase as needed)

**Speaker Connection:**

1. **Speaker Selection**:
   - **Type**: Passive 8Ω horn speaker (typical)
   - **Power handling**: 50-100W RMS (matches amp output)

2. **Speaker Wiring**:
   - Use **2-conductor speaker wire** (16 AWG minimum)
   - **Polarity**: Follow amp output labeling
     - **L+ or Mono+** (usually red terminal) → Horn positive terminal
     - **L− or GND** (usually black terminal) → Horn negative terminal
   - Strip ~6mm from each end, twist strands, secure under speaker terminals

3. **Speaker Mounting**:
   - Mount horn on **rear or top of enclosure**
   - Use **metal U-bracket or straps** for security
   - Position for **maximum acoustic coverage** toward forklift approach
   - If mounted outdoors, add **drainage holes** near bottom
   - Ensure **no obstructions** blocking sound output

4. **Mounting Steps**:
   - Cut opening in enclosure (if needed)
   - Install U-bracket or mounting plate
   - Secure horn with bolts and washers
   - Tighten securely to prevent vibration rattle
   - Route speaker wires with strain relief (avoid pinching)

---

### **Section 4: Pause Button (GPIO17) Wiring**

```
FRONT PANEL
┌──────────────────────────────┐
│                              │
│   [BIG PAUSE BUTTON]         │
│   (white momentary, 40mm)    │
│                              │
└────┬──────────────────────┬──┘
     │ COM (common)         │ NO (normally open)
     │                      │
     │  ┌──────Twisted Pair Wire──────┐
     │  │ (16-22 AWG, ~1 meter)       │
     │  │                              │
     │  │  ┌──────GPIO Header──────┐   │
     │  │  │   Pin 11: GPIO17       │   │
     │  │  │   Pin 14: GND          │   │
     │  │  │                        │   │
     └─→├──┤ Pin 14 (GND) ◄─────────┴──┤ COM
        │  │                        │
        └──┤ Pin 11 (GPIO17) ◄──────┤ NO
           │                        │
           └────────────────────────┘
```

**Wiring Steps:**

1. **Identify Button Terminals**:
   - Get **big momentary push button** (typically white, 40-50mm diameter)
   - Use **multimeter in continuity mode**
   - Find which two terminals are connected when button is pressed
   - Label them: **COM** (common) and **NO** (normally open)

2. **Prepare Wires**:
   - Use **16-22 AWG twisted pair cable** (phone cable type works well)
   - Length: ~1 meter (enough to route neatly from front panel to Pi)
   - Strip **6mm insulation** from each end

3. **Solder to Button**:
   - Heat solder iron to 350°C
   - Apply small amount of rosin-core solder to joint
   - Solder one wire to **COM terminal**
   - Solder other wire to **NO terminal**
   - Let cool ~10 seconds before handling
   - Optional: Use heat shrink tubing to insulate solder joint

4. **Route Wires to GPIO Header**:
   - Pass through **enclosure wall grommet** or cable pass-through
   - Route inside enclosure, avoiding 12V amp wiring
   - Use velcro strips to organize

5. **Connect to GPIO Header**:
   ```
   Looking at Pi from above (USB ports pointing away):
   
   ┌─────────────────────────────────┐
   │ Pin 1  [•] [•] Pin 2            │
   │ Pin 3  [•] [•] Pin 4            │
   │ ...                             │
   │ Pin 11 [GPIO17] [•] Pin 12      │
   │        [•] [•]                  │
   │ Pin 13 [•] [•] Pin 14           │ ← GND
   │        [•] [•]                  │
   └─────────────────────────────────┘
   ```
   
   - **COM wire** → **Pin 14 (GND)**
   - **NO wire** → **Pin 11 (GPIO17)**
   - Options for connection:
     - **Solder directly** to GPIO pins (permanent, clean)
     - **Push into header sockets** with thin wire (removable)
     - Use small female jumper connectors (test-friendly)

6. **Test Connection**:
   - **Power off Pi**
   - Set multimeter to **continuity mode**
   - Probe Pin 11 and Pin 14
   - **Button open**: No continuity (infinite resistance)
   - **Button pressed**: Continuity shows (beep or 0Ω)

7. **Secure Installation**:
   - Use **heat shrink tubing** over solder joints
   - Wrap cable with **velcro straps** to organize
   - Avoid sharp bends (minimum 5cm radius)
   - Label wires with small tags if multiple buttons used

---

### **Section 5: Soft Power Button (J2 Header)**

```
FRONT PANEL
┌────────────────────┐
│ [Small J2 Button]  │ (tiny, ~15mm)
│ (power control)    │
└────┬────────────┬──┘
     │            │
     │ [Wire]     │ (22-24 AWG, ~30cm)
     │            │
     │  ┌─────────────────────┐
     │  │ Pi J2 Header        │
     │  │ (2-pin near audio)  │
     │  │                     │
     └─→├─ J2 Pin 1           │
        │                     │
        ├─ J2 Pin 2 ◄─────────┤
        │                     │
        └─────────────────────┘
```

**Wiring Steps:**

1. **Locate J2 Header**:
   - Found on Raspberry Pi 5 circuit board
   - Usually located near audio jack or on far corner
   - Consult official **Raspberry Pi 5 pinout diagram** if unsure
   - 2-pin header, no polarity required

2. **Button Preparation**:
   - Use **small momentary push button** (normally open, 2 terminals)
   - Solder **22-24 AWG wires** to both terminals
   - Length: ~30 cm (enough to route to front panel)
   - Strip ~5mm insulation

3. **Connect to J2**:
   - **Button terminal 1** → **J2 pin 1**
   - **Button terminal 2** → **J2 pin 2**
   - **Polarity doesn't matter** (momentary button is symmetrical)
   - Options:
     - Solder directly (permanent)
     - Push into header sockets (removable)
     - Use small jumper connectors

4. **Mount on Front Panel**:
   - Drill small hole **8-10 mm diameter**
   - Install button with washer and locknut on inside
   - Leave wires loose (not under strain)
   - Optional: Mount with metal bracket for stability

5. **Test Functionality**:
   - Power on Pi
   - Press J2 button briefly (~0.5 second)
   - Pi firmware should acknowledge press (check system logs)
   - Hold button 5-10 seconds to trigger graceful shutdown

---

### **Section 6: 12V Cooling Fan**

```
12V PSU (same as amplifier rail)
         │
         ├─ RED ─────────────┬────────────────┐
         │                   │                │
         │              ┌────▼────────┐       │
         │              │ 12V Fan     │       │
         │              │ (with filter)       │
         │              │             │       │
         └─ BLACK ──────┼────────────┘       │
            (GND)       │                    │
                        └────────────────────┘
                           (always-on)
```

**Installation Steps:**

1. **Fan Specifications**:
   - Voltage: **12V DC**
   - Current: Typically **0.3-0.5A** (low power)
   - Airflow: Intake or exhaust (choose one direction)

2. **Select Location**:
   - **Top of enclosure** (exhaust air out through filter)
   - **Rear of enclosure** (pull ambient air in)
   - Leave **50mm clearance** around fan for air circulation

3. **Mount Fan**:
   - Use **metal U-bracket** or mounting straps
   - Secure with bolts and washers (prevent vibration)
   - Attach **dust filter** to intake side
   - Ensure airflow direction is consistent with design

4. **Wiring**:
   - **RED (12V+)** → Positive terminal of fan
   - **BLACK (GND)** → Negative terminal of fan
   - Use **18-20 AWG** wire (low current, fine gauge acceptable)
   - Route along 12V amp wiring (same circuit)

5. **Control Options**:
   - **Always-on** (simplest): Wire directly to 12V rail
     - Pro: Simple, no additional control needed
     - Con: Consumes small amount of power continuously
   - **Manual switch** (advanced): Add slide switch in series
     - Pro: Can turn off during low-activity periods
     - Con: Requires user to manage (risk of overheating)

6. **Testing**:
   - Apply 12V to fan terminals
   - Verify fan spins smoothly (no grinding)
   - Check for proper airflow direction
   - Listen for unusual noise (bearing issue if grinding)

---

## B. ESP32-PoE + Sensors (Mezzanine End) – Complete Wiring

### **Section 1: PoE Splitter Installation**

```
PoE Distribution
┌──────────────────────┐
│ PoE Injector (48V)   │
│ (your wall switch)   │
└──────────┬───────────┘
           │
      [Cat6 PoE Cable]
      (from workbench)
           │
      ┌────▼──────────────────┐
      │ PoE Splitter          │
      │ (Passive 54V→5V)      │
      │                       │
      │ Ethernet IN ◄─ Cat6   │
      │ 5V+ OUT ────────────→ │
      │ GND OUT ────────────→ │
      │                       │
      │ DIP Switches:         │
      │ [Set to 5V output]    │
      │                       │
      └────┬────────┬─────────┘
           │        │
        [5V Rail] [GND Rail]
```

**Installation Steps:**

1. **Select Mounting Location**:
   - Wall-mounted bracket near sensors
   - Keep **close to ESP and SR04 modules** (< 1 meter)
   - Ensure **good ventilation** around splitter

2. **Prepare Cat6 Connection**:
   - Route **Cat6 cable from Pi's PoE injector** to mezzanine
   - Use proper **RJ45 connectors** with strain relief boots
   - Secure cable with **cable ties** or clips every meter
   - Protect from physical damage (sharp edges, machinery)

3. **Connect PoE Input**:
   - Plug **Cat6 cable** into splitter's **Ethernet IN port**
   - Ensure connection is fully seated and tight
   - Lock RJ45 connector clip to prevent accidental disconnect

4. **Configure Splitter** (if applicable):
   - Check splitter manual for **DIP switch settings**
   - Most passive splitters: Set for **5V output** (not 12V)
   - Some splitters: Dip switches select 5V vs 12V
   - Verify correct setting before applying power

5. **Test Output Voltage**:
   - Use **multimeter in DC voltage mode**
   - Red probe: Splitter **5V+ terminal**
   - Black probe: Splitter **GND terminal**
   - Reading should be: **4.8 – 5.2V DC**
   - If reading is 0V or 48V: Check DIP switches or splitter documentation

---

### **Section 2: ESP32-PoE GPIO Pinout Reference**

```
ESP32-PoE GPIO Assignment:

GPIO14  ───────→ SR04 #1 TRIG (output, 3.3V)
GPIO16  ───────→ SR04 #2 TRIG (output, 3.3V)
GPIO15  ◄─────── SR04 #1 ECHO (input, 3.3V)
GPIO32  ◄─────── SR04 #2 ECHO (input, 3.3V)
3.3V    ◄─────── PoE Splitter 3.3V+ (power)
GND     ◄─────── PoE Splitter GND (ground)
```

---

### **Section 3: SR04 #1 Sensor Wiring**

```
SENSOR ASSEMBLY

3.3V from PoE Splitter 3.3V Regulator
         │
    ┌────▼──────────────┐
    │ HC-SR04 #1        │
    │                   │
    │ VCC ──────────────┤ (direct 3.3V)
    │ GND ──────────────┤ (common ground)
     │ TRIG ─────────────┤ → ESP GPIO14 (direct 3.3V)
    │ ECHO ─────────────┤ → ESP GPIO15 (direct 3.3V)
    │                   │
    └───────────────────┘
```

**Why No Voltage Divider?**
- SR04 can operate at 3.3V logic levels when powered from 3.3V source
- ECHO output matches supply voltage (3.3V when powered by 3.3V)
- ESP32 GPIO inputs are rated for 3.3V - perfect match
- Direct connection without conversion needed

**Wiring Steps:**

1. **Connect SR04 Power**:
   - **SR04 #1 VCC** → **3.3V rail** from 3.3V regulator
   - **SR04 #1 GND** → **GND rail** from PoE splitter

2. **Connect Trigger**:
   - **SR04 #1 TRIG** → **ESP GPIO14** (direct, no resistor)
   - Use **24 AWG wire**, keep run short (~50 cm max)
   - Single wire connection

3. **Connect Echo**:
   - **SR04 #1 ECHO** → **ESP GPIO15** (direct connection, 3.3V logic)
   - Use **24 AWG wire**, keep run short (~50 cm max)

4. **Verification**:
   - With ESP powered off, measure GPIO15: Should read 0V (or close to it)
   - If stuck at 0V: Check GND connections

---

### **Section 4: SR04 #2 Sensor Wiring (if used)**

**Identical to SR04 #1, but with different GPIO pins:**
- **VCC** → **ESP32 3.3V pin** (shared with SR04 #1)
- **GND** → **ESP32 GND pin** (common ground)
- **TRIG #2** → **ESP GPIO16** (not GPIO14)
- **ECHO #2** → **ESP GPIO32** (not GPIO15)
- **NO voltage dividers** - all 3.3V direct connections
                      │
                     GND
```

---

## Complete Pin Reference Table

| Component | GPIO/Pin | Connection | Notes |
|-----------|----------|------------|-------|
| **Pi GPIO Header Pin 11** | GPIO17 | Pause button NO | Input with pull-up enabled |
| **Pi GPIO Header Pin 14** | GND | Pause button COM | Ground reference |
| **Pi J2 Header** | Pin 1, 2 | Soft power button | No polarity |
| **Pi PoE HAT** | Ethernet | PoE injector | 48V in, 5V out |
| **Pi USB Port** | USB 2 or 3 | Audio USB adapter | 3.5mm to amplifier |
| **12V Amp Power** | — | +12V (RED), GND (BLACK) | 10 AWG, 15A fuse |
| **12V Amp Audio** | — | 3.5mm stereo in | From USB adapter |
| **12V Amp Speaker** | — | L+/L− to Horn | 16 AWG speaker wire |
| **12V Fan** | — | +12V (RED), GND (BLACK) | 18 AWG, always-on or switched |
| **ESP GPIO14** | Output | SR04 #1 TRIG | 3.3V output, direct (no resistors) |
| **ESP GPIO16** | Output | SR04 #2 TRIG | 3.3V output, direct (no resistors) |
| **ESP GPIO15** | Input | SR04 #1 ECHO | 3.3V input, direct (NO voltage divider) |
| **ESP GPIO32** | Input | SR04 #2 ECHO | 3.3V input, direct (NO voltage divider) |
| **SR04 #1 VCC** | — | ESP32 3.3V pin | Shared with SR04 #2 |
| **SR04 #1 GND** | — | ESP32 GND pin | Common ground (shared) |

---

## Quick Troubleshooting Guide

| Problem | Most Likely Cause | Quick Fix |
|---------|-------------------|-----------|
| **No UDP data reaching Pi** | Ethernet not connected | Check static IP: Pi=192.168.10.1, ESP=192.168.10.20, run tcpdump on Pi |
| **Intermittent distance readings** | Loose solder joint | Re-solder all GPIO and divider connections |
| **5V reading on ESP GPIO15/32** | ECHO pin not connected | Verify SR04 ECHO wire firmly connected to GPIO pin |
| **Horn not making sound** | USB audio not detected | Run `aplay -L`, check alsamixer volume |
| **Pause button doesn't work** | GPIO17 not pulled high | Verify wiring: COM→GND, NO→GPIO17 |
| **UPS not charging** | Power connection loose | Check USB-C or 5V header firmly connected |
| **12V amp won't turn on** | Reversed polarity | Verify RED=+12V, BLACK=GND with multimeter |
| **No PoE power at mezzanine** | Splitter misconfigured | Check DIP switches for 5V setting |
| **PoE splitter outputs 0V** | Input disconnected | Ensure Cat6 firmly plugged into Ethernet IN |
| **Amp outputs only hum/noise** | Audio cable near 12V wiring | Reroute 3.5mm away from 12V amp circuit |

---

## System Validation Checklist

- [ ] PoE HAT supplies 5V to Raspberry Pi (multimeter test)
- [ ] PoE Splitter outputs 3.3V at mezzanine (multimeter test)
- [ ] SR04 #1 wiring: TRIG→GPIO14, ECHO→GPIO15 (continuity test)
- [ ] SR04 #2 wiring: TRIG→GPIO16, ECHO→GPIO32 (continuity test)
- [ ] GPIO17 wiring: COM→GND, NO→GPIO17 (continuity test when pressed)
- [ ] J2 power button wired across 2-pin header (Pi firmware recognizes press)
- [ ] 12V amp wiring: RED=+12V, BLACK=GND (polarity verified)
- [ ] 12V amp speaker wiring: L+ to horn+, L− to horn− (no reversed polarity)
- [ ] USB audio adapter detected: `aplay -L` shows USB device
- [ ] Audio cable routed away from 12V circuits (minimize hum)
- [ ] All 5V and GND connections tight (no loose wires)
- [ ] SR04 sensors mounted rigidly (no vibration-induced errors)
- [ ] Cat6 PoE cable secure at both ends (strain relief boots)
- [ ] ESP firmware: WiFi credentials and Pi IP address updated
- [ ] Pi Python app: Config JSON threshold set for your mezzanine height

All components share **common 5V/GND via PoE network**, ensuring safe, integrated operation.
