# Creepy Portrait — Complete Raspberry Pi 4 / Pi 5 Install Guide

**What you'll end up with:** A Raspberry Pi 4 or Pi 5 that boots into a fullscreen
face-tracking 3D portrait. Point a USB webcam at anyone and the skull (or
jack-o-lantern) slowly turns to follow them.

> **Supported hardware: Raspberry Pi 4 and Pi 5 only.**
> The Pi 3B, Pi Zero 2W, and all earlier models will not work — their VideoCore IV
> GPU lacks the desktop OpenGL support required by this project on modern
> Raspberry Pi OS. Do not attempt the install on those boards.

**Time required:** About 1.5–2 hours, most of it unattended while things
compile.

---

## What you need before you start

| Item | Notes |
|------|-------|
| Raspberry Pi 4 or Pi 5 | **Pi 4:** 2GB RAM minimum, 4GB recommended. **Pi 5:** any RAM size works. Pi 3 and earlier are not supported. |
| MicroSD card | 16GB minimum, Class 10 / A1 or better |
| USB webcam | Any webcam that works on Linux. Logitech C270 or C920 are ideal. |
| HDMI monitor + cable | Pi 4 and Pi 5 both use micro-HDMI — you may need an adapter |
| USB keyboard | Just for setup |
| Power supply | Official USB-C supply: 5V/3A for Pi 4, 5V/5A for Pi 5. Phone chargers cause instability. |
| A second computer | Windows, Mac, or Linux — used only to flash the SD card |
| Internet connection | Wired ethernet recommended for the Pi during install |

---

## Part 1 — Flash the SD Card

Do this on your second computer, not the Pi.

**1.1** Download and install **Raspberry Pi Imager** from:
```
https://www.raspberrypi.com/software/
```
Available for Windows, macOS, and Linux.

**1.2** Insert your microSD card into your computer's card reader.

**1.3** Open Raspberry Pi Imager and make these selections:

- **Raspberry Pi Device** → `Raspberry Pi 4` or `Raspberry Pi 5` (match your board)
- **Operating System** → `Raspberry Pi OS (other)` → `Raspberry Pi OS (64-bit)`
  - This is the full desktop version. Do NOT choose Lite — it has no display server.
- **Storage** → select your microSD card

**1.4** Click the **gear icon** (or "Edit Settings") before writing. Configure:

- Set hostname (e.g. `creepyportrait`)
- Enable SSH
- Set a username and password (remember these)
- Configure WiFi if you're not using ethernet

**1.5** Click **Save**, then **Write**. Wait for it to complete and verify.
Eject the card safely.

---

## Part 2 — First Boot

**2.1** Insert the SD card into the Pi. Connect your monitor, keyboard,
and ethernet cable. Plug in power last.

**2.2** Wait for the desktop to appear. Complete the first-run wizard:
- Set your country/language/timezone
- Change password if prompted
- Connect to WiFi if not using ethernet
- **Skip** the software update for now (we'll do it in the terminal)

**2.3** Open a terminal. You'll be typing a lot of commands here.
Find it in the taskbar or press `Ctrl+Alt+T`.

---

## Part 3 — System Update

```bash
sudo apt update && sudo apt upgrade -y
```

This takes 5–15 minutes depending on how many updates are pending. Let it finish completely.

Reboot when done:

```bash
sudo reboot
```

---

## Part 4 — Install openFrameworks

openFrameworks is the graphics framework the project is built on. It needs
to be compiled on the Pi itself. **This is the long step — it takes 30–60
minutes.** You don't need to watch it.

**4.1** Open a terminal and download openFrameworks:

```bash
cd ~
wget https://github.com/openframeworks/openFrameworks/releases/download/0.12.0/of_v0.12.0_linuxaarch64_release.tar.gz
```

If wget is slow, you can also download it on your other computer and copy it
over via USB. The filename is `of_v0.12.0_linuxaarch64_release.tar.gz`.

**4.2** Extract it:

```bash
tar -xf of_v0.12.0_linuxaarch64_release.tar.gz
rm of_v0.12.0_linuxaarch64_release.tar.gz
mv of_v0.12.0_linuxaarch64_release openFrameworks
```

**4.3** Install all system dependencies (say yes to everything):

```bash
cd ~/openFrameworks/scripts/linux/debian
sudo ./install_dependencies.sh
```

This installs OpenCV, GStreamer, OpenAL, and about 30 other libraries.
Takes 5–10 minutes.

**4.4** Compile the openFrameworks library. Go make a cup of tea.

```bash
cd ~/openFrameworks/libs/openFrameworksCompiled/project
make -j$(nproc) Release
```

`-j$(nproc)` uses all 4 CPU cores in parallel. On a Pi 4 this takes roughly
30–45 minutes. You'll see a lot of compiler output scrolling by — that's
normal. It's done when you see `make[1]: Leaving directory` with no error.

**4.5** Verify it worked by compiling and running a test app:

```bash
cd ~/openFrameworks/examples/3d/3DPrimitivesExample
make -j$(nproc)
./bin/3DPrimitivesExample
```

You should see spinning 3D shapes on screen. Press `Ctrl+C` to quit.
**Do not continue until this test works.**

---

## Part 5 — Install the Creepy Portrait Project

**5.1** Copy the project zip to the Pi. You can do this a few ways:

- **USB drive:** Copy `creepyportrait-fixed.zip` to a USB stick, plug it
  into the Pi, then:
  ```bash
  cp /media/pi/YOUR_DRIVE_NAME/creepyportrait-fixed.zip ~/
  ```

- **Over the network (from your other computer):**
  ```bash
  # Run this on your OTHER computer (replace pi-hostname and username):
  scp creepyportrait-fixed.zip pi@creepyportrait.local:~/
  ```

- **Or clone directly from GitHub if you've pushed it there:**
  ```bash
  cd ~/openFrameworks/apps/myApps
  git clone https://github.com/maserowik/creepyportrait creepyportrait
  # Then skip step 5.2
  ```

**5.2** Extract it into the right place:

```bash
cd ~/openFrameworks/apps/myApps
unzip ~/creepyportrait-fixed.zip
mv creepyportrait-fixed creepyportrait
```

**5.3** Verify the structure looks right:

```bash
ls ~/openFrameworks/apps/myApps/creepyportrait/
```

You should see: `src/  bin/  addons/  Makefile  addons.make  config.make  README.md`

**5.4** Verify OpenCV is present (required for face detection):

```bash
pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv 2>/dev/null || echo "OpenCV not found"
```

If you see `OpenCV not found`, install it manually:

```bash
sudo apt install -y libopencv-dev
```

---

## Part 6 — Build the Project

```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
make -j$(nproc) Release
```

Takes 2–5 minutes. Success looks like:

```
Linking bin/creepyportrait
make[1]: Leaving directory '...'
```

If you see errors, check the troubleshooting section at the bottom.

---

## Part 7 — Connect Your Webcam and Run

**7.1** Plug your USB webcam in. Check it's detected:

```bash
ls /dev/video*
```

It should show `/dev/video0` (and maybe `/dev/video1` — the first one is
usually your webcam).

**7.2** Run the portrait. Make sure you're still in the project folder:

```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
./bin/creepyportrait 0
```

The `0` is the webcam device ID matching `/dev/video0`.

The window will open fullscreen. Step back 1–2 metres, face the camera
directly, and wait 2–3 seconds for the first detection. The skull should
slowly turn to follow you.

**Try all three models:**

```bash
# Evil jack-o-lantern
./bin/creepyportrait 0 jackevil

# Happy jack-o-lantern
./bin/creepyportrait 0 jackhappy

# Load all three, press 'm' to cycle between them
./bin/creepyportrait 0 all
```

> **Note — Pi camera module not supported on modern Pi OS:** The `pi` argument
> you'll see in the usage output (e.g. `./bin/creepyportrait pi`) requires the
> legacy MMAL/OMX camera stack, which was removed in Raspberry Pi OS Bullseye
> (November 2021) and all later releases. If you try it on a modern Pi OS it will
> crash immediately. Always use a USB webcam with a numeric device ID (`0`, `1`,
> etc.) as shown above. If you specifically need Pi camera module support, see the
> `libcamera`-based replacement addon mentioned in the project's
> `MODERNIZATION_NOTES.md`.

---

## Keyboard Controls

| Key | What it does |
|-----|-------------|
| `v` | Toggle video overlay — shows the camera feed and a green box around detected faces |
| `r` | Toggle auto-rotation mode — skull spins on its own, good for testing |
| `m` | Switch to next model (only works when launched with `all`) |
| `Ctrl+C` | Quit |

Press `v` first if the skull isn't tracking — it lets you see whether the
camera is working and whether a face is being detected.

---

## Part 8 — Make it Run on Boot (Optional)

If you want the portrait to start automatically when the Pi powers on:

**8.1** Create an autostart file:

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/creepyportrait.desktop
```

**8.2** Paste this content (adjust `0` to your webcam device ID if needed):

```ini
[Desktop Entry]
Type=Application
Name=Creepy Portrait
Exec=/bin/bash -c 'cd /home/pi/openFrameworks/apps/myApps/creepyportrait && ./bin/creepyportrait 0'
X-GNOME-Autostart-enabled=true
```

Save with `Ctrl+O`, `Enter`, then exit with `Ctrl+X`.

**8.3** Set the Pi to boot straight to desktop without asking for a login:

```bash
sudo raspi-config
```

Go to `System Options` → `Boot / Auto Login` → `Desktop Autologin`.

Reboot to test:

```bash
sudo reboot
```

---

## Tuning for Better Tracking

Open `src/main.cpp` and find the `#ifdef TARGET_RASPBERRY_PI` block.
After editing, rebuild with `make -j$(nproc) Release`.

| Setting | Default | What to change |
|---------|---------|---------------|
| `faceUpdateDelay` | `2.0` | Lower to `0.5`–`1.0` on a Pi 4 for snappier tracking |
| `videoFOV` | `60` | Match your webcam's FOV. Wide-angle webcams need `75`–`90` |
| `faceDepth` | `10.0` | Increase if the head barely moves; decrease if it over-rotates |
| `noFaceResetSeconds` | `6.0` | How long until it returns to center when no face is found |
| `videoWidth/Height` | `160x120` | Increase to `320x240` for better detection, uses more CPU |

---

## Troubleshooting

**The app opens and immediately closes**
Run from the terminal (not double-click) so you can see the error. Most
common cause is not being in the project directory when you run it — the
app can't find its data files.

**`Cannot find haarcascade_frontalface_default.xml`**
You must run the binary from the project folder:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
./bin/creepyportrait 0       # correct
bin/creepyportrait 0         # also correct
/home/pi/.../creepyportrait  # WRONG — run from project folder
```

**No webcam detected / `ls /dev/video*` shows nothing**
Unplug and replug the webcam. Some webcams need a moment. Check:
```bash
lsusb
```
Your webcam should appear in the list. If not, try a different USB port.

**Face not tracking**
Press `v` to show the video overlay. If you can see yourself in the
corner but no green box appears, move closer, improve lighting, or face
the camera more directly. The Haar detector works best with a
well-lit face looking straight at the camera.

**Very choppy / slow tracking**
Increase `faceUpdateDelay` to `1.5` or `2.0` in `main.cpp` and rebuild.
The skull will be slower to react but the animation will be smoother.

**`cannot open display` error**
This happens when running over SSH without X forwarding. Run on the Pi
desktop directly, or prefix your command with:
```bash
export DISPLAY=:0
./bin/creepyportrait 0
```

**Compile errors mentioning `ofGLProgrammableRenderer`**
Make sure you're using the `creepyportrait-fixed` zip from this project,
not the original 2013 code from GitHub.

**Out of memory during compilation**
Add some swap space:
```bash
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile   # change CONF_SWAPSIZE=100 to CONF_SWAPSIZE=1024
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```
Then retry the `make` command.

---

## Hardware Summary

| Pi Model | Works? | Notes |
|----------|--------|-------|
| Pi 4 (2GB+) | ✅ Yes | Recommended minimum |
| Pi 5 | ✅ Yes | Faster, better tracking |
| Pi 400 | ✅ Yes | Same chip as Pi 4 |
| Pi 3B / 3B+ | ❌ No | VideoCore IV GPU lacks desktop OpenGL support on modern Pi OS |
| Pi Zero 2W | ❌ No | Same GPU issue + only 512MB RAM |
| Pi 1 / 2 / Zero | ❌ No | Too slow, wrong GPU, 32-bit only |

**If your board is not in the ✅ column, stop here — it will not work.**

---

*Project originally by Tony DiCola / Adafruit (2013). Modernized for
openFrameworks 0.12+ on Raspberry Pi 4/5.*
