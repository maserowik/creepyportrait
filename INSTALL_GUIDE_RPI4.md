# Creepy Portrait — Complete Setup Guide for Raspberry Pi 4 and Pi 5

**What you will end up with:** A Raspberry Pi that boots up and shows a fullscreen 3D skull on your monitor. Point a USB webcam at anyone walking past and the skull slowly turns its head to follow them. When nobody is around it wanders on its own. The eyes blink and move. It plays spooky audio clips. It is genuinely unsettling.

**No programming experience needed.** Every command is written out exactly as you need to type it. When this guide says "type this" it means type it exactly — every letter, every symbol, every space matters.

**How long it takes:** About 2 to 3 hours, but most of that time the Pi is doing work on its own while you wait. You are not sitting at a keyboard the whole time.

**Tested on:** Raspberry Pi 4 with 4GB RAM, Raspberry Pi OS Trixie 64-bit (April 2026).

---

## Before You Start — What You Need

Read this whole section before buying anything.

**The computer (Raspberry Pi):**
You need a Raspberry Pi 4 or Pi 5. The Pi 4 with 4GB of RAM is the recommended choice — it is widely available secondhand for around $35–50. The Pi 5 is faster but more expensive. Pi 3, Pi Zero, and anything older will not work — the graphics hardware inside them is the wrong type.

**The memory card (MicroSD):**
This is like the hard drive for the Pi. Get a 16GB or larger card. Look for "Class 10" or "A1" on the packaging — this tells you it is fast enough. SanDisk and Samsung brands are reliable. Avoid no-name cheap cards.

**The camera (USB webcam):**
The Logitech C270 is the recommended choice — it is inexpensive, widely available, and works reliably. The C920 is better for a permanent installation because it handles low light better and has a wider field of view. Avoid very cheap no-name webcams — many have compatibility problems.

**The screen (HDMI monitor):**
Any monitor or TV with an HDMI input works. The Pi 4 and Pi 5 use a smaller connector called Micro-HDMI — you will need either a Micro-HDMI to HDMI cable or an adapter. These cost a few dollars and are available anywhere.

**The power supply:**
Do not use a phone charger. You need the official Raspberry Pi power supply. For Pi 4 it is 5V/3A USB-C. For Pi 5 it is 5V/5A USB-C. Using the wrong power supply causes random crashes and corrupted memory cards.

**A speaker:**
The skull plays audio clips when it detects a face. Any USB speaker works — plug it into one of the USB ports on the Pi. You can also use a 3.5mm speaker plugged into the headphone jack. If you do not have a speaker the skull will still work, just silently.

**A second computer:**
You need a Windows PC or Mac to prepare the memory card. You only use it once at the beginning. Any computer that has a card reader or a USB card reader dongle will work.

**Internet connection:**
Connect the Pi to your router with an ethernet cable during setup. Wi-Fi works but is slower and less reliable for a big download.

---

## Part 1 — Prepare the Memory Card

Do this on your Windows PC or Mac, not on the Pi.

**Step 1.1 — Download Raspberry Pi Imager**

Go to this address in your web browser and download the Imager for your computer:
```
https://www.raspberrypi.com/software/
```
Install it like any normal program.

**Step 1.2 — Insert the memory card**

Put your MicroSD card into your computer's card reader. If your computer does not have one built in, use a USB card reader dongle — they cost about $5.

**Step 1.3 — Open Raspberry Pi Imager and choose your settings**

Open the Raspberry Pi Imager program. You will see three boxes to fill in:

First box — **Raspberry Pi Device:** Click it and choose `Raspberry Pi 4` or `Raspberry Pi 5` to match what you have.

Second box — **Operating System:** Click it, then choose `Raspberry Pi OS (other)`, then choose **`Raspberry Pi OS Full (64-bit)`**. It must say Full — the version without Full does not include the graphical desktop that the skull needs to run.

Third box — **Storage:** Click it and choose your MicroSD card from the list.

**Step 1.4 — Configure your settings**

Click the button that says "Edit Settings" or looks like a gear icon. Fill in:
- Hostname: type `creepyportrait`
- Enable SSH: tick the box (this lets you control the Pi remotely if needed)
- Username: choose a username, for example `creepyportrait`
- Password: choose a password and write it down
- Wi-Fi: if you are not using an ethernet cable, enter your Wi-Fi name and password here

Click Save.

**Step 1.5 — Write the card**

Click Write. It will warn you that everything on the card will be erased — click Yes. Wait for it to finish writing and then verify. When it says complete, eject the card safely and remove it.

---

## Part 2 — First Boot and Important Display Setting

**Step 2.1 — Connect everything to the Pi**

Connect in this order:
1. Insert the MicroSD card into the slot on the Pi
2. Connect your monitor with the Micro-HDMI cable
3. Connect your USB keyboard
4. Connect your ethernet cable to your router
5. Plug in the power supply last

The Pi will start booting automatically when you plug in the power.

**Step 2.2 — Complete the first-run setup**

Wait for the desktop to appear — it may take a minute or two. You will see a setup wizard. Go through it:
- Set your country, language, and timezone
- Change your password if prompted
- Connect to Wi-Fi if you are not using ethernet
- When it asks about software updates, skip for now

**Step 2.3 — Open a Terminal**

A terminal is a window where you type commands. To open one, look at the taskbar at the top of the screen and find an icon that looks like a black rectangle, or press `Ctrl+Alt+T` on your keyboard.

When the terminal opens you will see a blinking cursor. This is where you will type all the commands in this guide. Type them exactly as written, then press Enter to run them.

**Step 2.4 — Switch to X11 display mode**

This is a required step. The Pi's new operating system uses a display system called Wayland by default, but the skull program needs an older system called X11. You need to switch it.

Type this command and press Enter:
```bash
sudo raspi-config
```

It will ask for your password — type it and press Enter. A blue menu will appear. Use the arrow keys on your keyboard to navigate.

Go to: **Advanced Options** → press Enter → **Wayland** → press Enter → **X11** → press Enter

Press the right arrow key to highlight OK and press Enter. When it asks to reboot, choose Yes.

**Step 2.5 — Confirm X11 is working**

After the Pi reboots, open a terminal again and type:
```bash
echo $XDG_SESSION_TYPE
```

The screen must show `x11`. If it shows `wayland` you need to repeat step 2.4.

> **Why this matters:** If you skip this step the skull program will crash immediately with an error message. X11 is required and there is no other way around it.

---

## Part 3 — Update the System

Type this command and press Enter:
```bash
sudo apt update && sudo apt upgrade -y
```

This updates all the software on the Pi to the latest versions. It will take between 5 and 15 minutes. Let it finish completely. When it is done, reboot:
```bash
sudo reboot
```

---

## Part 4 — Install openFrameworks

openFrameworks is the graphics and media toolkit that the skull program is built on. You need to install it before you can build the skull.

**Step 4.1 — Download openFrameworks**

Type each of these commands one at a time, pressing Enter after each one:
```bash
cd ~
wget https://github.com/openframeworks/openFrameworks/releases/download/0.12.0/of_v0.12.0_linuxaarch64_release.tar.gz
tar -xf of_v0.12.0_linuxaarch64_release.tar.gz
rm of_v0.12.0_linuxaarch64_release.tar.gz
mv of_v0.12.0_linuxaarch64_release openFrameworks
```

The download is large and may take several minutes depending on your internet speed.

**Step 4.2 — Install dependencies**

Dependencies are other software packages that openFrameworks needs to work. Run this script:
```bash
cd ~/openFrameworks/scripts/linux/debian
sudo ./install_dependencies.sh
```

> **You will see this warning — it is safe to ignore:**
> `E: Unable to locate package libgconf-2-4`
> This package no longer exists in the current version of the operating system. The script installs everything else correctly. Keep going.

**Step 4.3 — Install additional packages**

Copy and paste this entire block into the terminal at once, then press Enter:
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
  mesa-utils \
  python3-pip \
  python3-rpi.gpio
```

This takes 5 to 10 minutes. Everything should install without errors.

**Step 4.4 — Install the LED Python library**

This installs the software needed to control the LED strip. If you are not adding an LED strip you can skip this step, but it does not hurt to run it anyway:
```bash
sudo pip3 install rpi_ws281x --break-system-packages
```

**Step 4.5 — Verify the key libraries installed correctly**

Run each of these commands. Each one should print a version number:
```bash
pkg-config --modversion cairo
pkg-config --modversion sndfile
pkg-config --modversion openal
pkg-config --modversion libcurl
pkg-config --modversion opencv4
```

If any of them shows nothing, go back and run the install block from step 4.3 again.

---

## Part 5 — Build openFrameworks

This is the longest step. On a Pi 4 it takes 30 to 60 minutes. On a Pi 5 about 20 minutes. Start it and go do something else.

```bash
cd ~/openFrameworks/libs/openFrameworksCompiled/project
make -j$(nproc) Release
```

If you want to watch the progress, open a second terminal window and type:
```bash
watch -n 3 'echo "Compiled: $(find ~/openFrameworks/libs/openFrameworksCompiled/lib/linuxaarch64/obj/Release -name "*.o" | wc -l) files" && ls ~/openFrameworks/libs/openFrameworksCompiled/lib/linuxaarch64/libopenFrameworks.a 2>/dev/null && echo "DONE!" || echo "Still building..."'
```

You are done when you see `DONE!` and the original terminal shows `make[1]: Leaving directory` with no red error lines.

**Step 5.1 — Test that it worked**

This test must be run from a terminal on the Pi desktop — not over SSH. Type:
```bash
cd ~/openFrameworks/examples/3d/3DPrimitivesExample
make -j$(nproc)
./bin/3DPrimitivesExample
```

You should see spinning 3D shapes on screen. Press `Ctrl+C` to quit.

**Do not continue to the next step until this test works.**

---

## Part 6 — Download and Build Creepy Portrait

```bash
cd ~/openFrameworks/apps/myApps
git clone https://github.com/maserowik/creepyportrait creepyportrait
cd creepyportrait
make -j$(nproc) Release
```

This takes 2 to 5 minutes. You are done when you see `Linking bin/creepyportrait` with no red error lines.

---

## Part 7 — Add Your Audio Clips

The skull plays spooky audio clips when it detects a face and while it wanders. It comes with no audio files — you provide your own.

**What kind of files work:**
- Format: WAV files only (not MP3, not AAC)
- Any sample rate and bit depth works
- Any length works — short clips of 3 to 15 seconds work best
- You can have as many clips as you like — the skull picks one at random each time

**Where to put them:**

The audio folder is already created for you. Copy your WAV files into it:
```
~/openFrameworks/apps/myApps/creepyportrait/bin/data/audio/
```

To open this folder in the Pi's file manager, open the Files app from the desktop and navigate to:
`Home → openFrameworks → apps → myApps → creepyportrait → bin → data → audio`

Then drag and drop your WAV files in.

**What happens without audio files:**
The skull will still work perfectly — it just will not play any sounds. You can add audio files at any time without rebuilding anything.

**When audio plays:**
- One clip plays immediately when the skull first detects a face
- While a face is present, another clip plays every 8 to 20 seconds
- While the skull is wandering with no face detected, a clip plays every 15 to 45 seconds
- Press `s` at any time to play a random clip immediately

---

## Part 8 — Plug In Your Webcam and Run

**Step 8.1 — Connect the webcam**

Plug your USB webcam into any USB port on the Pi.

**Step 8.2 — Check the Pi can see it**

Type:
```bash
ls /dev/video*
```

You should see `/dev/video0` in the list. If you only see numbers like `/dev/video10` or higher with no `video0`, unplug the webcam, wait a few seconds, and plug it back in.

**Step 8.3 — Run the program**

Type:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0
```

The window will open fullscreen. Step back 1 to 2 metres from the camera, face it directly, and wait a few seconds. The skull will turn to look at you.

**Step 8.4 — Try the other models**

The skull comes in three versions. To try each one:
```bash
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackevil
MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0 jackhappy
```

> **Note:** The `all` argument and `m` key to cycle through models while running is not currently working. Launch each model using its own command above.

---

## Part 9 — Auto-Start When the Pi Turns On (Optional)

If you want the skull to start automatically every time the Pi boots up:

**Step 9.1 — Create the autostart file**

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/creepyportrait.desktop
```

**Step 9.2 — Paste this content into the file**

```ini
[Desktop Entry]
Type=Application
Name=Creepy Portrait
Exec=/bin/bash -c 'sleep 8 && cd /home/creepyportrait/openFrameworks/apps/myApps/creepyportrait && MESA_GL_VERSION_OVERRIDE=3.3 GST_V4L2_USE_LIBV4L2=1 ./bin/creepyportrait 0'
X-GNOME-Autostart-enabled=true
```

> **Important:** Replace `creepyportrait` in the path above with whatever username you chose when setting up the Pi. For example if your username is `michael` the path should read `/home/michael/openFrameworks/...`

Save the file by pressing `Ctrl+O` then Enter. Close it by pressing `Ctrl+X`.

**Step 9.3 — Set the Pi to boot to desktop automatically**

```bash
sudo raspi-config
```

Go to: **System Options** → **Boot / Auto Login** → **Desktop Autologin**

Reboot to test:
```bash
sudo reboot
```

---

## Keyboard Controls

When the program is running, press these keys to control it:

| Key | What it does |
|-----|-------------|
| `v` | Show or hide the camera view and face detection box on screen |
| `r` | Make the skull spin around automatically — good for testing without a webcam |
| `w` | Make the skull start wandering immediately without waiting |
| `c` | Snap the skull back to face the center of the screen |
| `e` | Turn eye animation on or off |
| `j` | Open or close the jaw manually |
| `s` | Play a random audio clip immediately |
| `l` | Cycle the LED candle strip through its states (ember, active, fade) |
| `i` | Show or hide the system information overlay — displays date, uptime, IP address, CPU temperature, RAM, current audio clip, LED state, and more |
| `Ctrl+C` | Quit the program |

---

## Tuning the Tracking

You can adjust how the skull behaves by editing a settings file. Open a terminal and type:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
nano src/main.cpp
```

After making any changes, rebuild by typing:
```bash
make -j$(nproc) Release
```

**Face update delay** — how often the Pi looks for faces
- Setting name: `faceUpdateDelay`
- Default value: `0.01` (checks as fast as possible)
- If the skull movement is jerky, increase this to `0.5` or `1.0`

**Camera field of view** — must match your webcam
- Setting name: `videoFOV`
- Logitech C170: set to `58`
- Logitech C270: set to `60`
- Logitech C920: set to `78`
- If wrong, the skull will overshoot or undershoot when tracking faces

**How far to rotate** — how aggressively the skull follows faces
- Setting name: `faceDepth`
- Default: `20.0`
- Increase if the skull barely moves. Decrease if it overshoots wildly.

**Reset delay** — how long before the skull returns to center after losing a face
- Setting name: `noFaceResetSeconds`
- Default: `6.0` seconds
- Increase for a busy room where people move in and out frequently

**Wander delay** — how long with no face before the skull starts wandering
- Setting name: `noFaceWanderSeconds`
- Default: `600.0` seconds (10 minutes)
- Decrease if you want the wandering effect to start sooner

---

## Troubleshooting

**The skull crashes immediately with a platform error**
You are still on Wayland. Follow step 2.4 to switch to X11. Run `echo $XDG_SESSION_TYPE` — it must show `x11`.

**The skull crashes when run over SSH**
SSH sessions have no display. You must run the program from a terminal on the Pi's own desktop — either with a physical monitor or through VNC with a desktop window.

**The skull is completely white with no colours or textures**
You are running through VNC which does not support the hardware graphics the skull needs. Connect a real HDMI monitor.

**The skull does not move when I stand in front of the camera**
Press `v` to show the camera view. If you can see yourself but no green box appears around your face, try moving closer (about 1 metre), improve the room lighting, and face the camera directly. The face detector works best with even lighting on a face looking straight ahead.

**The green box appears on background objects when nobody is there**
The face detector is picking up false positives. Open `src/VideoFaceDetector.cpp` and find `setNeighbors` — increase the value from `5` to `7` or `8` to make detection more strict.

**The skull movement is very choppy**
Increase `faceUpdateDelay` to `1.0` or `2.0` in `src/main.cpp` and rebuild.

**No audio plays when a face is detected**
Check that your WAV files are in the correct folder: `bin/data/audio/`. Press `s` to test — if nothing plays, check your speaker is plugged in and the Pi's volume is turned up. Right-click the speaker icon in the taskbar to check volume.

**No `/dev/video0` after plugging in the webcam**
Type `lsusb` to see if the Pi can see the webcam at all. Try a different USB port. If it still does not appear, test the webcam on another computer.

**The program opens and immediately closes**
Always navigate to the project folder first before running:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
./bin/creepyportrait 0
```

**Warning about `ofGstVideoUtils not loaded` repeating constantly**
This is not an error. It just means the webcam is not plugged in. Plug in the webcam to stop the warnings.

**Compilation fails with out of memory error**
Add more swap space:
```bash
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
```
Find the line that says `CONF_SWAPSIZE=100` and change `100` to `1024`. Save with `Ctrl+O` then Enter, exit with `Ctrl+X`, then:
```bash
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```
Try compiling again.

**`ofGLProgrammableRenderer` compile errors**
Make sure you cloned from the `main` branch:
```bash
git clone https://github.com/maserowik/creepyportrait creepyportrait
```

---

## Webcam Reference

| Camera | Field of View | Focus | Low Light | Best For |
|--------|--------------|-------|-----------|---------|
| Logitech C170 | 58° | Fixed | Basic | Testing only |
| Logitech C270 | 60° | Fixed | Good | General use |
| Logitech C920 | 78° | Auto | Excellent | Permanent wall mount |

Set `videoFOV` in `src/main.cpp` to match your camera's value from the table above.

---

## Hardware Compatibility

| Pi Model | Works? | Notes |
|----------|--------|-------|
| Pi 4 (2GB+) | ✅ Yes | Recommended |
| Pi 5 | ✅ Yes | Faster, better tracking |
| Pi 400 | ✅ Yes | Same chip as Pi 4 |
| Pi 3B / 3B+ | ❌ No | Wrong graphics hardware |
| Pi Zero 2W | ❌ No | Wrong graphics hardware, not enough RAM |
| Pi 1 / 2 / Zero | ❌ No | Too slow |

**If your Pi is not in the Yes column above, stop — it will not work.**

---

*Original project by Tony DiCola / Adafruit (2013). Modernized for openFrameworks 0.12+ on Raspberry Pi 4/5 by @maserowik.*

---
---

# Appendix A — Adding an LED Candle Strip

This appendix covers adding a WS2812B LED strip behind your display frame to create an atmospheric candlelight glow that reacts to the skull's state. This is completely optional — the skull works perfectly without it.

When the LED strip is running it produces an organic flickering candle effect. When someone stands in front of the skull the lights brighten and become more active. When they walk away the lights slowly fade back to a dim ember glow. The effect is controlled by a small Python program that runs alongside the skull.

---

## What You Need for the LED Strip

| Item | Notes |
|------|-------|
| WS2812B LED strip | Get the 30 LED per metre density. You need enough to go around the inside of your frame. 1 metre is usually enough for a picture frame sized display. Look for "WS2812B 30 LED/m" — this exact model is required, other types will not work with this guide. |
| 5V 3A power supply | Must be 5 volts, 3 amps (written as 5V 3A). This is separate from the Pi's power supply. Get one with bare wire leads or screw terminals, not a USB plug. |
| Three short wires | About 20cm each. Any flexible wire works — old USB cable wire, doorbell wire, or hookup wire from a hardware store. You need three different colours to avoid mixing them up. |
| Small screwdriver | For the screw terminals on the LED strip if it has them, or for stripping wire ends. |

---

## Understanding the LED Strip

Before wiring anything, look at your LED strip. At one end you will see three connections labelled:

- **5V** — power (positive)
- **GND** — ground (negative)  
- **DIN** — data input (the signal wire)

> **Important — direction matters:** The strip only works in one direction. Look for a small arrow printed on the strip. The arrow points away from the end you connect to. Always connect to the end where the arrow is pointing away from you.

The strip also has a cut line marked every few LEDs — you can cut it to length there with scissors if needed.

---

## Wiring

You are making three connections. Do this with the Pi and the power supply both switched off.

**Connection 1 — Power to the strip:**
Connect the positive wire from your 5V power supply to the **5V** pad on the LED strip.

**Connection 2 — Ground:**
This connection has two parts. Connect the negative wire from your 5V power supply to the **GND** pad on the LED strip. Then run a second wire from that same GND pad on the strip to **Pin 6** on the Raspberry Pi (this is a Ground pin — see the diagram below).

> **Why two ground wires to GND?** The Pi needs to share a common ground with the LED strip or the signal wire will not work correctly. This is the most important part of the wiring.

**Connection 3 — Data signal:**
Run a wire from the **DIN** pad on the LED strip to **Pin 12** on the Raspberry Pi. Pin 12 is also called GPIO 18.

**Raspberry Pi Pin Layout (the connector on the Pi with 40 pins):**

Looking at the Pi with the USB ports facing away from you, the pins are arranged in two rows. Count from the top-left corner:

```
Pin 1  (3.3V)    Pin 2  (5V)
Pin 3  (GPIO 2)  Pin 4  (5V)
Pin 5  (GPIO 3)  Pin 6  (GND)   ← connect strip GND here
Pin 7  (GPIO 4)  Pin 8  (GPIO 14)
Pin 9  (GND)     Pin 10 (GPIO 15)
Pin 11 (GPIO 17) Pin 12 (GPIO 18) ← connect strip DIN here
```

Pin 6 is in the right column, third from the top. Pin 12 is in the right column, sixth from the top.

> **Double-check before powering on:** Wrong wiring can damage the Pi or the LED strip. Before switching anything on, confirm: 5V power supply positive → strip 5V, 5V power supply negative → strip GND, strip GND → Pi Pin 6, strip DIN → Pi Pin 12.

---

## Allow the Pi to Control the LEDs

The Pi needs permission to use the GPIO pin for LED control. Run these two commands:

```bash
sudo usermod -a -G gpio $USER
echo 'SUBSYSTEM=="gpio", GROUP="gpio", MODE="0660"' | sudo tee /etc/udev/rules.d/99-gpio.rules
```

Then reboot:
```bash
sudo reboot
```

---

## Set Up the LED Sidecar Program

The LED strip is controlled by a small Python program called `led_candle.py` that runs alongside the skull. It reads a small text file that the skull writes to, and changes the LED behaviour based on what the skull is doing.

**Step A.1 — Check the sidecar file is there**

```bash
ls ~/openFrameworks/apps/myApps/creepyportrait/led_candle.py
```

You should see the filename printed back. If you see "No such file or directory" then the file did not come down with the repository — contact the project maintainer.

**Step A.2 — Test the LED strip**

With the LED strip wired up and the 5V power supply switched on, run:
```bash
cd ~/openFrameworks/apps/myApps/creepyportrait
sudo python3 led_candle.py
```

You should see the LEDs start flickering like candle flame within a few seconds. Press `Ctrl+C` to stop.

If the LEDs do not light up, check your wiring carefully — go back through the wiring section above and confirm every connection.

**Step A.3 — Allow the program to run without sudo**

Normally the LED program needs to run as administrator (sudo). This command allows it to run normally:
```bash
sudo bash -c 'echo "creepyportrait ALL=(ALL) NOPASSWD: /usr/bin/python3 /home/creepyportrait/openFrameworks/apps/myApps/creepyportrait/led_candle.py" >> /etc/sudoers'
```

> **Important:** Replace both instances of `creepyportrait` in that command with your actual username if it is different.

---

## Add the LED Strip to Auto-Start

If you set up the skull to auto-start in Part 9, you need to create a second autostart file for the LED program. The skull and the LED sidecar each have their own separate autostart file. Do not replace the skull's file — create a new one alongside it.

Make sure the skull autostart file from Part 9 is already in place. If not, do Part 9 first.

Create a second autostart file for the LED sidecar:
```bash
nano ~/.config/autostart/ledcandle.desktop
```

Paste this content:
```ini
[Desktop Entry]
Type=Application
Name=LED Candle
Exec=/bin/bash -c 'sleep 12 && sudo python3 /home/creepyportrait/openFrameworks/apps/myApps/creepyportrait/led_candle.py --loglevel warning'
X-GNOME-Autostart-enabled=true
```

> **Important:** Replace `creepyportrait` in the path with your actual username.

> **Why sleep 12?** The LED sidecar waits 12 seconds on boot — 4 seconds longer than the skull — to make sure the skull has fully started and created its state files before the sidecar tries to read them.

Save with `Ctrl+O` then Enter. Exit with `Ctrl+X`. Reboot to test:
```bash
sudo reboot
```

Both the skull and the LED strip should now start automatically when the Pi powers on.

---

## LED Candle States

The LED strip has three states that the skull switches between automatically:

| State | What it looks like | When it happens |
|-------|--------------------|-----------------|
| Ember | Very dim, slow deep orange glow with occasional flicker | Nobody has been detected for a while |
| Active | Bright, lively candle flame with wind gusts and shimmer | A face has just been detected |
| Fade | Gradually dimming from active back down to ember | A few seconds after the face disappears |

Press `l` while the skull is running to cycle through the states manually for testing.

The `i` key overlay shows the current LED state in the top-right corner of the screen so you can confirm everything is communicating correctly.

---

## LED Troubleshooting

**LEDs do not light up at all**
Check that the 5V power supply is switched on. Check the wiring — confirm 5V to strip 5V, ground to strip GND, Pi Pin 6 to strip GND, Pi Pin 12 to strip DIN. Make sure you are connecting to the correct end of the strip (the end where the arrow points away from you).

**Only the first few LEDs light up, the rest stay dark**
The power supply may not be providing enough current. Make sure you are using a 5V 3A supply, not a lower-rated one.

**LEDs light up but show wrong colours or flicker erratically**
The ground connection between the Pi and the strip is missing or loose. Check that you have a wire running from the strip GND to Pi Pin 6.

**`sudo python3 led_candle.py` shows a permission error**
Run the usermod command from Step A.3 again and reboot.

**The skull runs but the LEDs stay in ember state and never react**
The skull and LED program communicate through a file called `led_state.txt` in the `bin/data/` folder. Check that the skull is actually running — press `i` to show the overlay and look for the Candle line. If it says `not running` the LED sidecar is not running. If it says `stale` the sidecar crashed — restart it with `sudo python3 led_candle.py` in a second terminal.

---

*Original project by Tony DiCola / Adafruit (2013). Modernized for openFrameworks 0.12+ on Raspberry Pi 4/5 by @maserowik.*
