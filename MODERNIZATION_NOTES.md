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

**What changed:**

All deprecated openFrameworks calls were replaced with their modern equivalents throughout `CreepyPortrait.cpp` and `Model.cpp`. The shader files were rewritten for GLSL 1.50 (OpenGL 3.2 core profile) which is what the Pi 4 and Pi 5 support via Mesa.

The face detection settings were tuned for better real-world performance:
- `setNeighbors` increased from 1 to 5 — dramatically reduces false positives where background objects get detected as faces
- `setMinAreaRadius` set to 30 — filters out very small detections that are rarely real faces
- Video resolution set to 320x240 — the right balance between detection quality and CPU load on Pi 4

The fullscreen mode was fixed — the original code used a deprecated window mode that did not work on modern Pi OS.

---

## Phase 2 — Jaw Animation

**What changed:** `Model.cpp`, `Model.h`

A `jawAngle` float variable was added to the Model class. In the draw loop, mesh index 1 (the lower jaw) is now transformed separately from the rest of the skull — it translates down to the jaw hinge point, rotates on the X axis by `jawAngle` degrees, then translates back. This gives a realistic opening and closing motion.

The jaw transform was added to both draw paths in Model.cpp — the VBO path (used when normal mapping is enabled) and the mesh fallback path. Both must always be kept in sync.

---

## Phase 3 — Bloodshot Eyes

**What changed:** `Model.cpp`, `Model.h`, `eye.vert`, `eye.frag`

Eye socket coordinates were determined by examining the skull mesh geometry. Two flat quad meshes (simple rectangles) are placed directly at the eye socket positions inside the 3D scene. They use a separate eye shader and an `eyes.png` texture showing a bloodshot eyeball.

Because the eye quads are placed inside the skull's transformation matrix, they automatically rotate with the skull when it tracks faces. This gives the correct behaviour — the eyes always point in the same direction as the skull.

The eye shader supports two uniforms that can be driven from the main program:
- `pupilScale` — scales the eye texture, used for pulsing and blinking effects
- `pupilOffset` — shifts the eye texture UV coordinates, used for eye movement effects
- `blinkProgress` — controls the eyelid position for realistic blinking

---

## Phase 4 — Center Key

**What changed:** `CreepyPortrait.cpp`

The `c` key was added. Pressing it immediately sets both `targetRotation` and `currentRotation` to zero, snapping the skull to face forward. It also resets `faceLastSeen` to the current time, which exits wander mode if it was active.

---

## Phase 5 — Idle Wander

**What changed:** `CreepyPortrait.cpp`, `main.cpp`

After `noFaceWanderSeconds` (default 10 minutes) with no face detected, the skull enters wander mode. Two overlapping sine waves at different frequencies and phases drive `currentRotation.x` and `currentRotation.y` independently, producing an organic-looking figure-8 search pattern.

The `w` key was added to force wander mode immediately for testing — it sets `faceLastSeen` to a large negative value so the condition fires on the next frame.

When a face is detected while in wander mode, the skull snaps instantly to center before beginning to track.

---

## Phase 6 — Audio and Jaw Sync

**What changed:** `CreepyPortrait.cpp`

An audio state machine was added with three states: IDLE, TRIGGERED, and PLAYING. When a face is detected and the state is IDLE, the audio clip plays. When the face is lost, the audio stops.

Jaw movement is driven by the audio spectrum. `ofSoundGetSpectrum(64)` returns the frequency content of the currently playing audio. The average amplitude across all bands is mapped to a target jaw angle. A lerp smooths the movement so the jaw does not snap around too aggressively.

The `j` key toggles the jaw open and closed independently of audio, for testing. The `s` key plays the audio clip independently of face detection.

The audio backend was configured to use PipeWire (via `alsoftrc`) for compatibility with modern Pi OS which removed PulseAudio in favour of PipeWire.

---

## Phase 7 — Eye Animation

**What changed:** `CreepyPortrait.cpp`, `Model.cpp`, `eye.frag`

Four eye animation effects were implemented, all driven from `updateCurrentRotation()` in `CreepyPortrait.cpp`. The `e` key toggles all eye animation on and off.

**Pulsate:** A continuous sine wave drives `pupilScale` between approximately 0.74 and 0.90, giving the eyes a slow living throb. Because the eye shader scales UV coordinates inversely, higher `pupilScale` values make the eye appear smaller — this was counterintuitive during development and required experimentation to get right.

**Blink:** A three-phase blink cycle controlled by `dilationTimer`, `twitching`, `twitchTimer`, and `twitchDuration`. When the timer fires (randomly every 2 to 8 seconds), the blink sequence begins. Phase 1 closes: `twitchAmount` ramps from 0 to 1 over 35% of the blink duration. Phase 2 holds: `twitchAmount` stays at 1 for 30% of the duration. Phase 3 opens: `twitchAmount` ramps back from 1 to 0. The `blinkProgress` uniform in the eye shader receives `twitchAmount` and draws a dark band sweeping from the top of the eye downward as the value increases. The shader uses `smoothstep` to soften the eyelid edge.

**Microsaccade:** Every 0.5 to 2 seconds, a small random offset is added to `microsaccadeOffset`. The offset decays back toward zero each frame via `glm::mix`. This creates tiny involuntary-looking movements that make the eyes appear alive even when nothing dramatic is happening. The values are kept in UV coordinate space at `±0.02` maximum to avoid pushing the texture outside its valid range.

**Dart:** Every 3 to 9 seconds, `dartOffset` snaps to a random position (`±0.08` UV units), holds there for 0.3 to 0.7 seconds, then lerps back to zero. The dart offset is combined with `microsaccadeOffset` and the total is passed as `pupilOffset` to the eye shader.

**Critical lesson learned during development:** `pupilOffset` is a UV-space value. Passing world-space coordinates into it causes the UV to go completely out of range and produces vivid stripe artifacts. All offsets must be in UV space (small values, typically `±0.05` or less). This was the root cause of several failed attempts during development.

**Another critical lesson:** The eye quads must remain inside the skull's transformation matrix. Moving them outside causes them to no longer rotate with the skull. The eyelid shader approach (`blinkProgress`) was chosen specifically because it does not require any geometry movement — everything happens in the fragment shader.

---

## What Has Not Changed

The face detection algorithm itself — a Haar cascade classifier — is the same algorithm from 2013. It works well in good lighting with faces looking directly at the camera, but struggles in low light and with faces at angles. A future improvement (Phase 16 in the roadmap) would replace it with a modern DNN-based detector for better real-world performance.

The 3D models are the originals from the Video Copilot Halloween pack. They have not been modified.

---

## Known Issues and Limitations

- The `m` key to cycle between models does not work when launched with the `all` argument. Launch each model directly as a command line argument instead.
- The Pi camera is not supported on modern Pi OS (Bullseye 2021 and later). Always use a USB webcam.
- Running over VNC produces a white skull with no textures because VNC software rendering does not support the hardware OpenGL shaders. Use a physical HDMI monitor.
- `TARGET_RASPBERRY_PI` is not defined by the build system on modern Pi OS, so the Pi always uses the desktop configuration block in `main.cpp`. The Pi-specific config block exists but is never activated. This is harmless — the desktop config works correctly on Pi.

---

## File Reference

| File | Purpose |
|------|---------|
| `src/CreepyPortrait.cpp` | Main application logic — face tracking, rotation, all animation state machines |
| `src/CreepyPortrait.h` | Class definition and all state variables |
| `src/Model.cpp` | 3D model loading and rendering, jaw transform, eye quad drawing |
| `src/Model.h` | Model class definition |
| `src/VideoFaceDetector.cpp` | Haar cascade face detection wrapper |
| `src/VideoSource.cpp` | Webcam and Pi camera abstraction |
| `src/main.cpp` | Entry point, configuration, window setup |
| `bin/data/eye.vert` | Eye quad vertex shader |
| `bin/data/eye.frag` | Eye quad fragment shader — handles pulsate, blink eyelid, pupil offset |
| `bin/data/lighting_gl_bump.frag` | Skull fragment shader with normal mapping (desktop/Pi 4+) |
| `bin/data/lighting_gl_bump.vert` | Skull vertex shader with normal mapping |
| `bin/data/lighting_es.frag` | Skull fragment shader without normal mapping (Pi fallback) |
| `bin/data/lighting_es.vert` | Skull vertex shader without normal mapping |

---

*Original project by Tony DiCola / Adafruit (2013). Modernized by @maserowik.*
