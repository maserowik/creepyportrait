#!/usr/bin/env python3
"""
Phase 13 - LED Candle Sidecar
Drives a 30-LED WS2812B strip to simulate a candle flame.
Reads led_state.txt and led_audio.txt written by the main C++ app.

Usage:
    sudo python3 led_candle.py [--loglevel debug|info|warning|error]

Must be run as root (required by rpi-ws281x for DMA access).
Run from the creepyportrait project root directory.
"""

import time
import math
import random
import argparse
import logging
import logging.handlers
import os
import sys

from rpi_ws281x import PixelStrip, Color

# ── Hardware config ───────────────────────────────────────────────────────────
LED_COUNT       = 30
LED_PIN         = 18       # GPIO 18 (PWM0)
LED_FREQ_HZ     = 800000
LED_DMA         = 10
LED_BRIGHTNESS  = 255      # Max hardware brightness — we control brightness in software
LED_INVERT      = False
LED_CHANNEL     = 0

# ── File paths ────────────────────────────────────────────────────────────────
STATE_FILE      = "bin/data/led_state.txt"
AUDIO_FILE      = "bin/data/led_audio.txt"
LOG_FILE        = "bin/data/logs/ledcandle.log"

# ── Candle colour palette (R, G, B) ──────────────────────────────────────────
# Blue tint for base corners
BLUE_TINT       = (20,  10, 180)
# Ember colours (no face / dying)
EMBER_DIP       = (180, 30,   0)
EMBER_BASE      = (255, 60,   0)
# Active colours (face detected)
ACTIVE_BASE     = (255, 90,  10)
ACTIVE_PEAK     = (255, 120, 20)

# ── LED layout ────────────────────────────────────────────────────────────────
# 30 LEDs across 3 sides: left (0-9), top (10-19), right (20-29)
# Bottom is empty. Blue tint applies to LEDs 0-2 (bottom left) and 27-29 (bottom right).
BLUE_LEDS       = {0, 1, 2, 27, 28, 29}
# Position along strip normalised 0.0 (bottom) to 1.0 (top)
def led_position(i):
    if i < 10:    return i / 9.0         # left side: 0=bottom, 9=top
    elif i < 20:  return 1.0             # top: all at maximum
    else:         return 1.0 - (i - 20) / 9.0  # right side: 20=top, 29=bottom

# ── Logging setup ─────────────────────────────────────────────────────────────
def setup_logging(level_str):
    os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
    level = {
        "debug":   logging.DEBUG,
        "info":    logging.INFO,
        "warning": logging.WARNING,
        "error":   logging.ERROR,
    }.get(level_str.lower(), logging.WARNING)

    fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s",
                            datefmt="%Y-%m-%d %H:%M:%S")
    handler = logging.handlers.RotatingFileHandler(
        LOG_FILE, maxBytes=5*1024*1024, backupCount=5)
    handler.setFormatter(fmt)

    log = logging.getLogger("ledcandle")
    log.setLevel(level)
    log.addHandler(handler)

    # Also log to stderr so we can see it in the terminal
    sh = logging.StreamHandler()
    sh.setFormatter(fmt)
    log.addHandler(sh)
    return log

# ── File readers ──────────────────────────────────────────────────────────────
_last_known_state = "ember"

def read_state():
    global _last_known_state
    try:
        with open(STATE_FILE, "r") as f:
            val = f.read().strip()
        if val in ("ember", "active", "fade"):
            _last_known_state = val
            return val
        return _last_known_state
    except Exception:
        return _last_known_state

def read_audio():
    try:
        with open(AUDIO_FILE, "r") as f:
            return float(f.read().strip())
    except Exception:
        return 0.0

# ── Colour helpers ────────────────────────────────────────────────────────────
def lerp_colour(a, b, t):
    t = max(0.0, min(1.0, t))
    return (
        int(a[0] + (b[0] - a[0]) * t),
        int(a[1] + (b[1] - a[1]) * t),
        int(a[2] + (b[2] - a[2]) * t),
    )

def scale_colour(c, s):
    s = max(0.0, min(1.0, s))
    return (int(c[0]*s), int(c[1]*s), int(c[2]*s))

def add_colour(a, b):
    return (min(255, a[0]+b[0]), min(255, a[1]+b[1]), min(255, a[2]+b[2]))

# ── Main candle engine ────────────────────────────────────────────────────────
class CandleEngine:
    def __init__(self, log):
        self.log = log

        # Per-LED phase offsets for organic flicker — each LED drifts independently
        self.phase     = [random.uniform(0, math.pi*2) for _ in range(LED_COUNT)]
        self.speed     = [random.uniform(2.0, 5.0)     for _ in range(LED_COUNT)]
        self.noise     = [random.uniform(0.0, 1.0)     for _ in range(LED_COUNT)]

        # State machine
        self.state          = "ember"
        self.prev_state     = "ember"
        self.fade_start     = 0.0
        self.fade_duration  = 5.0

        # Brightness envelope — smoothly lerped target
        self.brightness     = 0.15    # current overall brightness
        self.target_bright  = 0.15

        # Gust effect
        self.gust_active    = False
        self.gust_start     = 0.0
        self.gust_duration  = 0.0
        self.next_gust      = time.time() + random.uniform(30, 60)

        # Nearly dying effect
        self.dying_active   = False
        self.dying_start    = 0.0
        self.next_dying     = time.time() + random.uniform(60, 120)

        # First light effect
        self.firstlight_active  = False
        self.firstlight_start   = 0.0
        self.firstlight_done    = False

        # Audio gust cooldown
        self.last_audio_gust    = 0.0

    def update_state(self, new_state, now):
        if new_state == self.state:
            return
        self.log.info(f"State change: {self.state} -> {new_state}")
        self.prev_state = self.state

        # First light trigger: ember -> active
        if self.state == "ember" and new_state == "active":
            self.firstlight_active = True
            self.firstlight_start  = now
            self.firstlight_done   = False
            self.log.debug("First light sequence triggered")

        # Fade trigger: active -> ember (via fade)
        if new_state == "fade":
            self.fade_start = now
            self.log.debug("Fade sequence triggered")

        self.state = new_state

    def trigger_gust(self, now, reason="wind"):
        if self.gust_active:
            return
        self.gust_active   = True
        self.gust_start    = now
        self.gust_duration = random.uniform(0.3, 0.7)
        self.log.debug(f"Gust triggered: {reason}")

    def brightness_for_state(self, now):
        """Return target brightness 0.0-1.0 based on current state and effects."""
        if self.state == "ember":
            base = random.uniform(0.08, 0.18)
        elif self.state == "active":
            base = random.uniform(0.55, 0.85)
        elif self.state == "fade":
            t = min(1.0, (now - self.fade_start) / self.fade_duration)
            base = (1.0 - t) * 0.7 + t * 0.12
            base += random.uniform(-0.05, 0.05)
        else:
            base = 0.12

        # First light: flicker struggle then bloom
        if self.firstlight_active:
            elapsed = now - self.firstlight_start
            if elapsed < 0.5:
                # Struggle — rapid random flicker at low brightness
                base = random.uniform(0.05, 0.35)
            else:
                self.firstlight_active = False
                self.firstlight_done   = True
                self.log.debug("First light complete")

        # Nearly dying: dip very low and hold
        if self.dying_active:
            elapsed = now - self.dying_start
            if elapsed < 0.8:
                # Dip fast
                base = base * (1.0 - elapsed / 0.8) * 0.15
            elif elapsed < 2.5:
                # Hold near zero
                base = random.uniform(0.01, 0.04)
            elif elapsed < 5.0:
                # Slow recovery
                base = 0.04 + (elapsed - 2.5) / 2.5 * 0.12
            else:
                self.dying_active = False
                self.log.debug("Nearly dying recovery complete")

        # Gust: sharp dip then recovery
        if self.gust_active:
            elapsed = now - self.gust_start
            if elapsed < self.gust_duration:
                t = elapsed / self.gust_duration
                # Sharp dip then recover
                dip = math.sin(t * math.pi)
                base = base * (1.0 - dip * 0.7)
            else:
                self.gust_active = False

        return max(0.0, min(1.0, base))

    def get_led_colour(self, i, now, bright):
        pos = led_position(i)

        # Per-LED flicker via sine wave + noise
        # Top LEDs flicker faster (heat shimmer)
        spd = self.speed[i] * (1.0 + pos * 0.8)
        self.phase[i] += spd * 0.016   # approx 60fps delta
        flicker = (math.sin(self.phase[i]) + 1.0) / 2.0
        # Add small noise nudge
        self.noise[i] += random.uniform(-0.15, 0.15)
        self.noise[i]  = max(0.0, min(1.0, self.noise[i]))
        flicker = flicker * 0.7 + self.noise[i] * 0.3

        # Choose colour based on state and position
        if self.state == "ember" or self.state == "fade":
            col = lerp_colour(EMBER_DIP, EMBER_BASE, flicker)
        else:
            col = lerp_colour(ACTIVE_BASE, ACTIVE_PEAK, flicker)

        # Blue tint at base corner LEDs — very subtle
        if i in BLUE_LEDS:
            blue_amount = 0.12 if self.state == "active" else 0.06
            blue_tint   = scale_colour(BLUE_TINT, blue_amount)
            # Flicker the blue independently
            blue_flicker = random.uniform(0.4, 1.0)
            blue_tint    = scale_colour(blue_tint, blue_flicker)
            col = add_colour(col, blue_tint)

        # Scale by overall brightness and position (top brighter)
        pos_scale = 0.6 + pos * 0.4
        final_bright = bright * pos_scale
        col = scale_colour(col, final_bright)

        return col

    def update(self, strip, state_str, audio_amp, now):
        # State transitions
        self.update_state(state_str, now)

        # Nearly dying — only during ember, random trigger
        if self.state == "ember" and not self.dying_active:
            if now > self.next_dying:
                self.dying_active = True
                self.dying_start  = now
                self.next_dying   = now + random.uniform(60, 120)
                self.log.debug("Nearly dying effect triggered")

        # Wind gust — only during active, random trigger
        if self.state == "active" and not self.gust_active:
            if now > self.next_gust:
                self.trigger_gust(now, "wind")
                self.next_gust = now + random.uniform(30, 60)

        # Audio reaction — loud sound triggers gust
        if audio_amp > 0.08 and self.state == "active":
            if now - self.last_audio_gust > 3.0:
                self.trigger_gust(now, "audio")
                self.last_audio_gust = now

        # Get overall brightness
        target = self.brightness_for_state(now)
        # Smooth brightness changes
        self.brightness += (target - self.brightness) * 0.12

        # Write each LED
        for i in range(LED_COUNT):
            r, g, b = self.get_led_colour(i, now, self.brightness)
            strip.setPixelColor(i, Color(r, g, b))

        strip.show()

# ── Entry point ───────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="LED Candle Sidecar")
    parser.add_argument("--loglevel", default="warning",
                        choices=["debug", "info", "warning", "error"])
    args = parser.parse_args()

    log = setup_logging(args.loglevel)
    log.info("LED candle sidecar starting")
    log.info(f"GPIO pin: {LED_PIN}, LED count: {LED_COUNT}")
    log.info(f"State file: {STATE_FILE}")
    log.info(f"Audio file: {AUDIO_FILE}")

    # Initialise strip
    strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA,
                       LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
    try:
        strip.begin()
        log.info("LED strip initialised on GPIO 18")
    except Exception as e:
        log.error(f"Failed to initialise LED strip: {e}")
        sys.exit(1)

    engine = CandleEngine(log)
    log.info("Candle engine started")

    try:
        while True:
            now       = time.time()
            state_str = read_state()
            audio_amp = read_audio()
            engine.update(strip, state_str, audio_amp, now)
            time.sleep(0.016)   # ~60fps update rate

    except KeyboardInterrupt:
        log.info("LED candle sidecar stopping")
        # Turn off all LEDs cleanly
        for i in range(LED_COUNT):
            strip.setPixelColor(i, Color(0, 0, 0))
        strip.show()
        log.info("LEDs off. Goodbye.")

if __name__ == "__main__":
    main()
