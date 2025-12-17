#!/usr/bin/env python3
"""
TFT LCD Display Test Script
Supports ST7735, ST7789, and ILI9341 SPI TFT displays

Hardware Connection (SPI):
  - VCC  → 3.3V or 5V (check your display spec)
  - GND  → GND
  - SCL  → GPIO 11 (SPI0 SCLK)
  - SDA  → GPIO 10 (SPI0 MOSI)
  - SDO  → GPIO 9  (SPI0 MISO, often unused on displays)
  - CS   → GPIO 8  (SPI0 CE0)
  - DC   → GPIO 25 (Data/Command)
  - RST  → GPIO 24 (Reset)
  - BLK  → 3.3V or 5V (Backlight, direct connection)

Installation:
  sudo pip3 install adafruit-circuitpython-rgb-display pillow
  sudo apt-get install python3-pil fonts-dejavu

Display Types (uncomment the one you have):
  - 1.44" 128x128 → ST7735
  - 1.8"  128x160 → ST7735
  - 2.0"  240x320 → ILI9341
  - 2.4"  240x320 → ILI9341
  - 2.8"  240x320 → ILI9341
  - 3.2"  240x320 → ILI9341
"""

import time
import board
import digitalio
from PIL import Image, ImageDraw, ImageFont

# Try to import display libraries
try:
    from adafruit_rgb_display import st7735  # For 1.44", 1.8" displays
    from adafruit_rgb_display import st7789  # For some 2.0" displays
    from adafruit_rgb_display import ili9341 # For 2.4", 2.8", 3.2" displays
    DISPLAY_AVAILABLE = True
except ImportError:
    print("ERROR: Display libraries not installed")
    print("Run: sudo pip3 install adafruit-circuitpython-rgb-display pillow")
    DISPLAY_AVAILABLE = False
    exit(1)


# ==============================================
# CONFIGURATION - EDIT THIS SECTION
# ==============================================

# Pin Configuration (BCM numbering)
CS_PIN = board.CE0                               # GPIO 8 (SPI0 CE0) - pass as board pin, not DigitalInOut
DC_PIN = digitalio.DigitalInOut(board.D25)      # GPIO 25 (Data/Command)
RESET_PIN = digitalio.DigitalInOut(board.D24)   # GPIO 24 (Reset)
BACKLIGHT_PIN = None  # Connect LED directly to 3.3V or 5V

# Display Selection - UNCOMMENT THE ONE YOU HAVE
DISPLAY_TYPE = "ST7735_128x128"   # 1.44" square display
# DISPLAY_TYPE = "ST7735_128x160"   # 1.8" rectangular display
# DISPLAY_TYPE = "ILI9341_240x320"  # 2.4", 2.8", 3.2" displays
# DISPLAY_TYPE = "ST7789_240x240"   # 2.0" square display

# Display rotation (0, 90, 180, 270)
ROTATION = 0

# ==============================================
# DISPLAY INITIALIZATION
# ==============================================

def init_backlight(pin_num):
    """Initialize backlight control (optional)."""
    try:
        import RPi.GPIO as GPIO
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)
        GPIO.setup(pin_num, GPIO.OUT)
        GPIO.output(pin_num, GPIO.HIGH)  # Turn on backlight
        print(f"[Display] Backlight enabled on GPIO{pin_num}")
    except Exception as e:
        print(f"[Display] Backlight control failed: {e}")


def init_display():
    """Initialize the TFT display based on type."""
    print(f"[Display] Initializing {DISPLAY_TYPE}...")
    
    # Create SPI bus
    spi = board.SPI()
    
    # Initialize display based on type
    if DISPLAY_TYPE == "ST7735_128x128":
        # 1.44" 128x128 display
        disp = st7735.ST7735R(
            spi, cs=CS_PIN, dc=DC_PIN, rst=RESET_PIN,
            width=128, height=128,
            x_offset=2, y_offset=1,  # May need adjustment
            rotation=ROTATION,
            baudrate=24000000
        )
        
    elif DISPLAY_TYPE == "ST7735_128x160":
        # 1.8" 128x160 display
        disp = st7735.ST7735R(
            spi, cs=CS_PIN, dc=DC_PIN, rst=RESET_PIN,
            width=128, height=160,
            x_offset=0, y_offset=0,
            rotation=ROTATION,
            baudrate=24000000
        )
        
    elif DISPLAY_TYPE == "ILI9341_240x320":
        # 2.4", 2.8", 3.2" 240x320 display
        disp = ili9341.ILI9341(
            spi, cs=CS_PIN, dc=DC_PIN, rst=RESET_PIN,
            width=240, height=320,
            rotation=ROTATION,
            baudrate=24000000
        )
        
    elif DISPLAY_TYPE == "ST7789_240x240":
        # 2.0" 240x240 display
        disp = st7789.ST7789(
            spi, cs=CS_PIN, dc=DC_PIN, rst=RESET_PIN,
            width=240, height=240,
            x_offset=0, y_offset=80,
            rotation=ROTATION,
            baudrate=24000000
        )
        
    else:
        print(f"ERROR: Unknown display type: {DISPLAY_TYPE}")
        print("Edit DISPLAY_TYPE in the configuration section")
        return None
    
    print(f"[Display] Initialized: {disp.width}x{disp.height}")
    return disp


# ==============================================
# TEST FUNCTIONS
# ==============================================

def test_colors(disp):
    """Test basic color display."""
    print("[Test] Color test...")
    
    colors = [
        (255, 0, 0),    # Red
        (0, 255, 0),    # Green
        (0, 0, 255),    # Blue
        (255, 255, 0),  # Yellow
        (255, 0, 255),  # Magenta
        (0, 255, 255),  # Cyan
        (255, 255, 255),# White
        (0, 0, 0)       # Black
    ]
    
    color_names = ["Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "White", "Black"]
    
    for color, name in zip(colors, color_names):
        print(f"  Showing {name}...")
        image = Image.new("RGB", (disp.width, disp.height), color)
        disp.image(image)
        time.sleep(1)


def test_text(disp):
    """Test text rendering."""
    print("[Test] Text test...")
    
    # Create blank image
    image = Image.new("RGB", (disp.width, disp.height), (0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Load fonts (try different sizes)
    try:
        font_large = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
        font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    except:
        font_large = ImageFont.load_default()
        font_small = ImageFont.load_default()
    
    # Draw text
    draw.text((10, 10), "TFT Display", font=font_large, fill=(255, 255, 255))
    draw.text((10, 40), "Test Screen", font=font_small, fill=(0, 255, 0))
    draw.text((10, 60), f"{disp.width}x{disp.height}", font=font_small, fill=(255, 255, 0))
    
    # Draw some shapes
    draw.rectangle((10, 80, 50, 120), outline=(255, 0, 0), fill=(128, 0, 0))
    draw.ellipse((60, 80, 100, 120), outline=(0, 255, 0), fill=(0, 128, 0))
    draw.line((10, 130, disp.width-10, 130), fill=(0, 0, 255), width=2)
    
    disp.image(image)
    time.sleep(3)


def test_sensor_display(disp):
    """Simulate sensor reading display (like forklift system)."""
    print("[Test] Sensor display simulation...")
    
    try:
        font_title = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16)
        font_value = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32)
        font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    except:
        font_title = font_value = font_small = ImageFont.load_default()
    
    # Simulate 10 readings
    for i in range(10):
        # Fake sensor values
        distance1 = 100 - (i * 5)
        distance2 = 95 - (i * 4)
        
        # Create image
        image = Image.new("RGB", (disp.width, disp.height), (0, 0, 0))
        draw = ImageDraw.Draw(image)
        
        # Title
        draw.text((10, 5), "Fork Height", font=font_title, fill=(255, 255, 255))
        
        # Sensor 1
        draw.text((10, 30), "Sensor 1:", font=font_small, fill=(0, 255, 255))
        color1 = (0, 255, 0) if distance1 > 80 else (255, 0, 0)
        draw.text((10, 45), f"{distance1} cm", font=font_value, fill=color1)
        
        # Sensor 2
        draw.text((10, 85), "Sensor 2:", font=font_small, fill=(0, 255, 255))
        color2 = (0, 255, 0) if distance2 > 80 else (255, 0, 0)
        draw.text((10, 100), f"{distance2} cm", font=font_value, fill=color2)
        
        # Status
        if distance1 < 80 or distance2 < 80:
            draw.rectangle((0, disp.height-25, disp.width, disp.height), fill=(255, 0, 0))
            draw.text((10, disp.height-20), "⚠ WARNING ⚠", font=font_small, fill=(255, 255, 255))
        else:
            draw.rectangle((0, disp.height-25, disp.width, disp.height), fill=(0, 128, 0))
            draw.text((10, disp.height-20), "✓ OK", font=font_small, fill=(255, 255, 255))
        
        disp.image(image)
        time.sleep(0.5)


def test_network_info(disp):
    """Display network information (useful for debugging)."""
    print("[Test] Network info display...")
    
    import socket
    
    try:
        font_title = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14)
        font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 10)
    except:
        font_title = font_small = ImageFont.load_default()
    
    # Get IP address
    try:
        hostname = socket.gethostname()
        ip_address = socket.gethostbyname(hostname)
    except:
        hostname = "Unknown"
        ip_address = "No Network"
    
    # Create image
    image = Image.new("RGB", (disp.width, disp.height), (0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Display info
    draw.text((10, 10), "Network Info", font=font_title, fill=(255, 255, 255))
    draw.text((10, 35), f"Host: {hostname}", font=font_small, fill=(0, 255, 0))
    draw.text((10, 50), f"IP: {ip_address}", font=font_small, fill=(0, 255, 255))
    draw.text((10, 65), f"Display: {disp.width}x{disp.height}", font=font_small, fill=(255, 255, 0))
    
    # Current time
    current_time = time.strftime("%H:%M:%S")
    draw.text((10, 85), f"Time: {current_time}", font=font_small, fill=(255, 255, 255))
    
    disp.image(image)
    time.sleep(5)


def test_pattern(disp):
    """Display test pattern."""
    print("[Test] Test pattern...")
    
    image = Image.new("RGB", (disp.width, disp.height), (0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw grid pattern
    for x in range(0, disp.width, 20):
        draw.line((x, 0, x, disp.height), fill=(50, 50, 50))
    for y in range(0, disp.height, 20):
        draw.line((0, y, disp.width, y), fill=(50, 50, 50))
    
    # Draw corners
    draw.rectangle((0, 0, 10, 10), fill=(255, 0, 0))
    draw.rectangle((disp.width-10, 0, disp.width, 10), fill=(0, 255, 0))
    draw.rectangle((0, disp.height-10, 10, disp.height), fill=(0, 0, 255))
    draw.rectangle((disp.width-10, disp.height-10, disp.width, disp.height), fill=(255, 255, 0))
    
    # Center circle
    center_x = disp.width // 2
    center_y = disp.height // 2
    draw.ellipse((center_x-20, center_y-20, center_x+20, center_y+20), 
                 outline=(255, 255, 255), fill=(128, 128, 128))
    
    disp.image(image)
    time.sleep(3)


# ==============================================
# MAIN TEST SEQUENCE
# ==============================================

def main():
    """Run all display tests."""
    print("=" * 60)
    print("TFT LCD Display Test")
    print("=" * 60)
    
    # Initialize backlight
    if BACKLIGHT_PIN:
        init_backlight(BACKLIGHT_PIN)
    
    # Initialize display
    disp = init_display()
    if disp is None:
        return
    
    print("\nRunning test sequence...")
    print("Press Ctrl+C to stop\n")
    
    try:
        # Run tests
        test_colors(disp)
        test_pattern(disp)
        test_text(disp)
        test_network_info(disp)
        test_sensor_display(disp)
        
        print("\n✓ All tests completed!")
        print("Display is working correctly.")
        
        # Keep final screen
        time.sleep(5)
        
    except KeyboardInterrupt:
        print("\n[Test] Interrupted by user")
    except Exception as e:
        print(f"\n[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # Clear display
        print("[Display] Clearing screen...")
        image = Image.new("RGB", (disp.width, disp.height), (0, 0, 0))
        disp.image(image)
        print("[Display] Test complete")


if __name__ == "__main__":
    main()
