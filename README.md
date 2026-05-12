# Creepy Portrait

A life-sized 3D skull (or jack-o-lantern) that watches you. Point a webcam at anyone and the skull slowly turns its head to follow them. When nobody is around it wanders on its own, looking around the room. The eyes blink, pulse, and dart around. Play it on a monitor inside a decorative frame for a Halloween display that will genuinely unsettle people.

**You do not need any programming experience to set this up.** Every step is written out exactly as you need to type it.

---

## What It Does

- **Follows your face** — the skull turns left, right, up and down to track whoever is standing in front of the camera
- **Wanders when alone** — after 10 minutes with nobody detected, the skull slowly looks around the room in a figure-8 pattern and drifts around the screen using organic sine wave movement
- **Animated eyes** — bloodshot eyes that pulse, blink with a realistic eyelid sweeping down and back up, and make subtle random movements
- **Jaw movement** — the jaw opens and closes in sync with audio
- **Sound** — plays audio clips when a face is detected
- **Three models** — a skull, an evil jack-o-lantern, and a happy jack-o-lantern

---

## What You Need

| Item | Where to get it | Notes |
|------|----------------|-------|
| Raspberry Pi 4 or Pi 5 | Amazon, eBay, PiShop | Pi 4 with 4GB RAM recommended. Pi 3 and earlier will NOT work. |
| MicroSD card | Amazon | 16GB minimum. Look for "Class 10" or "A1" speed rating. |
| USB webcam | Amazon | Logitech C270 or C920 recommended. |
| HDMI monitor | Any | Standard TV or computer monitor works fine. |
| Micro-HDMI cable or adapter | Amazon | The Pi uses a smaller connector than most TVs. |
| USB keyboard | Any | Just needed for setup. |
| Power supply | Official Pi Foundation store | Must be USB-C. 5V/3A for Pi 4. Do not use a phone charger. |
| Second computer | Your existing PC or Mac | Just used once to prepare the SD card. |
| Internet connection | Your home router | Wired ethernet cable recommended during setup. |

---

## Keyboard Controls

Once the program is running, these keys control it:

| Key | What it does |
|-----|-------------|
| `v` | Show or hide the camera view and face detection box |
| `r` | Make the skull spin automatically (good for testing) |
| `w` | Make the skull start wandering immediately |
| `c` | Snap the skull back to face center |
| `e` | Turn eye animation on or off |
| `j` | Open or close the jaw |
| `s` | Play the audio clip |
| `m` | Switch to the next model (only works when launched with `all`) |
| `Ctrl+C` | Quit the program |

---

## Hardware Compatibility

| Pi Model | Works? | Notes |
|----------|--------|-------|
| Pi 4 (2GB+) | ✅ Yes | Recommended minimum |
| Pi 5 | ✅ Yes | Faster, better tracking |
| Pi 400 | ✅ Yes | Same chip as Pi 4 |
| Pi 3B / 3B+ | ❌ No | Graphics hardware not supported |
| Pi Zero 2W | ❌ No | Graphics hardware not supported, not enough RAM |
| Pi 1 / 2 / Zero | ❌ No | Far too slow |

**If your Pi is not in the Yes column above, stop — it will not work.**

---

## Webcam Compatibility

| Camera | Notes |
|--------|-------|
| Logitech C170 | Works for testing. Limited range and low light performance. |
| Logitech C270 | Good all-rounder. Recommended for general use. |
| Logitech C920 | Best choice. Excellent low light, autofocus, wide field of view. Recommended for permanent wall mount. |

---

## Installation

See **INSTALL_GUIDE_RPI4.md** for the complete step-by-step setup guide.

---

## Running the Program

Open a terminal on the Pi desktop and type:

```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0
```

To run a specific model:

```bash
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackevil
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackhappy
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 all
```

---

## Credits

Original project by Tony DiCola / Adafruit (2013).
Modernized for openFrameworks 0.12+ on Raspberry Pi 4/5 by @maserowik.
