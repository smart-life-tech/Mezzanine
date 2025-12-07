#!/usr/bin/env python3
"""
Forklift Ultrasonic Warning System - Raspberry Pi Main Application

Function:
  Monitor UDP packets from ESP32 containing fork height distance data.
  When distance falls below threshold, trigger horn warning via USB audio interface.
  Handle pause button (GPIO17) for 2-minute silence.
  Handle soft power button (J2 header) for clean shutdown.

Hardware:
  - Raspberry Pi 5
  - PoE HAT (power + network)
  - Geekworm X1200 UPS with RTC
  - USB → 3.5mm audio adapter → 12V amplifier → horn speaker
  - GPIO17: Pause button (momentary, pulled HIGH internally)
  - J2 header: Soft power button (handled by firmware)
  - 12V fan on 12V rail

Network:
  - Receives UDP packets from ESP32-C6 at mezzanine
  - Packet format: "D1:xxx.x,D2:yyy.y\n"
  - Listens on configured UDP port (default 5005)

Configuration:
  - All settings in config.json
  - Audio file path, threshold distance, pause duration, GPIO pins
"""

import socket
import json
import threading
import time
import os
import signal
import sys
import subprocess
from pathlib import Path
from datetime import datetime, timedelta

try:
    from gpiozero import Button
except ImportError:
    print("Warning: gpiozero not installed. GPIO features disabled.")
    Button = None


# ============================================
# CONFIGURATION LOADING
# ============================================

class Config:
    """Load and manage system configuration."""
    
    def __init__(self, config_path="/home/mark/Mezzanine/raspberrypi/config.json"):
        self.config_path = config_path
        self.data = self._load_config()
    
    def _load_config(self):
        """Load JSON configuration file with defaults."""
        defaults = {
            "udp_listen_port": 5005,
            "distance_threshold_cm": 80,
            "pause_duration_seconds": 120,
            "alert_wav_path": "/home/mark/Mezzanine/raspberrypi/alert.wav",
            "pause_button_gpio": 17,
            "min_interval_between_alerts_sec": 1.0,
        }
        
        try:
            if os.path.exists(self.config_path):
                with open(self.config_path, 'r') as f:
                    loaded = json.load(f)
                    defaults.update(loaded)
                    print(f"[Config] Loaded from {self.config_path}")
            else:
                print(f"[Config] File not found: {self.config_path}")
                print(f"[Config] Using defaults. Create {self.config_path} to customize.")
        except Exception as e:
            print(f"[Config] Error loading config: {e}")
            print(f"[Config] Using defaults.")
        
        return defaults
    
    def get(self, key, default=None):
        """Get config value."""
        return self.data.get(key, default)
    
    def __repr__(self):
        return json.dumps(self.data, indent=2)


# ============================================
# AUDIO ALERT HANDLER
# ============================================

class AudioAlert:
    """Manage horn warning audio playback."""
    
    def __init__(self, wav_path):
        self.wav_path = wav_path
        self.last_alert_time = 0
        self.min_interval = 1.0  # Minimum seconds between alerts
    
    def set_min_interval(self, interval_sec):
        """Set minimum interval between alerts."""
        self.min_interval = interval_sec
    
    def play_alert(self):
        """
        Play horn warning sound.
        Uses aplay (ALSA) to play WAV file through USB audio adapter.
        """
        current_time = time.time()
        
        # Throttle alerts to avoid spam
        if current_time - self.last_alert_time < self.min_interval:
            return False
        
        if not os.path.exists(self.wav_path):
            print(f"[Alert] WAV file not found: {self.wav_path}")
            return False
        
        try:
            # Use aplay to play the audio file
            # By default, aplay uses the first available sound card (USB adapter)
            subprocess.run(
                ["aplay", self.wav_path],
                timeout=10,
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
            self.last_alert_time = current_time
            print(f"[Alert] Horn triggered at {datetime.now().strftime('%H:%M:%S')}")
            return True
        except FileNotFoundError:
            print("[Alert] aplay not found. Install alsa-utils: sudo apt install -y alsa-utils")
            return False
        except Exception as e:
            print(f"[Alert] Error playing audio: {e}")
            return False


# ============================================
# PAUSE BUTTON HANDLER
# ============================================

class PauseButton:
    """
    Handle pause button GPIO input.
    
    Wiring:
      - Button COM → Pi GND
      - Button NO → Pi GPIO17
      - Pull-up enabled in gpiozero
    
    Behavior:
      - Press → silence alerts for duration
      - LED or indicator can show pause status
    """
    
    def __init__(self, gpio_pin=17):
        self.gpio_pin = gpio_pin
        self.pause_until = 0
        self.button = None
        self._init_button()
    
    def _init_button(self):
        """Initialize GPIO button with pull-up."""
        if Button is None:
            print("[Button] gpiozero not available. GPIO disabled.")
            return
        
        try:
            self.button = Button(
                self.gpio_pin,
                pull_up=True,           # Internal pull-up (button grounds GPIO when pressed)
                hold_time=0.1,          # Debounce time
                hold_repeat=False       # Don't repeat on hold
            )
            
            # Register callback for button press
            self.button.when_pressed = self._on_button_press
            print(f"[Button] Pause button initialized on GPIO{self.gpio_pin}")
        except Exception as e:
            print(f"[Button] Error initializing GPIO{self.gpio_pin}: {e}")
            self.button = None
    
    def _on_button_press(self):
        """Called when pause button is pressed."""
        print("[Button] PAUSE button pressed")
    
    def set_pause(self, duration_seconds):
        """
        Pause alerts for specified duration.
        
        Args:
            duration_seconds: How long to pause (typically 120 for 2 minutes)
        """
        self.pause_until = time.time() + duration_seconds
        minutes = duration_seconds / 60
        print(f"[Pause] Alerts paused for {minutes:.1f} minutes until {datetime.now() + timedelta(seconds=duration_seconds)}")
    
    def is_paused(self):
        """Check if we're currently in pause period."""
        return time.time() < self.pause_until
    
    def pause_button_active(self):
        """Check if button is currently physically pressed."""
        if self.button is None:
            return False
        return self.button.is_pressed


# ============================================
# UDP LISTENER
# ============================================

class UDPListener:
    """
    Receive distance data from ESP32-C6 over UDP.
    
    Packet format: "D1:xxx.x,D2:yyy.y\n"
    Example: "D1:45.3,D2:67.8\n"
    -1.0 = sensor error/timeout
    """
    
    def __init__(self, listen_port=5005):
        self.listen_port = listen_port
        self.socket = None
        self.running = False
        self.thread = None
        
        self.latest_distance_1 = 0.0
        self.latest_distance_2 = 0.0
        self.last_update_time = 0
    
    def start(self):
        """Start listening for UDP packets."""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.bind(('0.0.0.0', self.listen_port))
            self.socket.settimeout(1.0)  # 1 second receive timeout
            
            self.running = True
            self.thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.thread.start()
            
            print(f"[UDP] Listening on port {self.listen_port}")
        except Exception as e:
            print(f"[UDP] Error starting listener: {e}")
    
    def stop(self):
        """Stop listening."""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
    
    def _receive_loop(self):
        """Thread: receive UDP packets."""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(1024)
                self.last_update_time = time.time()
                
                # Parse packet: "D1:xxx.x,D2:yyy.y\n"
                message = data.decode('utf-8', errors='ignore').strip()
                self._parse_packet(message)
                
            except socket.timeout:
                pass  # Normal timeout
            except Exception as e:
                if self.running:
                    print(f"[UDP] Receive error: {e}")
    
    def _parse_packet(self, message):
        """Parse distance packet from ESP."""
        try:
            # Expected format: "D1:45.3,D2:67.8"
            parts = message.split(',')
            
            # Parse D1
            if len(parts) >= 1:
                d1_str = parts[0].split(':')
                if len(d1_str) == 2:
                    self.latest_distance_1 = float(d1_str[1])
            
            # Parse D2
            if len(parts) >= 2:
                d2_str = parts[1].split(':')
                if len(d2_str) == 2:
                    self.latest_distance_2 = float(d2_str[1])
            
        except Exception as e:
            # Silently ignore parse errors
            pass
    
    def get_distances(self):
        """Get latest distance readings."""
        return self.latest_distance_1, self.latest_distance_2
    
    def is_data_fresh(self, timeout_sec=2.0):
        """Check if we've received data recently."""
        return (time.time() - self.last_update_time) < timeout_sec


# ============================================
# MAIN ALERT SYSTEM
# ============================================

class ForkliftAlertSystem:
    """
    Main control system for fork height monitoring and alerting.
    """
    
    def __init__(self, config_path="/home/mark/Mezzanine/raspberrypi/config.json"):
        print("=" * 60)
        print("Forklift Ultrasonic Warning System - Raspberry Pi")
        print("=" * 60)
        
        # Load configuration
        self.config = Config(config_path)
        print(f"\n[System] Configuration:")
        print(f"  UDP Port: {self.config.get('udp_listen_port')}")
        print(f"  Threshold: {self.config.get('distance_threshold_cm')} cm")
        print(f"  Pause Duration: {self.config.get('pause_duration_seconds')} sec")
        print(f"  Alert File: {self.config.get('alert_wav_path')}")
        print(f"  Pause GPIO: {self.config.get('pause_button_gpio')}")
        
        # Initialize components
        self.listener = UDPListener(self.config.get('udp_listen_port'))
        self.alert = AudioAlert(self.config.get('alert_wav_path'))
        self.alert.set_min_interval(self.config.get('min_interval_between_alerts_sec'))
        self.pause_button = PauseButton(self.config.get('pause_button_gpio'))
        
        # State
        self.running = True
        self.alert_triggered = False
        self.startup_time = time.time()
        
        # Register signal handlers for clean shutdown
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
        self.alert.play_alert()
    
    def _signal_handler(self, signum, frame):
        """Handle Ctrl-C and kill signals for clean shutdown."""
        print(f"\n[System] Received signal {signum}. Shutting down...")
        self.shutdown()
    
    def start(self):
        """Start the monitoring system."""
        print("\n[System] Starting UDP listener...")
        self.listener.start()
        
        print("[System] System ready. Monitoring for forks...\n")
        time.sleep(1)
        
        # Main monitoring loop
        self._monitor_loop()
    
    def _monitor_loop(self):
        """
        Main loop: monitor distances and trigger alerts.
        """
        last_status_time = 0
        
        while self.running:
            try:
                current_time = time.time()
                
                # Get latest sensor data
                dist1, dist2 = self.listener.get_distances()
                
                # Use minimum distance (either sensor can trigger alert)
                min_distance = dist1
                if dist2 > 0:  # If second sensor is active
                    min_distance = min(dist1, dist2)
                
                # Check pause status and button state
                is_paused = self.pause_button.is_paused()
                button_pressed = self.pause_button.pause_button_active()
                
                # If button is currently pressed, activate pause
                if button_pressed and not is_paused:
                    self.pause_button.set_pause(self.config.get('pause_duration_seconds'))
                    is_paused = True
                
                # Check threshold and trigger alert
                if (not is_paused and 
                    min_distance > 0 and 
                    min_distance < self.config.get('distance_threshold_cm')):
                    
                    if not self.alert_triggered:
                        self.alert_triggered = True
                        self.alert.play_alert()
                else:
                    self.alert_triggered = False
                
                # Print status every 5 seconds
                if current_time - last_status_time >= 5.0:
                    last_status_time = current_time
                    status = "[ALERT]" if self.alert_triggered else "[OK]"
                    pause_status = " [PAUSED]" if is_paused else ""
                    data_status = " [Data OK]" if self.listener.is_data_fresh() else " [Data STALE]"
                    print(f"{status} D1={dist1:6.1f}cm D2={dist2:6.1f}cm Min={min_distance:6.1f}cm{pause_status}{data_status}")
                
                # Small sleep to prevent CPU spin
                time.sleep(0.1)
                
            except Exception as e:
                print(f"[System] Error in monitor loop: {e}")
                time.sleep(1)
    
    def shutdown(self):
        """Clean shutdown."""
        self.running = False
        print("[System] Stopping listener...")
        self.listener.stop()
        print("[System] System stopped.")
        sys.exit(0)


# ============================================
# ENTRY POINT
# ============================================

if __name__ == "__main__":
    try:
        # Default config path; can be overridden
        config_file = "/home/mark/Mezzanine/raspberrypi/config.json"
        if len(sys.argv) > 1:
            config_file = sys.argv[1]
        
        # Create and start the system
        system = ForkliftAlertSystem(config_file)
        system.start()
    
    except KeyboardInterrupt:
        print("\n[System] Interrupted by user.")
        sys.exit(0)
    except Exception as e:
        print(f"\n[System] Fatal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
