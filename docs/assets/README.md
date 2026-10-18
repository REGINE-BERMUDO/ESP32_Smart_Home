# Monochrome README artwork

The active header is `readme-banner.png`: the original approved black-and-white
silver-house artwork, preserved without modification. It is a generated design
illustration, not a photograph of the lab hardware.

## Badge and icon design

`badges/` contains twelve grayscale badges with technology logos, Inter lettering,
a silver perimeter, a soft white glow, and static reflective highlights. This is
a glass-style SVG finish, not live background blur or an interactive Liquid Glass
component. No React, animation, external badge service, or font request is needed
for display.

- Espressif marks identify the ESP32 boards and ESP-IDF SDK.
- PlatformIO, Bluetooth, and C++ badges use their technology marks.
- The dashboard badge includes HTML5, CSS, and JavaScript marks.
- Lucide provides generic Wi-Fi, verification, and section symbols.
- All visible artwork uses black, white, and neutral grays.

The badges' Inter letters are shaped with HarfBuzz and converted to SVG paths.
The body documentation stays native Markdown with GitHub's typography. Results
are static recorded checks for integration commit `df71770`, not live CI feeds.

## Sources and attribution

- [Simple Icons source manifest](sources/simple-icons/source.json) records the
  pinned upstream commit and technology marks; see its [license](sources/simple-icons/LICENSE).
- [Lucide source manifest](sources/lucide/source.json) records the pinned icon
  sources; see its [license](sources/lucide/LICENSE).
- Inter is already hosted at `GatewayNode/web/inter-latin.woff2`. Its notice is
  included in [INTER-LICENSE.txt](INTER-LICENSE.txt).

The original banner was created with the built-in image-generation tool using
the approved silver-house direction: a wide black banner, luminous silver house,
sweeping reflective ribbons, and the title ESP32 Smart Home. It has deliberately
not been regenerated during this refinement.

## Rebuild badges and section icons

From the repository root, use a separate Python environment:

```powershell
python -m venv "$env:TEMP/esp32-readme-tools"
& "$env:TEMP/esp32-readme-tools/Scripts/python.exe" -m pip install -r docs/assets/requirements.txt
& "$env:TEMP/esp32-readme-tools/Scripts/python.exe" docs/assets/build_graphics.py --root .
```

The generator uses bundled sources and leaves the banner PNG unchanged.
`graphics-manifest.json` records the active banner, badge labels, counts, and
rendering approach. These optional documentation tools add no firmware dependencies.
