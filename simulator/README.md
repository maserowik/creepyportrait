# Creepy Portrait Simulator

An interactive 3D preview of the Creepy Portrait project — shows the real skull
and jack-o-lantern models tracking your mouse, with the real textures and red
curtain background from the project.

## How to Run

The simulator loads assets from `bin/data/` using relative paths so it needs
a local web server — browsers block file:// access for security.

**From the repo root, run:**

```bash
python3 -m http.server 8080
```

Then open in your browser:

```
http://localhost:8080/simulator/
```

## What You Can Do

- **Move your mouse** over the portrait — the skull/jack tracks left and right
- **Camera Position** — try all 9 positions to see how mount location affects tracking quality
- **Model** — switch between skull, evil jack, and happy jack
- **Vertical Tracking** — toggle up/down tracking on/off
- **Tracking Speed** — adjust how snappy or smooth the tracking feels
- **Rotation Range** — adjust how far the skull turns

## Notes

- The white skull appearance means textures haven't loaded yet — wait a moment
- This is a preview tool only — actual rendering on the Pi looks better with hardware OpenGL
- The simulator uses Three.js r128 via CDN — requires internet on first load
