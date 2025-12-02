MEZZANINE/
│
├── esp32/                      # ESP32 mezzanine firmware
│   ├── include/                # Header files (if any)
│   │   └── README
│   │
│   ├── lib/                    # External libraries (optional)
│   │   └── README
│   │
│   ├── src/                    # Main firmware code
│   │   └── main.cpp            # <-- ESP32 firmware goes here
│   │
│   ├── test/
│   │   └── README
│   │
│   ├── platformio.ini          # <-- PlatformIO project config
│   └── .gitignore
│
├── raspberrypi/
│   ├── app.py                  # <-- Raspberry Pi alert system
│   ├── config.json             # <-- Threshold, GPIO, WAV path, etc.
│   ├── alert.wav               # <-- Horn alert sound
│   ├── service/                # Optional folder for systemd service
│   │   └── forklift.service
│   └── README.md
│
├── layout.md                   # Internal layout only (as you asked)
├── schematics.md               # Wiring schematic only (already created)
└── README.md                   # Project root README
