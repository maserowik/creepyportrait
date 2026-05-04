# Creepy Portrait — Modernization Notes

Original code by Tony DiCola, copyright 2013. This document describes changes
made to bring the project up to date with **openFrameworks 0.12+** on modern
hardware (Linux/macOS/Windows desktop + Raspberry Pi 4/5).

---

## Summary of Changes

### src/VideoSource.cpp

| Old (OF 0.8) | New (OF 0.12+) | Reason |
|---|---|---|
| `video.initGrabber(w, h, true)` | `video.initGrabber(w, h)` | 3rd bool param removed |
| `video.getPixelsRef()` | `video.getPixels()` | `getPixelsRef()` deprecated |
| `video.width` / `video.height` | `video.getWidth()` / `video.getHeight()` | Public fields removed |

### src/Model.h / src/Model.cpp

| Old | New | Reason |
|---|---|---|
| `image.loadImage(name)` | `image.load(name)` | `loadImage()` deprecated |
| `ofRotateY(angle)` | `ofRotateDeg(angle, 0, 1, 0)` | `ofRotateY()` deprecated |
| `ofVec3f` | `glm::vec3` | OF switched to GLM math library |
| `vec.dot(other)` | `glm::dot(vec, other)` | GLM free-function style |
| `vec.cross(other)` | `glm::cross(vec, other)` | GLM free-function style |
| `vec.normalize()` | `glm::normalize(vec)` | GLM free-function style |
| VBO created as local in `draw()` every frame | `std::vector<ofVbo> vbos` member built once in constructor; tangent attribute location cached in `cachedTangentLoc` and uploaded to VBO on first `draw()` only | All GPU data (mesh geometry + tangents) now lives permanently on the GPU; `draw()` calls only `drawElements()` after the first frame |
| `ambientTex` unbound in non-bump (`lighting_gl`) path | `diffuse` bound to unit 3 as fallback | Avoids undefined sampler read if `useNormalMapping` is ever set false |

### src/main.cpp

| Old | New | Reason |
|---|---|---|
| Missing `#include <iomanip>` | Added | Required for `std::setw` and `std::setiosflags`; was compiling by accident via GCC's transitive include of `<iostream>`, which is not guaranteed by the standard and fails on Clang/libc++ |

### src/CreepyPortrait.h / .cpp

| Old | New | Reason |
|---|---|---|
| `image.loadImage(name)` | `image.load(name)` | Deprecated |
| `ofRotateX/Y(angle)` | `ofRotateDeg(angle, x, y, z)` | Deprecated |
| `ofRect(...)` | `ofDrawRectangle(...)` | Deprecated |
| `ofVec2f` / `ofVec3f` | `glm::vec2` / `glm::vec3` | GLM migration |
| `vec.getInterpolated(b, t)` | `glm::mix(a, b, t)` | GLM equivalent |
| `camera.getModelViewMatrix() * ofVec4f` | Cast to `glm::vec4` first | Matrix * vec now needs GLM types |
| `ofVec2f videoOffset` | `glm::vec2 videoOffset` | GLM migration |

### src/VideoFaceDetector.cpp

| Old | New | Reason |
|---|---|---|
| `ofVec2f` for centroid math | `glm::vec2` | GLM migration |
| `vec.distance(other)` | `glm::distance(a, b)` | GLM free-function style |

### bin/data/lighting_gl_bump.frag

| Old | New | Reason |
|---|---|---|
| `texture2D(sampler, uv)` | `texture(sampler, uv)` | `texture2D` removed in GLSL 1.50 core |

> **Note:** The ES shaders (`lighting_es.frag`, `lighting_es_bump.frag`) still use
> `texture2D` — this is intentional and correct for GLSL ES used on Raspberry Pi.

---

## Required openFrameworks Version

**OF 0.12.0 or newer** is recommended. Download from https://openframeworks.cc

## Required Addons

- **ofxOpenCv** — bundled with OF, no separate install needed
- **ofxRPiCameraVideoGrabber** — only needed for Raspberry Pi target; included in
  the `addons/` folder of this project

## Building on Desktop (Linux / macOS / Windows)

```bash
# In your openFrameworks root (e.g. ~/openFrameworks):
cp -r /path/to/creepyportrait-modernized apps/myApps/creepyportrait
cd apps/myApps/creepyportrait
make -j$(nproc)
./bin/creepyportrait 0          # use webcam device 0, skull model
./bin/creepyportrait 0 jackevil # use webcam device 0, evil pumpkin
./bin/creepyportrait 0 all      # cycle through all models with 'm' key
```

## Building on Raspberry Pi 4/5 (64-bit Raspberry Pi OS)

The original `ofxRPiCameraVideoGrabber` addon was written for the Raspberry Pi
camera v1/v2 via the legacy `MMAL` interface, which was **removed** in
Raspberry Pi OS Bullseye (2021) and later. On modern Raspberry Pi OS you have
two options:

### Option A — Use a USB webcam (recommended, easiest)
Skip the Pi camera entirely and pass a device ID instead of `pi`:
```bash
./bin/creepyportrait 0
```

### Option B — Use `libcamera` with the newer `ofxRPiCamera` addon
The community has produced a `libcamera`-based replacement:
https://github.com/jvcleave/ofxRPiCamera

Replace the `addons/ofxRPiCameraVideoGrabber` folder with this newer addon and
rebuild. The `PiCameraSource` class in `VideoSource.h/.cpp` may need minor
adjustments to match the new addon's API.

## Keyboard Controls (unchanged)

| Key | Action |
|-----|--------|
| `v` | Toggle video/face-detection overlay display |
| `r` | Toggle skull auto-rotation mode (for shader testing) |
| `m` | Cycle to next 3D model (only when `all` models loaded) |

## Tuning Tips

- **`faceDepth`** — increase if the head doesn't turn far enough, decrease if
  it over-rotates.
- **`videoFOV`** — match to your webcam's diagonal FOV. 60° is a good default;
  wide-angle lenses need a higher value (e.g. 90°).
- **`faceBufferSize`** — increase on a fast desktop for smoother tracking;
  keep at 1–2 on a Pi to avoid lag.
- **`faceUpdateDelay`** — lower = more responsive but more CPU. On a Pi 4 you
  can safely go down to 0.3–0.5 s.
