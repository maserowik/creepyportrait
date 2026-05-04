# Creepy Portrait Simulator

An interactive 3D preview of the Creepy Portrait project — shows the real skull
and jack-o-lantern models tracking your mouse, with the real textures and red
curtain background from the project.

## How to Run

All assets are embedded in `index.html` — no separate asset loading needed.

**From the repo root:**

```bash
python3 -m http.server 8080
```

Then open from **any device on the same WiFi network:**

```
http://[Pi IP address]:8080/simulator/
```

Find your Pi's IP address with: `hostname -I`

## What You Can Do

- Move your mouse over the portrait — the skull/jack tracks left and right
- **Camera Position** — try all 9 positions to see how mount location affects tracking quality
- **Model** — switch between skull, evil jack, and happy jack
- **Vertical Tracking** — toggle up/down tracking on/off
- **Tracking Speed** — adjust how snappy or smooth the tracking feels
- **Rotation Range** — adjust how far the skull turns

## Notes

- The white skull appearance is normal when viewed through VNC — connect a real HDMI monitor for full textures
- This is a preview tool only — actual rendering on the Pi looks better with hardware OpenGL
- Three.js r128 is loaded via CDN — requires internet connection on first load
