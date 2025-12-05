/*
 * Forklift Ultrasonic Warning System - Olimex ESP32-PoE Firmware
 * 
 * Function: Read HC-SR04 ultrasonic sensors and transmit distance data
 *           over UDP to Raspberry Pi 5 at the workbench via Ethernet (PoE).
 * 
 * Hardware:
 *   - Olimex ESP32-PoE module (Ethernet-based, powered by PoE)
 *   - 1-2x HC-SR04 ultrasonic distance sensors
 *   - Ethernet connectivity to Raspberry Pi via single Cat6 PoE cable
 * 
 * Sensor Pinout (ESP32-PoE GPIO Assignment):
 *   SR04 #1: TRIG=GPIO14, ECHO=GPIO15 (with voltage divider for 5V→3.3V)
 *   SR04 #2: TRIG=GPIO16, ECHO=GPIO32 (with voltage divider for 5V→3.3V)
 *   GND shared across all devices via PoE
 *   5V from PoE splitter powers ESP and SR04 sensors
 * 
 * Network:
 *   - Connects via Ethernet (built-in to Olimex board)
 *   - PoE provides both power and network connectivity
 *   - Sends UDP packet to Raspberry Pi IP at port 5005
 *   - Packet format: "D1:xxx,D2:xxx\n"
 *   - Measurement cycle: 100ms (every 100ms send new reading)
 * 
 * Power:
 *   - Powered entirely from PoE injector via single Cat6 cable
 *   - Ground shared with sensors and Pi via network common ground
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <AsyncUDP.h>

// ============================================
// CONFIGURATION (Update these for your setup)
// ============================================

// Ethernet (Olimex PoE built-in, no config needed)
// The board auto-initializes with factory defaults

// Raspberry Pi UDP target
const char* udp_target_ip = "192.168.1.100";      // Pi IP address (update to your Pi IP)
const uint16_t udp_target_port = 5005;             // UDP port Pi listens on

// Measurement cycle timing
const unsigned long measurement_interval_ms = 100; // 100ms = 10 readings per second

// Sensor configuration
const uint8_t num_sensors = 2;                     // Number of SR04 sensors (1 or 2)

// ============================================
// GPIO PIN DEFINITIONS
// ============================================

// SR04 Sensor #1
const uint8_t SR04_1_TRIG = 14;   // Trigger pin for sensor 1
const uint8_t SR04_1_ECHO = 15;   // Echo pin for sensor 1 (5V input via divider)

// SR04 Sensor #2
const uint8_t SR04_2_TRIG = 16;   // Trigger pin for sensor 2
const uint8_t SR04_2_ECHO = 32;   // Echo pin for sensor 2 (5V input via divider)

// ============================================
// GLOBAL VARIABLES
// ============================================

AsyncUDP udp;
unsigned long last_measurement_time = 0;
bool eth_connected = false;

// Distance readings in centimeters
float distance_1_cm = 0.0;
float distance_2_cm = 0.0;

// ============================================
// ETH EVENT HANDLERS
// ============================================

void onEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("[ETH] Ethernet started");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("[ETH] Ethernet connected");
      break;
    case 2:
      Serial.println("[ETH] Ethernet got IP");
      Serial.print("[ETH] IP: ");
      Serial.println(ETH.localIP());
      Serial.print("[ETH] Hostname: ");
      Serial.println(ETH.getHostname());
      eth_connected = true;
      break;
    case 3:
      Serial.println("[ETH] Ethernet lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("[ETH] Ethernet disconnected");
      eth_connected = false;
      break;
    default:
      break;
  }
}

// ============================================
// FUNCTION: Read SR04 ultrasonic sensor
// ============================================

float readSR04(uint8_t trig_pin, uint8_t echo_pin) {
  /*
   * SR04 Timing protocol:
   * 1. Send 10µs HIGH pulse on TRIG
   * 2. Wait for ECHO to go HIGH
   * 3. Measure duration ECHO stays HIGH
   * 4. Distance(cm) = duration(µs) / 58
   * 
   * Timeout: If no echo received within 30ms, return -1 (error)
   */
  
  // Ensure trigger is LOW
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  
  // Send 10µs trigger pulse
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);
  
  // Wait for echo pin to go HIGH (with 30ms timeout)
  unsigned long timeout_start = micros();
  while (digitalRead(echo_pin) == LOW) {
    if (micros() - timeout_start > 30000) {
      return -1.0;  // Timeout, no echo received
    }
  }
  
  // Measure pulse duration
  unsigned long echo_start = micros();
  timeout_start = micros();
  while (digitalRead(echo_pin) == HIGH) {
    if (micros() - timeout_start > 30000) {
      return -1.0;  // Timeout while waiting for echo to end
    }
  }
  unsigned long echo_duration = micros() - echo_start;
  
  // Convert pulse duration to distance
  // Speed of sound ≈ 343 m/s = 0.0343 cm/µs
  // Distance = (duration * speed) / 2  (round trip)
  // Distance(cm) = duration(µs) / 58 (simplified)
  float distance = echo_duration / 58.0;
  
  return distance;
}

// ============================================
// FUNCTION: Send UDP packet to Raspberry Pi
// ============================================

void sendUDPPacket(float dist1, float dist2) {
  /*
   * Format: "D1:xxx.x,D2:yyy.y\n"
   * Example: "D1:45.3,D2:67.8\n"
   * -1.0 values indicate sensor error/timeout
   */
  
  if (!eth_connected) {
    return;  // Don't send if not connected
  }
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "D1:%.1f,D2:%.1f\n", dist1, dist2);
  
  // Send via UDP
  udp.writeTo((uint8_t*)buffer, strlen(buffer), 
              IPAddress(192, 168, 1, 100),    // Update to your Pi IP
              udp_target_port);
}

// ============================================
// SETUP
// ============================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== Forklift SR04 UDP System - Olimex ESP32-PoE ===");
  Serial.println("Initializing...");
  
  // Configure GPIO pins for SR04 sensors
  pinMode(SR04_1_TRIG, OUTPUT);
  pinMode(SR04_1_ECHO, INPUT);
  
  if (num_sensors == 2) {
    pinMode(SR04_2_TRIG, OUTPUT);
    pinMode(SR04_2_ECHO, INPUT);
  }
  
  // Ensure trigger pins start LOW
  digitalWrite(SR04_1_TRIG, LOW);
  if (num_sensors == 2) {
    digitalWrite(SR04_2_TRIG, LOW);
  }
  Serial.println("[Sensor] SR04 sensors configured.");
  // Register Ethernet event handlers
  WiFi.onEvent(onEvent);
  
  // Initialize Ethernet (PoE provides power + data)
  Serial.println("[ETH] Starting Ethernet (PoE)...");
  ETH.begin();  // Uses default PoE configuration for Olimex board
  
  // Wait for Ethernet connection (up to 20 seconds)
  int eth_wait = 0;
  while (!eth_connected && eth_wait < 200) {
    delay(100);
    Serial.print(".");
    eth_wait++;
  }
  
  if (eth_connected) {
    Serial.println("\n[ETH] Ethernet connected!");
    Serial.print("[ETH] IP: ");
    Serial.println(ETH.localIP());
  } else {
    Serial.println("\n[ETH] Ethernet connection timeout!");
    Serial.println("[ETH] Retrying in 10 seconds...");
  }
  
  // Initialize UDP listener
  if (udp.listen(5006)) {
    Serial.println("[UDP] Listener started on port 5006");
  }
  
  Serial.print("[UDP] Target Pi IP: ");
  Serial.print(udp_target_ip);
  Serial.print(":");
  Serial.println(udp_target_port);
  
  Serial.println("[System] Ready. Beginning measurements...\n");
  
  last_measurement_time = millis();
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Check if it's time for a new measurement
  unsigned long current_time = millis();
  
  if (current_time - last_measurement_time >= measurement_interval_ms) {
    last_measurement_time = current_time;
    
    // Read sensors
    distance_1_cm = readSR04(SR04_1_TRIG, SR04_1_ECHO);
    
    if (num_sensors == 2) {
      distance_2_cm = readSR04(SR04_2_TRIG, SR04_2_ECHO);
    } else {
      distance_2_cm = 0.0;  // Not used
    }
    
    // Send UDP packet to Pi
    if (eth_connected) {
      sendUDPPacket(distance_1_cm, distance_2_cm);
      
      // Debug output (every 10 cycles = 1 second)
      static uint8_t debug_counter = 0;
      if (++debug_counter >= 10) {
        debug_counter = 0;
        Serial.print("[Sensor] D1: ");
        Serial.print(distance_1_cm);
        Serial.print(" cm | D2: ");
        Serial.print(distance_2_cm);
        Serial.println(" cm");
      }
    } else {
      // Ethernet disconnected, try to reconnect
      static uint8_t recon_counter = 0;
      if (++recon_counter >= 100) {  // Try every 10 seconds
        recon_counter = 0;
        Serial.println("[ETH] Attempting reconnect...");
        ETH.begin();
      }
    }
  }
  
  // Allow other tasks to run
  delay(10);
}

/*
 * NOTES FOR OLIMEX ESP32-PoE:
 * 
 * 1. ETHERNET CONFIGURATION:
 *    - Board: Olimex ESP32-PoE has built-in Ethernet MAC
 *    - PoE: Powered by single Cat6 cable with PoE injector
 *    - No WiFi needed, uses Ethernet directly
 *    - ETH.begin() initializes with factory defaults
 * 
 * 2. GPIO MAPPING (Olimex ESP32-PoE Configuration):
 *    - TRIG pins (GPIO14, GPIO16): Direct outputs to SR04
 *    - ECHO pins (GPIO15, GPIO32): 3.3V inputs (via divider)
 *    - Other pins: Reserved for Ethernet controller
 * 
 * 3. VOLTAGE DIVIDER FOR ECHO PINS:
 *    SR04 ECHO outputs 5V, but ESP32 GPIO accepts max 3.3V.
 *    Use voltage divider:
 *      - 2kΩ resistor from SR04 ECHO to ESP GPIO
 *      - 1kΩ resistor from ESP GPIO to GND
 *    This drops 5V to ~3.3V at input.
 * 
 * 4. POWER DISTRIBUTION:
 *    All components powered from PoE splitter's 5V output:
 *    - Olimex ESP32-PoE: 5V/GND (from PoE)
 *    - SR04 #1: 5V/GND
 *    - SR04 #2: 5V/GND
 *    Common ground via PoE network.
 * 
 * 5. UDP NETWORKING:
 *    - Target: Raspberry Pi at configured IP (e.g., 192.168.1.100)
 *    - Port: 5005 (UDP)
 *    - Format: "D1:45.3,D2:67.8\n"
 *    - Interval: 100ms (10 readings/sec)
 * 
 * 6. TROUBLESHOOTING TG1WDT_SYS_RESET:
 *    If you see watchdog resets:
 *    a) Check Ethernet cable is connected to PoE injector
 *    b) Verify PoE injector is powered
 *    c) Try ETH.begin() with alternative configurations
 *    d) Check serial monitor for Ethernet event messages
 * 
 * 7. DEBUGGING:
 *    Serial output at 115200 baud shows:
 *    - Ethernet connection status
 *    - Distance readings every second
 *    - UDP transmission status
 */
