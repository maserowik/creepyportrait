# Creepy Portrait — Complete Raspberry Pi 4 / Pi 5 Install Guide

**What you'll end up with:** A Raspberry Pi 4 or Pi 5 that boots into a fullscreen
face-tracking 3D portrait. Point a USB webcam at anyone and the skull (or
jack-o-lantern) slowly turns to follow them.

> **Supported hardware: Raspberry Pi 4 and Pi 5 only.**
> Pi 3B, Pi Zero 2W, and all earlier models will not work — their GPU lacks
> the desktop OpenGL support required. Do not attempt the install on those boards.

**Time required:** About 2–3 hours, most of it unattended while things compile.

**Tested on:** Raspberry Pi OS Trixie 64-bit (released April 2026), Pi 4 4GB.

---

## What You Need Before You Start

| Item | Notes |
|------|-------|
| Raspberry Pi 4 or Pi 5 | Pi 4: 2GB minimum, 4GB recommended. Pi 5: any RAM size. |
| MicroSD card | 16GB minimum, Class 10 / A1 or better |
| USB webcam | Logitech C170 works for testing. C270 or C920 recommended for wall mount. |
| HDMI monitor + cable | Pi 4 and Pi 5 use micro-HDMI — you may need an adapter |
| USB keyboard | Just for setup |
| Power supply | Official USB-C: 5V/3A for Pi 4, 5V/5A for Pi 5. No phone chargers. |
| Second computer | Windows, Mac, or Linux — only used to flash the SD card |
| Internet connection | Wired ethernet recommended during install |

> **Critical:** Always run commands from a terminal on the Pi's physical desktop
> or inside a VNC desktop window. A plain SSH session has no display and the
> app will crash with `X11: Platform not initialized`.

---

## Part 1 — Flash the SD Card

Do this on your second computer, not the Pi.

**1.1** Download and install **Raspberry Pi Imager**:
```
https://www.raspberrypi.com/software/
```

**1.2** Insert your microSD card into your computer's card reader.

**1.3** Open Raspberry Pi Imager and make these selections:

- **Raspberry Pi Device** → match your board (`Raspberry Pi 4` or `Raspberry Pi 5`)
- **Operating System** → `Raspberry Pi OS (other)` → **`Raspberry Pi OS Full (64-bit)`**
  - Must be **Full** — it includes the desktop environment the skull needs.
  - Do NOT pick Lite — no display server means no skull.
- **Storage** → select your microSD card

**1.4** Click the gear icon (or "Edit Settings") and configure:

- Hostname: `creepyportrait`
- Enable SSH: yes
- Username and password: set and remember these
- WiFi: configure if not using ethernet

**1.5** Click **Save** then **Write**. Wait for it to finish and verify. Eject safely.

---

## Part 2 — First Boot and Critical X11 Switch

**2.1** Insert SD card into Pi. Connect monitor, keyboard, ethernet. Plug in power last.

**2.2** Wait for the desktop to appear. Complete the first-run wizard:
- Set country/language/timezone
- Change password if prompted
- Connect WiFi if not using ethernet
- Skip software update for now

**2.3** Open a terminal (`Ctrl+Alt+T` or taskbar).

**2.4** Switch from Wayland to X11. This step is **required** — new Pi OS defaults
to Wayland which is not compatible with openFrameworks/GLFW:

```bash
sudo raspi-config
```

Navigate to: **Advanced Options → Wayland → X11**

Confirm, then reboot:

```bash
sudo reboot
```

**2.5** After reboot, verify X11 is active:

```bash
echo $XDG_SESSION_TYPE
```

Must show `x11`. If it still shows `wayland`, repeat the raspi-config step above.

> **Why this matters:** Skipping this step causes the skull to crash immediately
> with `Failed to detect any supported platform` or `GLXBadFBConfig`. X11 is
> required — there is no workaround.

---

## Part 3 — System Update

```bash
sudo apt update && sudo apt upgrade -y
```

Takes 5–15 minutes. Let it finish completely, then reboot:

```bash
sudo reboot
```

---

## Part 4 — Install openFrameworks

**4.1** Download and extract openFrameworks:

```bash
cd ~
wget https://github.com/openframeworks/openFrameworks/releases/download/0.12.0/of_v0.12.0_linuxaarch64_release.tar.gz
tar -xf of_v0.12.0_linuxaarch64_release.tar.gz
rm of_v0.12.0_linuxaarch64_release.tar.gz
mv of_v0.12.0_linuxaarch64_release openFrameworks
```

**4.2** Run the OF dependency script:

```bash
cd ~/openFrameworks/scripts/linux/debian
sudo ./install_dependencies.sh
```

> **Expected warning — safe to ignore:**
> `E: Unable to locate package libgconf-2-4`
> This package was removed from Debian Trixie. openFrameworks does not actually
> need it. The script still installs everything else correctly. Continue.

**4.3** Install the additional packages that the OF script misses on Trixie.
Copy and paste this entire block as one command:

```bash
sudo apt install -y \
  libcairo2-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-bad1.0-dev \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav \
  libsndfile1-dev \
  libopenal-dev \
  libcurl4-openssl-dev \
  libpulse-dev \
  libasound2-dev \
  libpugixml-dev \
  liburiparser-dev \
  libmpg123-dev \
  libxxf86vm-dev \
  libboost-filesystem-dev \
  libboost-system-dev \
  libfreeimage-dev \
  librtaudio-dev \
  libjack-jackd2-dev \
  libopencv-dev \
  mesa-utils
```

Takes 5–10 minutes. All packages should install without errors.

**4.4** Verify the key libraries installed correctly:

```bash
pkg-config --modversion cairo
pkg-config --modversion sndfile
pkg-config --modversion openal
pkg-config --modversion libcurl
pkg-config --modversion opencv4
```

Each should print a version number. If any shows nothing, re-run the
`apt install` block from step 4.3.

---

## Part 5 — Compile openFrameworks

This is the long step. **Pi 4: 30–60 minutes. Pi 5: ~20 minutes.**
Start it and walk away.

```bash
cd ~/openFrameworks/libs/openFrameworksCompiled/project
make -j$(nproc) Release
```

**Watch progress in a second terminal window:**

```bash
watch -n 3 'echo "Compiled: $(find ~/openFrameworks/libs/openFrameworksCompiled/lib/linuxaarch64/obj/Release -name "*.o" | wc -l) files" && ls ~/openFrameworks/libs/openFrameworksCompiled/lib/linuxaarch64/libopenFrameworks.a 2>/dev/null && echo "DONE!" || echo "Still building..."'
```

You are done when you see `DONE!` and the main terminal shows:
```
make[1]: Leaving directory
```
with no red error lines.

**5.1** Run the spinning shapes test to confirm everything works.
Must be run from a terminal on the Pi desktop — not SSH:

```bash
cd ~/openFrameworks/examples/3d/3DPrimitivesExample
make -j$(nproc)
./bin/3DPrimitivesExample
```

You should see spinning 3D green shapes on screen. Press `Ctrl+C` to quit.

**Do not continue until this test works.**

---

## Part 6 — Clone and Build Creepy Portrait

```bash
cd ~/openFrameworks/apps/myApps
git clone -b update_test https://github.com/maserowik/creepyportrait creepyportrait
cd creepyportrait
make -j$(nproc) Release
```

Takes 2–5 minutes. Success looks like:

```
Linking bin/creepyportrait
make[1]: Leaving directory '...'
```

---

## Part 7 — Connect Your Webcam and Run

**7.1** Plug your USB webcam into any USB port on the Pi.

**7.2** Verify it is detected:

```bash
ls /dev/video*
```

You should see `/dev/video0` in the list. If you only see `/dev/video10`
and higher (no `video0`), the webcam is not detected — unplug, replug,
try a different USB port.

**7.3** Run from a terminal on the Pi desktop:

```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0
```

> **Why these are needed:**
> `MESA_GL_VERSION_OVERRIDE=3.3` — forces Mesa to expose OpenGL 3.3 which the shaders require. Without this the app may crash or render incorrectly on Pi 4.
> `GST_V4L2_USE_LIBV4L2=1` — tells GStreamer to use libv4l2 for webcam access, which fixes compatibility issues with some USB webcams including the C170.

The window opens fullscreen. Step back 1–2 metres, face the camera, and
wait 2–3 seconds. The skull will slowly turn to follow you.

> **C170 users:** The C170 has a fixed focus lens optimised for around 1 metre.
> For best detection stay close to that distance. Upgrade to a C920 for wall mount
> use where people will be at varying distances.

**Try all three models:**

```bash
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackevil    # evil jack-o-lantern
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackhappy   # happy jack-o-lantern
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 all         # all three, press m to cycle
```

> **Note — model switching:** The `m` key only works when launched with `all`. launching with a single model name like `skull` means there is only one model loaded so `m` appears to do nothing.

> **Note — Pi camera not supported:** The `pi` argument crashes on modern
> Pi OS (Bullseye 2021 and later). Always use a USB webcam with a numeric
> device ID (`0`, `1`, etc.).

---

## Keyboard Controls

| Key | What it does |
|-----|-------------|
| `v` | Toggle video overlay — shows camera feed and green box around detected faces |
| `r` | Toggle auto-rotation — skull spins on its own, good for testing without webcam |
| `m` | Switch to next model — **only works when launched with `all`, does nothing with a single model** |
| `Ctrl+C` | Quit |

---

## Part 8 — Auto-Start on Boot (Optional)

**8.1** Create the autostart file (replace `creepyportrait` with your username):

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/creepyportrait.desktop
```

**8.2** Paste this content:

```ini
[Desktop Entry]
Type=Application
Name=Creepy Portrait
Exec=/bin/bash -c 'cd /home/creepyportrait/openFrameworks/apps/myApps/creepyportrait && MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0'
X-GNOME-Autostart-enabled=true
```

Save: `Ctrl+O`, `Enter`. Exit: `Ctrl+X`.

**8.3** Set the Pi to boot to desktop without login prompt:

```bash
sudo raspi-config
```

Go to: **System Options → Boot / Auto Login → Desktop Autologin**

Reboot to test:

```bash
sudo reboot
```

---

## Tuning for Better Tracking

Edit `src/main.cpp` inside the `#ifdef TARGET_RASPBERRY_PI` block.
Edit `src/CreepyPortrait.cpp` for the angle multiplier setting.
After any change rebuild with `make -j$(nproc) Release`.

---

### `faceUpdateDelay`
**Current optimised value: `0.01`** *(in `src/main.cpp`)*

How long in seconds to wait between face detection runs.

- **Lower** = more frequent detection, snappier tracking, higher CPU load
- **Higher** = less frequent detection, smoother animation, lower CPU load
- `0.01` runs detection as fast as possible — increase to `0.5`–`1.0` if the Pi feels sluggish or the animation is choppy

---

### `videoFOV`
**Current value: `60`** *(in `src/main.cpp`)*

The diagonal field of view of your webcam in degrees. This must match your camera's actual spec or the skull will over or under rotate.

- **Too low** = skull over-rotates and overshoots faces at the edges of the frame
- **Too high** = skull under-rotates and barely moves even when a face moves far left or right
- Logitech C170 = `58`, Logitech C270 = `60`, Logitech C920 = `78`

---

### `faceDepth`
**Current optimised value: `20.0`** *(in `src/main.cpp`)*

An assumed distance value used in the rotation angle calculation. Does not represent a real physical distance — it is a tuning multiplier that scales how much the skull rotates in response to a detected face.

- **Higher** = skull rotates more aggressively to follow faces, more dramatic movement
- **Lower** = skull rotates less, subtle movement, may feel unresponsive
- If the skull barely moves try increasing this first. If the skull wildly overshoots try decreasing it.

---

### `noFaceResetSeconds`
**Current value: `6.0`** *(in `src/main.cpp`)*

How many seconds to wait after losing a detected face before the skull slowly rotates back to center.

- **Higher** = skull holds its last position longer before resetting — good for busy rooms where people move in and out of frame frequently
- **Lower** = skull snaps back to center quickly after a person walks away
- For a Halloween display `6.0`–`10.0` works well to avoid constant resetting

---

### `videoWidth` / `videoHeight`
**Current optimised value: `320x240`** *(in `src/main.cpp`)*

The resolution the face detector runs at. Higher resolution gives the detector more detail to work with but uses more CPU.

- **Higher** (`320x240`, `640x480`) = better detection at distance and in low light, more CPU load
- **Lower** (`160x120`) = faster detection, less CPU, may miss faces at distance or in poor lighting
- `320x240` is the recommended balance for Pi 4. Only increase to `640x480` on Pi 5.

---

### `angle.y` multiplier
**Current optimised value: `4.0`** *(in `src/CreepyPortrait.cpp`)*

Find the line: `return glm::vec2(angle.x * 2.0, angle.y * 4.0);`

Scales how much the skull rotates vertically (up/down) in response to a detected face position. The `angle.x` value controls left/right rotation.

- **Higher `angle.y`** = more up/down movement, skull reacts more to vertical face position changes
- **Lower `angle.y`** = less up/down movement, skull stays more level regardless of face height
- **Higher `angle.x`** = more left/right rotation range
- **Lower `angle.x`** = more subtle left/right movement
- The Haar face detector is less sensitive vertically than horizontally, which is why `angle.y` is set higher than `angle.x`

---

### `setNeighbors`
**Current optimised value: `5`** *(in `src/VideoFaceDetector.cpp`)*

How many overlapping detections are required before something is confirmed as a real face. This is the most important setting for reducing false positives.

- **Higher** = fewer false positives, more stable tracking, may miss faces in poor lighting
- **Lower** = more sensitive, picks up faces easier but also detects background objects as faces
- `1` is too sensitive — background objects get detected as faces
- `5` is the recommended balance — stable tracking with minimal false positives
- If the detector stops finding your face try dropping to `3` or `4`

---

### `setScaleHaar`
**Current value: `1.05`** *(in `src/VideoFaceDetector.cpp`)*

The scale step used when scanning the image at multiple sizes to find faces.

- **Lower** (e.g. `1.05`) = more thorough scan, finds faces at more sizes, slower
- **Higher** (e.g. `1.2`) = faster scan, may miss faces at certain distances
- `1.05` is the recommended value — leave this alone unless CPU is struggling

---

### `setMinAreaRadius`
**Current optimised value: `30`** *(in `src/VideoFaceDetector.cpp`)*

The minimum size in pixels a detected region must be to be considered a face. Filters out small false positives from distant objects or background patterns.

- **Higher** = only detects larger/closer faces, ignores small distant detections
- **Lower** = detects smaller/more distant faces but increases false positives
- `30` works well at 1–3 metre range with `320x240` resolution
- If upgrading to `640x480` resolution consider increasing to `50`–`60`

---

## Troubleshooting

**`[warning] ofGstVideoUtils: update(): ofGstVideoUtils not loaded` — repeating**
This is NOT an error. It means the webcam is not plugged in and GStreamer
cannot open the video device. The skull will still display correctly.
To stop the warnings: plug in your USB webcam. To suppress them while
testing without a webcam:
```bash
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 2>/dev/null
```

**`Failed to detect any supported platform` or `GLXBadFBConfig`**
X11 is not active. Run `echo $XDG_SESSION_TYPE` — if it shows `wayland`
you need to switch to X11 via raspi-config:
```bash
sudo raspi-config
# Advanced Options → Wayland → X11
sudo reboot
```

**`X11: Platform not initialized` / segfault on launch**
You are running from a plain SSH session with no display. Open a terminal
from inside the Pi desktop (physical monitor or VNC desktop window) and
run from there. Or set the display manually:
```bash
export DISPLAY=:0
./bin/creepyportrait 0
```

**Skull is completely white with no textures**
This happens when running through VNC — VNC software rendering does not
support the hardware OpenGL shaders. Connect a real HDMI monitor for
correct rendering with full textures and lighting.

**`E: Unable to locate package libgconf-2-4` during install_dependencies.sh**
Safe to ignore. This package was removed from Debian Trixie. The script
still installs all the packages that actually matter.

**`couldn't find gstreamer-app-0.10` or similar old package names**
The OF dependency script references some old Debian package names. Install
the modern replacements using the full apt block in Part 4.3.

**App opens and immediately closes**
Always run from the project directory — the app looks for data files
relative to where you launched it:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
./bin/creepyportrait 0
```

**`Cannot find haarcascade_frontalface_default.xml`**
Same as above — wrong launch directory. Always `cd` into the project folder first.

**No `/dev/video0` after plugging in webcam**
Check the webcam is detected at all:
```bash
lsusb
```
Your webcam should appear in the list. If not, try a different USB port
or test the webcam on another computer.

**Face not tracking — skull stares forward**
Press `v` to show the video overlay. If you can see yourself but no green
box appears: move closer to the camera (1 metre), improve room lighting,
face the camera directly. The Haar detector works best with a well-lit
face looking straight at the camera.

**Very choppy / slow tracking**
Increase `faceUpdateDelay` to `1.5` or `2.0` in `src/main.cpp` and rebuild.
The skull reacts slower but animation is smoother.

**Out of memory during compilation**
Add swap space:
```bash
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile   # change CONF_SWAPSIZE=100 to CONF_SWAPSIZE=1024
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```
Then retry `make -j$(nproc) Release`.

**`ofGLProgrammableRenderer` compile errors**
Make sure you cloned from the `update_test` branch, not `main`:
```bash
git clone -b update_test https://github.com/maserowik/creepyportrait creepyportrait
```

---

## Hardware Summary

| Pi Model | Works? | Notes |
|----------|--------|-------|
| Pi 4 (2GB+) | ✅ Yes | Recommended minimum |
| Pi 5 | ✅ Yes | Faster, better tracking |
| Pi 400 | ✅ Yes | Same chip as Pi 4 |
| Pi 3B / 3B+ | ❌ No | Wrong GPU for modern Pi OS |
| Pi Zero 2W | ❌ No | Same GPU issue + only 512MB RAM |
| Pi 1 / 2 / Zero | ❌ No | Too slow, wrong GPU, 32-bit only |

**If your board is not in the ✅ column, stop here — it will not work.**

---

## Webcam Reference

| Camera | FOV | Focus | Low Light | Notes |
|--------|-----|-------|-----------|-------|
| Logitech C170 | 58° | Fixed | Basic | Good for testing, limited range |
| Logitech C270 | 60° | Fixed | Good | Solid all-rounder |
| Logitech C920 | 78° | Auto | Excellent | Recommended for wall mount |

Set `videoFOV` in `src/main.cpp` to match your camera's FOV value above.

---

*Project originally by Tony DiCola / Adafruit (2013).
Modernized for openFrameworks 0.12+ on Raspberry Pi 4/5 by @maserowik.*
