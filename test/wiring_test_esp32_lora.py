# Wiring self-test for the "ESP32-LORA" board (IoT Connect, rev 1.0, 2024-01-13):
#   ESP32-WROOM-32 + RA-01 (SX1278) + SSD1306 0.96" OLED + NEO-6M GPS header + DHT11
# Pin map taken from Schematic_ESP32-LORA_2026-11-06.png.
#
# Checks: LEDs, OLED (I2C), SX1278 radio (SPI, RST, DIO0, DIO1), buttons (BOOT, BTN1, BTN2),
# GPS UART on GPIO 16/17, and the DHT11 data pin (tries 17 and 16, then scans free header pins).
#
# The radio is only put in receive / channel-activity-detection modes: it never transmits.
#
# Run:   mpremote connect COM10 run test/wiring_test_esp32_lora.py
#  or    copy to the board and `import wiring_test_esp32_lora`

import time
from machine import Pin, I2C, SPI, UART

# ---- Pin map (from the schematic) ----
LED = 2                                   # LED1 "OBLED" on IO2
BUTTONS = {"BOOT": 0, "BTN1": 39, "BTN2": 36}  # all active-low; 36/39 have external 10k pull-ups
OLED_SDA, OLED_SCL, OLED_ADDR = 21, 22, 0x3C   # no reset line on this board
LORA_SCK, LORA_MISO, LORA_MOSI = 5, 19, 27
LORA_CS, LORA_RST = 18, 23
LORA_DIO0, LORA_DIO1, LORA_DIO2 = 26, 33, 32   # DIO3 is tied to GND via 10k, DIO4/5 go to the header
GPS_RX2, GPS_TX2 = 16, 17                 # ESP32 RX2 <- GPS TX, ESP32 TX2 -> GPS RX
DHT_CANDIDATES = [15, 17, 16]  # verified working on GPIO 15 (header "IO15")
# Free output-capable pins on the "MCU Extra GPIO" header (34/35 are input-only, so can't host a DHT)
DHT_SCAN_EXTRA = [4, 13, 14, 25, 12]
BUTTON_WAIT_S = 10

results = []


def report(name, ok, detail=""):
    tag = "PASS" if ok is True else ("FAIL" if ok is False else "INFO")
    results.append((tag, name, detail))
    print("[%s] %-22s %s" % (tag, name, detail))


# ----------------------------------------------------------------- LED
def test_led():
    led = Pin(LED, Pin.OUT)
    for _ in range(3):
        led.on()
        time.sleep_ms(200)
        led.off()
        time.sleep_ms(200)
    report("LED1 (GPIO %d)" % LED, None, "blinked 3x - check it visually (LED2 = power, always on)")


# ----------------------------------------------------------------- OLED
def oled_cmds(i2c, cmds):
    for c in cmds:
        i2c.writeto(OLED_ADDR, bytes([0x00, c]))


def test_oled():
    i2c = I2C(0, scl=Pin(OLED_SCL), sda=Pin(OLED_SDA), freq=400000)
    found = i2c.scan()
    report("I2C scan", bool(found), "devices: %s" % [hex(a) for a in found])
    if OLED_ADDR not in found:
        report("OLED (SSD1306)", False,
               "0x%02X not found - check SDA=%d SCL=%d and 3.3 V" % (OLED_ADDR, OLED_SDA, OLED_SCL))
        return

    # Minimal SSD1306 init + stripe pattern (no driver module needed)
    oled_cmds(i2c, [0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14,
                    0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
                    0xDB, 0x40, 0xA4, 0xA6, 0xAF])
    oled_cmds(i2c, [0x21, 0, 127, 0x22, 0, 7])
    row = bytes(([0xFF] * 8 + [0x00] * 8) * 8)  # 128 columns of 8px stripes
    for _ in range(8):
        i2c.writeto(OLED_ADDR, b"\x40" + row)
    time.sleep_ms(1500)
    oled_cmds(i2c, [0xA7])  # invert
    time.sleep_ms(1500)
    oled_cmds(i2c, [0xA6])
    report("OLED (SSD1306)", True, "responds at 0x%02X - you should see vertical stripes that inverted once" % OLED_ADDR)


# ----------------------------------------------------------------- SX1278
class SX1278:
    def __init__(self):
        self.cs = Pin(LORA_CS, Pin.OUT, value=1)
        self.rst = Pin(LORA_RST, Pin.OUT, value=1)
        self.dio0 = Pin(LORA_DIO0, Pin.IN)
        self.dio1 = Pin(LORA_DIO1, Pin.IN)
        self.dio2 = Pin(LORA_DIO2, Pin.IN)
        self.spi = SPI(1, baudrate=1000000, polarity=0, phase=0,
                       sck=Pin(LORA_SCK), mosi=Pin(LORA_MOSI), miso=Pin(LORA_MISO))

    def read(self, reg):
        self.cs.off()
        self.spi.write(bytes([reg & 0x7F]))
        v = self.spi.read(1)[0]
        self.cs.on()
        return v

    def write(self, reg, val):
        self.cs.off()
        self.spi.write(bytes([reg | 0x80, val]))
        self.cs.on()

    def reset(self):
        self.rst.off()
        time.sleep_ms(2)
        self.rst.on()
        time.sleep_ms(10)

    @staticmethod
    def wait_pin(pin, timeout_ms):
        t0 = time.ticks_ms()
        while time.ticks_diff(time.ticks_ms(), t0) < timeout_ms:
            if pin.value():
                return True
        return False


def test_radio():
    r = SX1278()

    # Reset line: change a register, pulse reset, it must return to its default (RegFrfMsb = 0x6C)
    r.write(0x01, 0x00)  # sleep, FSK
    r.write(0x06, 0x55)
    r.reset()
    version = r.read(0x42)
    if version in (0x00, 0xFF):
        report("SX1278 SPI", False,
               "version reg = 0x%02X (no answer) - check SCK=%d MISO=%d MOSI=%d NSS=%d, 3.3 V"
               % (version, LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS))
        return
    report("SX1278 SPI (read)", version == 0x12, "version reg = 0x%02X (expect 0x12)" % version)
    frf = r.read(0x06)
    report("SX1278 RST (GPIO %d)" % LORA_RST, frf == 0x6C, "RegFrfMsb after reset = 0x%02X (expect 0x6C)" % frf)

    # MOSI: write patterns and read them back
    ok = True
    for pattern in (0x55, 0xAA, 0x6C):
        r.write(0x06, pattern)
        if r.read(0x06) != pattern:
            ok = False
    report("SX1278 SPI (write)", ok, "write/read-back of 0x55/0xAA")

    # LoRa mode
    r.write(0x01, 0x80)  # sleep + LoRa
    time.sleep_ms(10)
    r.write(0x01, 0x81)  # standby
    time.sleep_ms(5)

    # DIO0: channel activity detection -> CadDone on DIO0 (mapping 10). Receive-only.
    r.write(0x40, 0x80)
    r.write(0x12, 0xFF)  # clear IRQs
    r.write(0x01, 0x87)  # CAD
    got = r.wait_pin(r.dio0, 200)
    flags = r.read(0x12)
    report("SX1278 DIO0 (GPIO %d)" % LORA_DIO0, got,
           "CadDone seen (irq=0x%02X)" % flags if got else
           "pin never went high (irq=0x%02X%s)" % (flags, " - radio DID signal CadDone, so the DIO0 wire is the problem" if flags & 0x04 else ""))

    # DIO1: single receive with a short timeout -> RxTimeout on DIO1 (mapping 00)
    r.write(0x01, 0x81)
    time.sleep_ms(5)
    r.write(0x40, 0x00)
    r.write(0x1F, 0x08)  # symbol timeout
    r.write(0x12, 0xFF)
    r.write(0x01, 0x86)  # RX single
    got = r.wait_pin(r.dio1, 500)
    flags = r.read(0x12)
    report("SX1278 DIO1 (GPIO %d)" % LORA_DIO1, got,
           "RxTimeout seen (irq=0x%02X)" % flags if got else
           "pin never went high (irq=0x%02X%s)" % (flags, " - radio DID signal RxTimeout, so the DIO1 wire is the problem" if flags & 0x80 else ""))

    r.write(0x12, 0xFF)
    r.write(0x01, 0x80)  # back to sleep
    report("SX1278 DIO2 (GPIO %d)" % LORA_DIO2, None,
           "not testable without FHSS; idle level = %d (TinyGS doesn't need it for SX1278)" % r.dio2.value())


# ----------------------------------------------------------------- Buttons
def test_buttons():
    pins = {}
    for name, n in BUTTONS.items():
        # GPIO 34-39 have no internal pull-ups; the board has external 10k ones
        p = Pin(n, Pin.IN, Pin.PULL_UP) if n < 34 else Pin(n, Pin.IN)
        if not p.value():
            report("%s (GPIO %d)" % (name, n), False, "reads pressed before you touched it (stuck low / missing pull-up?)")
        else:
            pins[name] = p
    if not pins:
        return

    print(">>> Press each of %s within %d s ..." % (", ".join(pins), BUTTON_WAIT_S))
    seen = set()
    t0 = time.ticks_ms()
    while time.ticks_diff(time.ticks_ms(), t0) < BUTTON_WAIT_S * 1000 and len(seen) < len(pins):
        for name, p in pins.items():
            if name not in seen and not p.value():
                seen.add(name)
                print("    %s pressed" % name)
        time.sleep_ms(10)
    for name, p in pins.items():
        n = BUTTONS[name]
        if name in seen:
            report("%s (GPIO %d)" % (name, n), True, "press detected")
        else:
            report("%s (GPIO %d)" % (name, n), None, "no press seen (skipped or not working)")


# ----------------------------------------------------------------- GPS
def test_gps():
    """If a NEO-6M is fitted, GPIO 16/17 are taken and can't be used for the DHT11."""
    uart = UART(2, baudrate=9600, rx=GPS_RX2, tx=GPS_TX2, timeout=100)
    t0 = time.ticks_ms()
    buf = b""
    while time.ticks_diff(time.ticks_ms(), t0) < 2500:
        d = uart.read()
        if d:
            buf += d
        if b"$G" in buf:
            break
    uart.deinit()
    Pin(GPS_RX2, Pin.IN)
    Pin(GPS_TX2, Pin.IN)
    if b"$G" in buf:
        report("GPS (RX2=%d TX2=%d)" % (GPS_RX2, GPS_TX2), None,
               "NEO-6M detected (NMEA on GPIO %d) - 16/17 are in use by the GPS" % GPS_RX2)
        return True
    report("GPS (RX2=%d TX2=%d)" % (GPS_RX2, GPS_TX2), None, "no NMEA seen - GPS not fitted, so 16/17 are free")
    return False


# ----------------------------------------------------------------- DHT11
def line_state(pin_no):
    """Idle level with internal pull-down: an external pull-up (sensor module/resistor) still reads 1."""
    p = Pin(pin_no, Pin.IN, Pin.PULL_DOWN)
    time.sleep_ms(5)
    highs = sum(p.value() for _ in range(20))
    Pin(pin_no, Pin.IN)
    return highs


def try_dht(pin_no, attempts=3):
    import dht
    sensor = dht.DHT11(Pin(pin_no))
    for _ in range(attempts):
        try:
            sensor.measure()
            return sensor.temperature(), sensor.humidity()
        except OSError:
            time.sleep(1.2)  # DHT11 needs >= 1 s between reads
    return None


def test_dht(gps_present):
    def check(pin_no, verbose):
        highs = line_state(pin_no)
        reading = try_dht(pin_no)
        if reading:
            report("DHT11 (GPIO %d)" % pin_no, True, "%d C, %d %%RH  -> set 'Sensor Data Pin' to %d in TinyGS" % (reading[0], reading[1], pin_no))
            return True
        if verbose:
            hint = "pulled HIGH (something with a pull-up is connected)" if highs == 20 else "LOW/floating (no pull-up - nothing connected?)"
            if gps_present and pin_no == GPS_RX2:
                hint += "; GPS TX drives this pin"
            report("DHT11 (GPIO %d)" % pin_no, False, "no response; idle line " + hint)
        return False

    found = [p for p in DHT_CANDIDATES if check(p, True)]
    if not found:
        print(">>> DHT11 not on %s, scanning free header pins %s ..." % (DHT_CANDIDATES, DHT_SCAN_EXTRA))
        found = [p for p in DHT_SCAN_EXTRA if check(p, False)]
        if not found:
            report("DHT11 scan", False,
                   "not found on any pin - check VCC (3.3 V), GND, and a 4.7-10k pull-up on DATA")
    return found


# ----------------------------------------------------------------- Main
def main():
    print("\n=== TinyGS wiring test: ESP32-LORA (WROOM-32 + RA-01 SX1278) + DHT11 ===\n")
    gps = False
    for name, fn in (("LED", test_led), ("OLED", test_oled), ("Radio", test_radio),
                     ("Buttons", test_buttons), ("GPS", test_gps)):
        try:
            ret = fn()
            if name == "GPS":
                gps = bool(ret)
        except Exception as e:
            report(name, False, "exception: %r" % e)
    try:
        test_dht(gps)
    except Exception as e:
        report("DHT11", False, "exception: %r" % e)

    print("\n=== Summary ===")
    for tag, name, detail in results:
        print("%-4s  %s" % (tag, name))
    fails = sum(1 for r in results if r[0] == "FAIL")
    print("\n%d failure(s)" % fails)


main()
