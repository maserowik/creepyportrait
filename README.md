# Creepy Portrait

A face-tracking haunted portrait for Raspberry Pi 4/5. Point a USB webcam at
anyone and a 3D skull (or jack-o-lantern) slowly turns to follow them —
fullscreen, on a red curtain background, booting automatically on startup.

Built on [openFrameworks 0.12+](https://openframeworks.cc). Originally created
by [Tony DiCola / Adafruit (2013)](https://learn.adafruit.com/creepy-face-tracking-portrait)
and modernized for current hardware and OS versions.

---

## Hardware Requirements

| Item | Notes |
|------|-------|
| Raspberry Pi 4 (2GB+) or Pi 5 | Pi 3 and earlier will NOT work — wrong GPU |
| USB webcam | Logitech C270 or C920 recommended |
| MicroSD card | 16GB minimum, Class 10 / A1 |
| HDMI monitor | Pi 4/5 use micro-HDMI |
| USB-C power supply | 5V/3A for Pi 4 · 5V/5A for Pi 5 |

---

## Simulator

An interactive 3D preview is included in the `simulator/` folder. It uses the
real models and textures from the project so you can see exactly how the skull
tracks before building on the Pi.

```bash
# From the repo root:
python3 -m http.server 8080
# Then open: http://localhost:8080/simulator/
```

Features: all 3 models, 9 camera position options with quality scores, vertical
tracking toggle, speed and rotation range tuning, live status readouts.

---

## Quick Start

> **Before cloning:** You must first install openFrameworks and all dependencies.
> See [INSTALL_GUIDE_RPI4.md](INSTALL_GUIDE_RPI4.md) for the full step-by-step guide.

Once openFrameworks is installed and the spinning shapes test passes:

```bash
cd ~/openFrameworks/apps/myApps
git clone -b update_test https://github.com/maserowik/creepyportrait creepyportrait
cd creepyportrait
make -j$(nproc) Release
./bin/creepyportrait 0
```

> **Important:** Run from a terminal on the Pi desktop — not a plain SSH session.
> X11 must be active (`echo $XDG_SESSION_TYPE` should show `x11`).

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
./bin/creepyportrait 0 all         # all three
```

---

## Keyboard Controls

| Key | Action |
|-----|--------|
| `v` | Toggle camera/face-detection overlay |
| `r` | Toggle auto-rotation (test without webcam) |
| `m` | Cycle to next model (requires `all`) |
| `Ctrl+C` | Quit |

---

## Known Warnings (Not Errors)

**`[warning] ofGstVideoUtils: update(): ofGstVideoUtils not loaded`**
This repeating warning appears when no webcam is plugged in. It is harmless —
the skull still displays correctly. Plug in your USB webcam to stop it, or
suppress it with `./bin/creepyportrait 0 2>/dev/null`.

---

## Auto-Start on Boot

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/creepyportrait.desktop
```

Paste (replace `creepyportrait` with your username):

```ini
[Desktop Entry]
Type=Application
Name=Creepy Portrait
Exec=/bin/bash -c 'cd /home/creepyportrait/openFrameworks/apps/myApps/creepyportrait && ./bin/creepyportrait 0'
X-GNOME-Autostart-enabled=true
```

Enable desktop autologin:

```bash
sudo raspi-config
# System Options → Boot / Auto Login → Desktop Autologin
sudo reboot
```

---

## Tuning

Edit `src/main.cpp` inside the `#ifdef TARGET_RASPBERRY_PI` block,
then rebuild with `make -j$(nproc) Release`.

| Setting | Default | Notes |
|---------|---------|-------|
| `faceUpdateDelay` | `2.0` | Lower to `0.5`–`1.0` for snappier tracking |
| `videoFOV` | `60` | Match your webcam — wide-angle needs `75`–`90` |
| `faceDepth` | `10.0` | Increase if skull barely moves; decrease if over-rotates |
| `noFaceResetSeconds` | `6.0` | Time before skull resets to center when face lost |
| `videoWidth/Height` | `160x120` | Increase to `320x240` for better detection |

---

## What Changed from the Original

See [MODERNIZATION_NOTES.md](MODERNIZATION_NOTES.md) for the full change log
from OF 0.8 (2013) to OF 0.12+.

Key changes:
- All deprecated OF APIs updated (`loadImage`, `ofRotateY`, `ofVec2f/3f`, etc.)
- Window setup migrated to `ofGLESWindowSettings` / `ofGLWindowSettings` (OF 0.12 API)
- GLSL `texture2D` → `texture` in desktop bump shader
- VBO built once in constructor — no per-frame GPU uploads
- `ambientTex` fallback binding prevents undefined sampler reads
- `#include <iomanip>` added for Clang/libc++ compatibility
- `ofxRPiCameraVideoGrabber` excluded from non-Pi builds
- `ofGLProgrammableRenderer` removed — window settings handle renderer selection
- Tested and fixed on Raspberry Pi OS Trixie (Debian 13, April 2026)

---

## Roadmap

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 1 | ✅ Done | Face tracking skull/jacks — fullscreen, boots on startup |
| Phase 2 | Planned | Flickering light, mood states, particle effects |
| Phase 3 | Planned | Sound, mic reactions, jump scares, gesture triggers |
| Phase 4 | Future | Additional 3D models from Video Copilot Halloween pack |
| Phase 5 | Future | Phone/browser UI to select models and control settings |
| Phase 6 | Future | Full AI interactivity — voice, face recognition, conversation |

---

## License & Credits

**Source code** — Original copyright 2013 Tony DiCola, MIT License.

**3D models and textures** — Copyright [Video Copilot Inc](http://www.videocopilot.net/blog/2012/10/free-halloween-3d-model-pack/).
See `bin/data/models/3D_Products_License_Agreement.pdf`.

**Red curtain image** — Copyright Flickr user [sethoscope](http://www.flickr.com/photos/57845051@N00/2884743046/),
Creative Commons BY-NC-SA.

**ofxRPiCameraVideoGrabber** — Copyright Jason Van Cleave.

Modernized for OF 0.12+ / Raspberry Pi 4/5 / Pi OS Trixie by [@maserowik](https://github.com/maserowik).
