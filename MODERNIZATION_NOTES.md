# Creepy Portrait — Modernization Notes

This document describes all the changes made to bring the original 2013 Creepy Portrait project up to date for modern hardware and software, and the new features added during the modernization process.

The original project was created by Tony DiCola for Adafruit in 2013. It was written for a much older version of openFrameworks and ran on the original Raspberry Pi. A lot has changed in the decade since — the Pi hardware, the operating system, the graphics libraries, and the C++ language itself have all moved on. This document records what was changed and why.

---

## Why the Original Code Stopped Working

The original code used several parts of openFrameworks that were deprecated and eventually removed in version 0.12. It also assumed a 32-bit ARM processor (the original Pi used ARMv6) and an OpenGL ES graphics environment. Modern Pi hardware runs 64-bit ARM and supports full desktop OpenGL, which is actually better — but requires different shader code and different library calls.

Specific things that had broken by 2024:

- `ofVec2f` and `ofVec3f` — the old vector math types were replaced by `glm::vec2` and `glm::vec3` from the GLM library
- `ofRotateX()`, `ofRotateY()` — deprecated, replaced by `ofRotateDeg()` with an axis parameter
- `loadImage()` — replaced by `load()`
- `ofRect()` — replaced by `ofDrawRectangle()`
- `ofVec2f::getInterpolated()` — replaced by `glm::mix()`
- The shader files used GLSL 1.20 syntax which is not accepted by modern Mesa drivers on Pi
- The Raspberry Pi camera interface changed completely in Bullseye 2021 — the old `raspicam` approach no longer works
- The openFrameworks dependency installer script references packages that have been renamed or removed in Debian Trixie

---

## Phase 1 — Core Modernization and Tracking Improvements

**What changed:** `CreepyPortrait.cpp`, `Model.cpp`, shader files

All deprecated openFrameworks calls were replaced with their modern equivalents throughout `CreepyPortrait.cpp` and `Model.cpp`. The shader files were rewritten for GLSL 1.50 (OpenGL 3.2 core profile) which is what the Pi 4 and Pi 5 support via Mesa.

The face detection settings were tuned for better real-world performance:
- `setNeighbors` increased from 1 to 5 — dramatically reduces false positives where background objects get detected as faces
- `setMinAreaRadius` set to 30 — filters out very small detections that are rarely real faces
- Video resolution set to 320x240 — the right balance between detection quality and CPU load on Pi 4

The fullscreen mode was fixed — the original code used a deprecated window mode that did not work on modern Pi OS.

---

## Phase 2 — Jaw Animation

**What changed:** `Model.cpp`, `Model.h`

A `jawAngle` float variable was added to the Model class. In the draw loop, mesh index 1 (the lower jaw) is now transformed separately from the rest of the skull — it translates down to the jaw hinge point at Y=−70, rotates on the X axis by `jawAngle` degrees, then translates back. This gives a realistic opening and closing motion.

The jaw transform was added to both draw paths in Model.cpp — the VBO path (used when normal mapping is enabled, which is always) and the mesh fallback path. Both must always be kept in sync or changes to one path will silently have no effect.

The skull is scaled to 0.85 (85%) to prevent the jaw from visually clipping through the bottom of the display frame at maximum open angle.

---

## Phase 3 — Bloodshot Eyes

**What changed:** `Model.cpp`, `Model.h`, `eye.vert`, `eye.frag`

Eye socket coordinates were determined by examining the skull mesh geometry. Two flat quad meshes (simple rectangles) are placed directly at the eye socket positions inside the 3D scene at `ofTranslate(-68,24,175)` (left) and `ofTranslate(82,20,175)` (right). They use a separate eye shader and an `eyes.png` texture showing a bloodshot eyeball.

Because the eye quads are placed inside the skull's transformation matrix, they automatically rotate with the skull when it tracks faces. This gives the correct behaviour — the eyes always point in the same direction as the skull.

The eye shader supports two uniforms that can be driven from the main program:
- `pupilScale` — scales the eye texture, used for pulsing and blinking effects
- `pupilOffset` — shifts the eye texture UV coordinates, used for eye movement effects
- `blinkProgress` — controls the eyelid position for realistic blinking

---

## Phase 4 — Center Key

**What changed:** `CreepyPortrait.cpp`

The `c` key was added. Pressing it immediately sets both `targetRotation` and `currentRotation` to zero, snapping the skull to face forward. It also resets `faceLastSeen` to the current time and sets `rotateSkull` to false, which exits wander mode if it was active.

---

## Phase 5 — Idle Wander

**What changed:** `CreepyPortrait.cpp`, `main.cpp`

After `noFaceWanderSeconds` (default 10 minutes) with no face detected, the skull enters wander mode. Two overlapping sine waves at different frequencies and phases drive `currentRotation.x` and `currentRotation.y` independently, producing an organic-looking figure-8 search pattern.

The `w` key was added to force wander mode immediately for testing — it sets `faceLastSeen` to a large negative value so the condition fires on the next frame.

When a face is detected while in wander mode, the skull snaps instantly to center before beginning to track.

---

## Phase 6 — Snap To Center

**What changed:** `CreepyPortrait.cpp`

When a face is detected during wander mode, both `targetRotation` and `currentRotation` are set to zero instantly before resuming tracking. This prevents the skull from sweeping across the screen from its wander position when a face suddenly appears.

---

## Phase 7 — Eye Animation

**What changed:** `CreepyPortrait.cpp`, `Model.cpp`, `eye.frag`

Four eye animation effects were implemented, all driven from `updateCurrentRotation()` in `CreepyPortrait.cpp`. All per-frame eye logic lives in `updateCurrentRotation()` — not in `update()`, which only calls `video->update()`. The `e` key toggles all eye animation on and off.

**Pulsate:** A continuous sine wave drives `pupilScale` between approximately 0.74 and 0.90, giving the eyes a slow living throb. Because the eye shader scales UV coordinates inversely, higher `pupilScale` values make the eye appear smaller — this was counterintuitive during development and required experimentation to get right.

**Blink:** A three-phase blink cycle controlled by `dilationTimer`, `twitching`, `twitchTimer`, and `twitchDuration`. When the timer fires (randomly every 2 to 8 seconds), the blink sequence begins. Phase 1 closes: `twitchAmount` ramps from 0 to 1 over 35% of the blink duration. Phase 2 holds: `twitchAmount` stays at 1 for 30% of the duration. Phase 3 opens: `twitchAmount` ramps back from 1 to 0. The `blinkProgress` uniform in the eye shader receives `twitchAmount` and draws a dark band sweeping from the top of the eye downward as the value increases. The shader uses `smoothstep` to soften the eyelid edge.

**Microsaccade:** Every 0.5 to 2 seconds, a small random offset is added to `microsaccadeOffset`. The offset decays back toward zero each frame via `glm::mix`. This creates tiny involuntary-looking movements that make the eyes appear alive even when nothing dramatic is happening. The values are kept in UV coordinate space at `±0.02` maximum to avoid pushing the texture outside its valid range.

**Dart:** Every 3 to 9 seconds, `dartOffset` snaps to a random position (`±0.08` UV units), holds there for 0.3 to 0.7 seconds, then lerps back to zero. The dart offset is combined with `microsaccadeOffset` and the total is passed as `pupilOffset` to the eye shader.

**Critical lesson learned during development:** `pupilOffset` is a UV-space value. Passing world-space coordinates into it causes the UV to go completely out of range and produces vivid stripe artifacts. All offsets must be in UV space (small values, typically `±0.05` or less).

**Another critical lesson:** The eye quads must remain inside the skull's transformation matrix. Moving them outside causes them to no longer rotate with the skull. The eyelid shader approach (`blinkProgress`) was chosen specifically because it does not require any geometry movement — everything happens in the fragment shader.

---

## Phase 8 — Float Drift

**What changed:** `CreepyPortrait.cpp`, `CreepyPortrait.h`

During idle wander mode, the skull now drifts its screen position as well as rotating. Two overlapping sine waves at different frequencies drive `wanderDrift.x` and `wanderDrift.y` independently, producing slow unpredictable movement across the screen. The frequencies and phases are chosen to avoid any obvious repeating pattern.

The drift is applied in `draw()` as an `ofTranslate(wanderDrift.x, wanderDrift.y, 0)` inside the camera transform block. When wander mode ends, `wanderDrift` lerps back toward zero using `glm::mix` with a factor of `0.05f` for a smooth return rather than a snap.

The drift amplitudes (`±180` pixels on X, `±100` pixels on Y) were chosen to keep the skull clearly on screen at typical display resolutions while still making the movement feel significant.

---

## Phase 10 — Multiple Random WAV Files

**What changed:** `CreepyPortrait.cpp`, `CreepyPortrait.h`

The single hardcoded audio clip was replaced with a dynamic system. At startup, `ofDirectory` scans `bin/data/audio/` for all `.wav` files and stores them in `audioClips`. The directory is sorted alphabetically for consistent ordering. If no files are found the audio system is silently disabled.

Three audio trigger contexts were implemented:
- **Face detected:** A random clip plays immediately when a face first appears
- **Face present replay:** While a face remains, another random clip plays every 8 to 20 seconds (randomised each time via `audioRepeatDelay`)
- **Wander:** A random clip plays every 15 to 45 seconds while the skull is wandering with no face detected (randomised each time via `audioWanderTimer`)

The `s` key plays a random clip immediately regardless of state, for testing.

A `currentClipName` member variable (type `std::string`, default `"none"`) tracks the filename of the most recently loaded clip. It is updated at every `soundPlayer.load()` call using `ofFilePath::getFileName()` to store just the filename without the full path.

---

## Phase 11 — Audio and Jaw Sync

**What changed:** `CreepyPortrait.cpp`

Jaw movement is driven by the audio spectrum. `ofSoundGetSpectrum(64)` returns the frequency content of the currently playing audio each frame. The average amplitude across all 64 bands is computed, smoothed via `ofLerp` with a factor of `0.15f`, then mapped to a target jaw angle using `ofMap` with a threshold of `0.03f`. A second lerp with factor `0.08f` smooths the jaw movement itself so it does not snap aggressively.

The jaw sensitivity (`ofMap` upper threshold of `0.03f`) was tuned after the final audio content was confirmed. If audio content changes significantly, this value may need retuning — louder or quieter clips will change where the jaw sits at rest and at peak.

---

## Phase 13 — LED Candle Lighting

**What changed:** `led_candle.py` (new file), `CreepyPortrait.cpp`

A WS2812B 30-LED strip on GPIO 18 provides atmospheric candlelight behind the display frame. Because the LED strip requires root access to the GPIO hardware, it is controlled by a separate Python sidecar process (`led_candle.py`) rather than from within the C++ application. The two processes communicate through two small text files in `bin/data/`:

- `led_state.txt` — written by the C++ app, read by the Python sidecar. Contains the current state string: `ember`, `active`, or `fade`.
- `led_audio.txt` — written by the C++ app to signal audio amplitude to the sidecar for real-time LED reaction.

**LED states:**
- `ember` — very dim, slow deep orange glow with occasional organic flicker. The default state when no face has been detected for a while.
- `active` — bright lively candle flame with wind gust effects, heat shimmer, and blue tint at the base corners. Triggered when a face is detected.
- `fade` — gradually dims from active back down to ember over 5 seconds. Triggered a few seconds after the face disappears.

The sidecar implements several organic lighting behaviours: base flicker, heat shimmer, wind gust, first light startup sequence, and a nearly-dying ember effect. All are driven by sine waves and random perturbations to avoid mechanical-looking repetition.

The `l` key cycles the LED state manually through ember → active → fade → ember by writing to `led_state.txt` directly from the C++ key handler. This allows testing without needing a face in front of the camera.

Logrotate, sudoers, and autostart configuration for the sidecar are covered in the install guide.

---

## Phase 14 — Logging to File

**What changed:** `CreepyPortrait.cpp`, `main.cpp`

`ofLogToFile()` was added to write all log output to `bin/data/logs/creepyportrait.log`. The default log level is `OF_LOG_WARNING` — informational and debug messages are suppressed unless explicitly requested. A `--loglevel` command line argument was added accepting `debug`, `info`, `warning`, or `error`.

Logrotate is configured at `/etc/logrotate.d/creepyportrait` with daily rotation, 5 rotations kept, compression enabled, and `copytruncate` so the running application does not need to be restarted when the log rolls over.

Live log monitoring:
```bash
tail -f ~/openFrameworks/apps/myApps/creepyportrait/bin/data/logs/creepyportrait.log
```

---

## Phase 20 — System Info Overlay

**What changed:** `CreepyPortrait.cpp`, `CreepyPortrait.h`

The `i` key toggles a fullscreen information overlay drawn directly in OpenGL using raw `glOrtho` + `glRecti` calls. This bypasses the openFrameworks blend state dependency that caused rendering artifacts when using `ofDrawRectangle` for the background panel.

The overlay displays: date/time, app uptime, system uptime and boot time, hostname, IP addresses for all active non-loopback interfaces, MAC addresses, kernel version, CPU model, CPU frequency, CPU temperature (in both Celsius and Fahrenheit), RAM usage, disk usage, FPS, current model name, skull rotation angles, and face detection state.

Two app-specific fields were added at the bottom of the overlay:

**Candle state:** Reads `led_state.txt` live each frame using `stat()` to check the file's modification time. If the file does not exist, displays `not running`. If the file has not been modified in more than 10 seconds (indicating the Python sidecar has crashed or stopped), appends `(stale)` to the state string. This gives immediate visual feedback on the health of the LED sidecar without needing a separate terminal window.

**WAV:** Shows the filename of the currently playing or last played audio clip using `currentClipName`. If the sound player is currently playing, the filename is shown as-is. If nothing is playing and a clip has been played before, it is shown as `last: filename.wav`. If no clip has ever played since startup, shows `none`.

---

## Key Architecture Lessons

These are the lessons that caused the most time lost during development and are worth preserving explicitly:

**Dual draw path in Model.cpp:** There are two draw paths — the VBO path (when `useNormalMapping=true`, which is always) and the mesh fallback path. Any per-mesh transform like the jaw rotation must be added to both paths or it will silently have no effect. `useNormalMapping` is set to `true` in `main.cpp` and never changes, so the VBO path is always the active one — but the fallback must be kept in sync regardless.

**Frame logic lives in `updateCurrentRotation()`:** All per-frame animation logic (jaw lerp, rotation updates, eye animation) must go in `updateCurrentRotation()`. The `update()` function only calls `video->update()`. Putting animation logic in `update()` causes it to run before the video frame is processed and produces incorrect behaviour.

**Patching with Python3 only:** When patching source files, always use `python3` with binary-safe file handling and exact byte strings. Never use shell heredoc scripts for patches — they mangle special characters. Always confirm exact whitespace with `cat -A` before writing any patch. The codebase uses tabs for indentation throughout.

**UV space for eye offsets:** All values passed to `pupilOffset` in the eye shader must be in UV coordinate space (typically `±0.05` or less). Passing world-space values causes the texture UV to go wildly out of range and produces vivid stripe artifacts across the eyes.

---

## What Has Not Changed

The face detection algorithm itself — a Haar cascade classifier — is the same algorithm from 2013. It works well in good lighting with faces looking directly at the camera, but struggles in low light and with faces at angles. A future improvement (Phase 16 in the roadmap) would replace it with a modern DNN-based detector for better real-world performance.

The 3D models are the originals from the Video Copilot Halloween pack. They have not been modified.

---

## Known Issues and Limitations

- The `m` key to cycle between models does not work when launched with the `all` argument. Launch each model directly using its own command line argument instead.
- Jaw animation does not work on the pumpkin models. The pumpkin meshes consist of a body (mesh_1, 12936 vertices) and a lid (mesh_2, 7879 vertices) — the carved mouth is cut into the body mesh and is not a separate piece. True jaw animation would require creating a new mouth mesh in 3D modelling software (e.g. Blender) and exporting it as a separate PLY file. For now the pumpkins track faces and animate eyes but the jaw stays static.
- The Pi camera is not supported on modern Pi OS (Bullseye 2021 and later). Always use a USB webcam.
- Running over VNC produces a white skull with no textures because VNC software rendering does not support the hardware OpenGL shaders. Use a physical HDMI monitor.
- `TARGET_RASPBERRY_PI` is not defined by the build system on modern Pi OS, so the Pi always uses the desktop configuration block in `main.cpp`. The Pi-specific config block exists but is never activated. This is harmless — the desktop config works correctly on Pi.
- The LED sidecar (`led_candle.py`) requires root access to the GPIO hardware and must be launched with `sudo`. The `i` overlay will show `stale` if the sidecar stops running — this is the intended way to detect a sidecar crash without opening a separate terminal.

---

## File Reference

| File | Purpose |
|------|---------|
| `src/CreepyPortrait.cpp` | Main application logic — face tracking, rotation, all animation state machines, key handlers, system info overlay |
| `src/CreepyPortrait.h` | Class definition and all state variables |
| `src/Model.cpp` | 3D model loading and rendering, jaw transform, eye quad drawing |
| `src/Model.h` | Model class definition including `jawAngle` |
| `src/VideoFaceDetector.cpp` | Haar cascade face detection wrapper |
| `src/VideoSource.cpp` | Webcam and Pi camera abstraction |
| `src/main.cpp` | Entry point, configuration, window setup |
| `bin/data/eye.vert` | Eye quad vertex shader |
| `bin/data/eye.frag` | Eye quad fragment shader — handles pulsate, blink eyelid, pupil offset |
| `bin/data/lighting_gl_bump.frag` | Skull fragment shader with normal mapping (desktop/Pi 4+) |
| `bin/data/lighting_gl_bump.vert` | Skull vertex shader with normal mapping |
| `bin/data/lighting_es.frag` | Skull fragment shader without normal mapping (Pi fallback) |
| `bin/data/lighting_es.vert` | Skull vertex shader without normal mapping |
| `bin/data/audio/` | Directory for WAV audio clips — place any number of `.wav` files here |
| `bin/data/logs/creepyportrait.log` | Application log file — monitored via `tail -f` |
| `bin/data/led_state.txt` | IPC file — C++ app writes LED state, Python sidecar reads it |
| `bin/data/led_audio.txt` | IPC file — C++ app writes audio amplitude, Python sidecar reads it |
| `led_candle.py` | Python sidecar — controls WS2812B LED strip on GPIO 18 |

---

*Original project by Tony DiCola / Adafruit (2013). Modernized by @maserowik.*
