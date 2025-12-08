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
 *   SR04 #1: TRIG=GPIO14, ECHO=GPIO15 (3.3V logic levels)
 *   SR04 #2: TRIG=GPIO16, ECHO=GPIO32 (3.3V logic levels)
 *   GND shared across all devices via PoE
 *   5V from PoE splitter powers ESP and SR04 sensors
 *
 * Network (NO-ROUTER SETUP):
 *   - Static IP configuration (no DHCP server)
 *   - ESP32 #1: 192.168.10.20
 *   - ESP32 #2: 192.168.10.21 (change local_IP below)
 *   - Raspberry Pi: 192.168.10.1 (acts as gateway)
 *   - PoE Switch: Only provides power + Ethernet bridging
 *   - Sends UDP packet to Pi at 192.168.10.1:5005
 *   - Packet format: "D1:xxx,D2:xxx\n"
 *   - Measurement cycle: 100ms (every 100ms send new reading)
 *
 * Power:
 *   - Powered entirely from PoE switch via single Cat6 cable
 *   - Ground shared with sensors and Pi via network common ground
 *
 * IMPORTANT: For second ESP32 board, change line 42 to:
 *   IPAddress local_IP(192, 168, 10, 21);  // ESP32 #2
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <AsyncUDP.h>

// ============================================
// CONFIGURATION (Update these for your setup)
// ============================================

// NO-ROUTER NETWORK CONFIGURATION
// Static IP configuration (required when no DHCP/router present)
IPAddress local_IP(192, 168, 10, 20);  // ESP32 static IP (use .21 for second board)
IPAddress gateway(192, 168, 10, 1);    // Pi acts as gateway
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
IPAddress primaryDNS(192, 168, 10, 1); // Pi as DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);    // No secondary DNS

// WiFi Fallback (if Ethernet fails)
const char *wifi_ssid = "VM0683147";        // WiFi network name
const char *wifi_password = "gbyvzrWjb6gk"; // WiFi password
const bool enable_wifi_fallback = true;     // Set true to enable WiFi fallback

// Raspberry Pi UDP target
const char *udp_target_ip = "192.168.10.1"; // Pi IP address (NO-ROUTER: Pi = 192.168.10.1)
const uint16_t udp_target_port = 5005;      // UDP port Pi listens on

// Measurement cycle timing
const unsigned long measurement_interval_ms = 100; // 100ms = 10 readings per second

// Sensor configuration
const uint8_t num_sensors = 2; // Number of SR04 sensors (1 or 2)

// ============================================
// GPIO PIN DEFINITIONS
// ============================================

// SR04 Sensor #1
const uint8_t SR04_1_TRIG = 14; // Trigger pin for sensor 1
const uint8_t SR04_1_ECHO = 15; // Echo pin for sensor 1 (5V input via divider)

// SR04 Sensor #2
const uint8_t SR04_2_TRIG = 16; // Trigger pin for sensor 2
const uint8_t SR04_2_ECHO = 32; // Echo pin for sensor 2 (5V input via divider)

// ============================================
// GLOBAL VARIABLES
// ============================================

AsyncUDP udp;
unsigned long last_measurement_time = 0;
bool eth_connected = false;
bool wifi_connected = false;
bool network_connected = false; // True if either ETH or WiFi works

// Distance readings in centimeters
float distance_1_cm = 0.0;
float distance_2_cm = 0.0;

// ============================================
// NETWORK EVENT HANDLERS
// ============================================
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_POWER 12
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN // external 50MHz crystal
void setupEthernet()
{
  Serial.println("[ETH] Initializing Olimex ESP32-PoE Ethernet...");

  // Enable PHY power
  pinMode(ETH_PHY_POWER, OUTPUT);
  digitalWrite(ETH_PHY_POWER, 1);
  delay(100);

  // Start Ethernet PHY
  if (!ETH.begin(ETH_PHY_TYPE, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE))
  {
    Serial.println("[ETH] ERROR: ETH.begin() failed!");
  }

  // Apply static IP AFTER Ethernet is alive
  delay(500);
  ETH.config(local_IP, gateway, subnet, gateway, secondaryDNS);

  Serial.println("[ETH] Waiting for link...");
  int retries = 0;
  while (!ETH.linkUp() && retries < 50)
  {
    Serial.print(".");
    delay(200);
    retries++;
  }

  if (ETH.linkUp())
  {
    Serial.println("\n[ETH] ✓ LINK UP!");
    Serial.print("[ETH] IP: ");
    Serial.println(ETH.localIP());
  }
  else
  {
    Serial.println("\n[ETH] ✗ LINK FAILED (Check cable/switch)");
  }
}
void onEvent(WiFiEvent_t event)
{
  switch (event)
  {
  // Ethernet Events
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
    eth_connected = true;
    network_connected = true;
    break;
  case 3:
    Serial.println("[ETH] Ethernet lost IP");
    eth_connected = false;
    network_connected = wifi_connected; // Still connected if WiFi works
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("[ETH] Ethernet disconnected");
    eth_connected = false;
    network_connected = wifi_connected; // Still connected if WiFi works
    break;

  // WiFi Events
  case 10:
    Serial.println("[WiFi] WiFi started");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.println("[WiFi] WiFi connected");
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.println("[WiFi] WiFi got IP");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
    wifi_connected = true;
    network_connected = true;
    break;
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    Serial.println("[WiFi] WiFi lost IP");
    wifi_connected = false;
    network_connected = eth_connected; // Still connected if ETH works
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.println("[WiFi] WiFi disconnected");
    wifi_connected = false;
    network_connected = eth_connected; // Still connected if ETH works
    break;

  default:
    break;
  }
}

// ============================================
// FUNCTION: Read SR04 ultrasonic sensor
// ============================================

float readSR04(uint8_t trig_pin, uint8_t echo_pin)
{
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
  while (digitalRead(echo_pin) == LOW)
  {
    if (micros() - timeout_start > 30000)
    {
      return -1.0; // Timeout, no echo received
    }
  }

  // Measure pulse duration
  unsigned long echo_start = micros();
  timeout_start = micros();
  while (digitalRead(echo_pin) == HIGH)
  {
    if (micros() - timeout_start > 30000)
    {
      return -1.0; // Timeout while waiting for echo to end
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

void sendUDPPacket(float dist1, float dist2)
{
  /*
   * Format: "D1:xxx.x,D2:yyy.y\n"
   * Example: "D1:45.3,D2:67.8\n"
   * -1.0 values indicate sensor error/timeout
   */

  if (!network_connected)
  {
    return; // Don't send if not connected
  }

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "D1:%.1f,D2:%.1f\n", dist1, dist2);

  // Send via UDP - Parse IP string and send
  IPAddress targetIP;
  targetIP.fromString(udp_target_ip);
  udp.writeTo((uint8_t *)buffer, strlen(buffer),
              targetIP,
              udp_target_port);
}

// ============================================
// SETUP
// ============================================

void setup()
{
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== Forklift SR04 UDP System - Olimex ESP32-PoE ===");
  Serial.println("Initializing...");

  // Configure GPIO pins for SR04 sensors
  pinMode(SR04_1_TRIG, OUTPUT);
  pinMode(SR04_1_ECHO, INPUT);

  if (num_sensors == 2)
  {
    pinMode(SR04_2_TRIG, OUTPUT);
    pinMode(SR04_2_ECHO, INPUT);
  }

  // Ensure trigger pins start LOW
  digitalWrite(SR04_1_TRIG, LOW);
  if (num_sensors == 2)
  {
    digitalWrite(SR04_2_TRIG, LOW);
  }
  Serial.println("[Sensor] SR04 sensors configured.");
  // Register Ethernet event handlers
  WiFi.onEvent(onEvent);

  // Initialize Ethernet with STATIC IP (NO-ROUTER setup)
  Serial.println("[ETH] Starting Ethernet (PoE) with STATIC IP...");
  Serial.print("[ETH] Static IP: ");
  Serial.println(local_IP);
  Serial.print("[ETH] Gateway (Pi): ");
  Serial.println(gateway);

  // Configure static IP BEFORE starting Ethernet
  if (!ETH.begin(ETH_PHY_TYPE, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE))
  {
    Serial.println("[ETH] ERROR: ETH.begin() failed!");
  }

  // Apply static IP AFTER Ethernet is alive
  delay(500);
  ETH.config(local_IP, gateway, subnet, gateway, secondaryDNS);

  // Wait for Ethernet link and connection (up to 15 seconds)
  Serial.println("[ETH] Waiting for link...");
  int eth_wait = 0;
  while (!eth_connected && eth_wait < 150)
  {
    delay(200);
    if (eth_wait % 10 == 0)
      Serial.print(".");
    eth_wait++;
    if eth_wait
      > 50
      {
        Serial.println("\n[ETH] Still waiting for link...");
        break;
      }
  }

  if (eth_connected)
  {
    Serial.println("\n[ETH] ✓ Ethernet connected!");
    Serial.print("[ETH] IP: ");
    Serial.println(ETH.localIP());
    Serial.print("[ETH] Gateway: ");
    Serial.println(ETH.gatewayIP());
    Serial.print("[ETH] Subnet: ");
    Serial.println(ETH.subnetMask());
    Serial.print("[ETH] MAC: ");
    Serial.println(ETH.macAddress());
    network_connected = true;
  }
  else
  {
    Serial.println("\n[ETH] Ethernet connection failed!");

    // Try WiFi fallback if enabled
    if (enable_wifi_fallback)
    {
      Serial.println("[WiFi] Attempting WiFi fallback...");
      Serial.print("[WiFi] Connecting to: ");
      Serial.println(wifi_ssid);

      WiFi.mode(WIFI_STA);
      WiFi.begin(wifi_ssid, wifi_password);

      int wifi_wait = 0;
      while (!wifi_connected && wifi_wait < 200)
      {
        delay(500);
        Serial.print(".");
        wifi_wait++;
      }

      if (wifi_connected)
      {
        Serial.println("\n[WiFi] WiFi connected!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        network_connected = true;
      }
      else
      {
        Serial.println("\n[WiFi] WiFi connection failed!");
        Serial.println("[ERROR] No network connection available!");
      }
    }
  }

  // Initialize UDP listener
  if (udp.listen(5006))
  {
    Serial.println("[UDP] Listener started on port 5006");
  }
  else
  {
    Serial.println("[UDP] Failed to start listener!");
  }

  Serial.print("[UDP] Target Pi IP: ");
  Serial.print(udp_target_ip);
  Serial.print(":");
  Serial.println(udp_target_port);

  if (network_connected)
  {
    Serial.println("[System] ✓ Network connected - Ready!");
  }
  else
  {
    Serial.println("[System] ✗ NO NETWORK - Will retry...");
  }

  Serial.println("[System] Beginning measurements...\n");

  last_measurement_time = millis();
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
  // Check if it's time for a new measurement
  unsigned long current_time = millis();

  if (current_time - last_measurement_time >= measurement_interval_ms)
  {
    last_measurement_time = current_time;

    // Read sensors
    distance_1_cm = readSR04(SR04_1_TRIG, SR04_1_ECHO);

    if (num_sensors == 2)
    {
      distance_2_cm = readSR04(SR04_2_TRIG, SR04_2_ECHO);
    }
    else
    {
      distance_2_cm = 0.0; // Not used
    }

    // Send UDP packet to Pi
    if (network_connected)
    {
      sendUDPPacket(distance_1_cm, distance_2_cm);

      // Debug output (every 10 cycles = 1 second)
      static uint8_t debug_counter = 0;
      if (++debug_counter >= 10)
      {
        debug_counter = 0;
        String connection_type = eth_connected ? "ETH" : "WiFi";
        Serial.print("[" + connection_type + "] D1: ");
        Serial.print(distance_1_cm);
        Serial.print(" cm | D2: ");
        Serial.print(distance_2_cm);
        Serial.println(" cm");
      }
    }
    else
    {
      // No network connection - show error message periodically
      static uint8_t recon_counter = 0;
      if (++recon_counter >= 100)
      { // Print error every 10 seconds
        recon_counter = 0;
        Serial.println("[ERROR] No network connection!");
        Serial.println("[ERROR] Check Ethernet cable and PoE power");
        Serial.println("[ERROR] Verify Pi is at 192.168.10.1");
        Serial.print("[Sensor] D1: ");
        Serial.print(distance_1_cm);
        Serial.print(" cm | D2: ");
        Serial.print(distance_2_cm);
        Serial.println(" cm (NOT SENT)");
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
 *    - ECHO pins (GPIO15, GPIO32): Direct inputs from SR04
 *    - All logic levels at 3.3V (no voltage conversion needed)
 *    - Other pins: Reserved for Ethernet controller
 *
 * 3. POWER DISTRIBUTION:
 *    All components powered from PoE splitter's 3.3V output:
 *    - Olimex ESP32-PoE: 3.3V/GND (from PoE splitter)
 *    - SR04 #1: 3.3V/GND
 *    - SR04 #2: 3.3V/GND
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
