# Creepy Portrait

A face-tracking haunted portrait for Raspberry Pi 4/5. Point a USB webcam at anyone and a 3D skull (or jack-o-lantern) slowly turns to follow them — fullscreen, on a red curtain background, booting automatically on startup.

Built on [openFrameworks 0.12+](https://openframeworks.cc). Originally created by [Tony DiCola / Adafruit (2013)](https://learn.adafruit.com/creepy-face-tracking-portrait) and modernized for current hardware and OF versions.

---

## Hardware Requirements

| Item | Notes |
|------|-------|
| Raspberry Pi 4 (2GB+) or Pi 5 | Pi 3 and earlier will NOT work — wrong GPU |
| USB webcam | Logitech C270 or C920 recommended |
| MicroSD card | 16GB minimum, Class 10 / A1 |
| HDMI monitor | Pi 4/5 use micro-HDMI |
| USB-C power supply | 5V/3A for Pi 4 · 5V/5A for Pi 5 |

> **Pi camera module note:** The `pi` argument in the usage output requires the legacy MMAL/OMX camera stack removed in Raspberry Pi OS Bullseye (2021). On modern Pi OS always use a USB webcam with a numeric device ID.

---

## Quick Start

Clone directly onto your Pi after completing the openFrameworks install:

```bash
cd ~/openFrameworks/apps/myApps
git clone https://github.com/maserowik/creepyportrait creepyportrait
cd creepyportrait
make -j$(nproc) Release
./bin/creepyportrait 0
```

See [INSTALL_GUIDE_RPI4.md](INSTALL_GUIDE_RPI4.md) for the complete step-by-step setup guide including openFrameworks installation.

---

## Models

| Argument | Model |
|----------|-------|
| *(none)* | 💀 Skull (default) |
| `skull` | 💀 Skull |
| `jackevil` | 😈 Evil jack-o-lantern |
| `jackhappy` | 🎃 Happy jack-o-lantern |
| `all` | All three — press `m` to cycle |

```bash
./bin/creepyportrait 0              # skull
./bin/creepyportrait 0 jackevil    # evil jack
./bin/creepyportrait 0 jackhappy   # happy jack
./bin/creepyportrait 0 all         # all three, press m to switch
```

---

## Keyboard Controls

| Key | Action |
|-----|--------|
| `v` | Toggle camera/face-detection overlay |
| `r` | Toggle auto-rotation mode |
| `m` | Cycle to next model (requires `all`) |
| `Ctrl+C` | Quit |

---

## Auto-Start on Boot

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/creepyportrait.desktop
```

Paste:

```ini
[Desktop Entry]
Type=Application
Name=Creepy Portrait
Exec=/bin/bash -c 'cd /home/pi/openFrameworks/apps/myApps/creepyportrait && ./bin/creepyportrait 0'
X-GNOME-Autostart-enabled=true
```

Then enable desktop autologin:

```bash
sudo raspi-config
# System Options → Boot / Auto Login → Desktop Autologin
sudo reboot
```

---

## Tuning

Edit `src/main.cpp` inside the `#ifdef TARGET_RASPBERRY_PI` block, then rebuild with `make -j$(nproc) Release`.

| Setting | Default | Notes |
|---------|---------|-------|
| `faceUpdateDelay` | `2.0` | Lower to `0.5`–`1.0` for snappier tracking on Pi 4 |
| `videoFOV` | `60` | Match your webcam — wide-angle needs `75`–`90` |
| `faceDepth` | `10.0` | Increase if skull barely moves; decrease if it over-rotates |
| `noFaceResetSeconds` | `6.0` | Time before skull resets to center when face is lost |
| `videoWidth/Height` | `160x120` | Increase to `320x240` for better detection (uses more CPU) |

---

## Troubleshooting

**Skull opens and immediately closes**
Always run from the project directory:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
./bin/creepyportrait 0
```

**No webcam detected**
```bash
ls /dev/video*   # should show /dev/video0
lsusb            # webcam should appear in list
```

**Face not tracking** — press `v` to show the camera overlay. If you can see yourself but no green box appears, improve lighting or move closer to the camera.

**`cannot open display` error**
```bash
export DISPLAY=:0 && ./bin/creepyportrait 0
```

**Out of memory during compilation**
```bash
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile   # set CONF_SWAPSIZE=1024
sudo dphys-swapfile setup && sudo dphys-swapfile swapon
```

---

## What Changed from the Original

See [MODERNIZATION_NOTES.md](MODERNIZATION_NOTES.md) for the full change log from OF 0.8 (2013) to OF 0.12+.

Key fixes:
- All deprecated OF APIs updated
- GLSL `texture2D` → `texture` in desktop bump shader
- VBO built once in constructor — no per-frame GPU uploads
- `ambientTex` fallback binding for non-bump path
- `#include <iomanip>` added for Clang/libc++ compatibility
- `ofxRPiCameraVideoGrabber` correctly excluded from non-Pi builds

---

## Roadmap

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 1 | ✅ Done | Face tracking skull/jacks — fullscreen, boots on startup |
| Phase 2 | Planned | Flickering light, mood states, particle effects |
| Phase 3 | Planned | Sound, mic reactions, jump scares, gesture triggers |
| Phase 4 | Future | Additional 3D models |
| Phase 5 | Future | TBD |
| Phase 6 | Future | Full AI interactivity — voice, face recognition, conversation |

---

## License & Credits

**Source code** — Original copyright 2013 Tony DiCola, MIT License.

**3D models and textures** — Copyright [Video Copilot Inc](http://www.videocopilot.net/blog/2012/10/free-halloween-3d-model-pack/). See `bin/data/models/3D_Products_License_Agreement.pdf`.

**Red curtain image** — Copyright Flickr user [sethoscope](http://www.flickr.com/photos/57845051@N00/2884743046/), Creative Commons BY-NC-SA.

**ofxRPiCameraVideoGrabber** — Copyright Jason Van Cleave.

Modernized for OF 0.12+ / Raspberry Pi 4/5 by [@maserowik](https://github.com/maserowik).
