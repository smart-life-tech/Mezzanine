/*
 * Forklift Ultrasonic Warning System - ESP32-C6 Firmware
 * 
 * Function: Read HC-SR04 ultrasonic sensors and transmit distance data
 *           over UDP to Raspberry Pi 5 at the workbench.
 * 
 * Hardware:
 *   - ESP32-C6 module (powered by PoE splitter at mezzanine)
 *   - 1-2x HC-SR04 ultrasonic distance sensors
 *   - WiFi/Ethernet connectivity to Raspberry Pi
 * 
 * Sensor Pinout (ESP32-C6):
 *   SR04 #1: TRIG=GPIO2,  ECHO=GPIO5 (with voltage divider for 5V→3.3V)
 *   SR04 #2: TRIG=GPIO4,  ECHO=GPIO6 (with voltage divider for 5V→3.3V)
 *   GND shared across all devices via PoE splitter
 *   5V from PoE splitter powers ESP and SR04 sensors
 * 
 * Network:
 *   - Connects to WiFi SSID with configurable credentials
 *   - Sends UDP packet to Raspberry Pi IP at port 5005
 *   - Packet format: "D1:xxx,D2:xxx\n"  (distance in cm, comma separated)
 *   - Measurement cycle: 100ms (every 100ms send new reading)
 * 
 * Power:
 *   - Powered entirely from PoE splitter's 5V output
 *   - Ground shared with sensors and Pi via network common ground
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

// ============================================
// CONFIGURATION (Update these for your setup)
// ============================================

// WiFi credentials
const char* ssid = "YOUR_SSID";                    // WiFi network name
const char* password = "YOUR_PASSWORD";            // WiFi password

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
const uint8_t SR04_1_TRIG = 2;   // Trigger pin for sensor 1
const uint8_t SR04_1_ECHO = 5;   // Echo pin for sensor 1 (5V input via divider)

// SR04 Sensor #2
const uint8_t SR04_2_TRIG = 4;   // Trigger pin for sensor 2
const uint8_t SR04_2_ECHO = 6;   // Echo pin for sensor 2 (5V input via divider)

// ============================================
// GLOBAL VARIABLES
// ============================================

AsyncUDP udp;
unsigned long last_measurement_time = 0;

// Distance readings in centimeters
float distance_1_cm = 0.0;
float distance_2_cm = 0.0;

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
  
  Serial.println("\n\n=== Forklift SR04 UDP System - ESP32-C6 ===");
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
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 30) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Retrying in 10 seconds...");
  }
  
  // Initialize UDP
  if (udp.listen(5006)) {
    Serial.println("UDP listener started on port 5006");
  }
  
  Serial.print("Target Pi IP: ");
  Serial.print(udp_target_ip);
  Serial.print(":");
  Serial.println(udp_target_port);
  
  Serial.println("System ready. Beginning measurements...\n");
  
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
    if (WiFi.status() == WL_CONNECTED) {
      sendUDPPacket(distance_1_cm, distance_2_cm);
      
      // Debug output (every 10 cycles = 1 second)
      static uint8_t debug_counter = 0;
      if (++debug_counter >= 10) {
        debug_counter = 0;
        Serial.print("D1: ");
        Serial.print(distance_1_cm);
        Serial.print(" cm | D2: ");
        Serial.print(distance_2_cm);
        Serial.println(" cm");
      }
    } else {
      // WiFi disconnected, attempt reconnect
      Serial.println("WiFi disconnected. Attempting reconnect...");
      WiFi.reconnect();
    }
  }
  
  // Allow other tasks to run
  delay(10);
}

/*
 * NOTES:
 * 
 * 1. VOLTAGE DIVIDER FOR ECHO PINS:
 *    SR04 ECHO outputs 5V, but ESP32-C6 GPIO accepts max 3.3V.
 *    Use a voltage divider:
 *      - 2kΩ resistor from SR04 ECHO to ESP GPIO (e.g., GPIO5)
 *      - 1kΩ resistor from ESP GPIO to GND
 *    This drops 5V to ~3.3V at the input.
 * 
 * 2. POWER DISTRIBUTION:
 *    All components powered from PoE splitter's 5V output:
 *    - ESP32: 5V/GND
 *    - SR04 #1: 5V/GND
 *    - SR04 #2: 5V/GND (if used)
 *    Common ground via PoE network.
 * 
 * 3. WiFi SETUP:
 *    Update ssid and password to match your network.
 *    Update udp_target_ip to your Raspberry Pi's IP address.
 * 
 * 4. MEASUREMENT ACCURACY:
 *    - 100ms measurement cycle ensures smooth data flow
 *    - 10 readings/second is sufficient for forklift tracking
 *    - Timeout of 30ms handles no-echo or out-of-range (>5m)
 * 
 * 5. DEBUGGING:
 *    Serial output at 115200 baud for monitoring via USB.
 *    Set `build_flags = -DCORE_DEBUG_LEVEL=2` in platformio.ini
 */
