# FreeSabers

A ReeSabers-style custom saber engine for **Beat Saber Quest 1.40.8**. FreeSabers imports real
ReeSabers presets, stores its own `.freesaber` configs, and renders the blade, motion-blur trail,
glow and cut sparks itself instead of leaving the vanilla saber untouched.

## Features

- **Real ReeSabers import** – reads `.reesaber` files produced by ReeSabers 0.3.x (modules
  `reezonate.blur-saber`, `reezonate.simple-trail`, `reezonate.sparks-vfx`,
  `reezonate.vanilla-saber`, `reezonate.empty`) and converts them to native `.freesaber` presets.
- **Editable `.freesaber` presets** – human-readable JSON, versioned, with a left and/or right saber.
- **Procedural blade** – radius profile, caps, colour/alpha/scale mappings, emissive + PBR material
  properties, colour override and hue animation.
- **True motion-blur trail** – custom ribbon mesh with age-based width/colour/alpha, speed response
  and multiple simultaneous trails. Presets without a trail get one generated from `blurFrames`.
- **Cut sparks** – billboard particle bursts at the blade tip driven by `sparks-vfx` settings.
- **Glow shell** – optional transparent outer blade with pulse and cut flash.
- **Hanging bolts** – optional spring-chain joints below the blade tip.
- **Material drivers** – time, speed, time-after-cut and direction drivers adjust trail opacity.
- **Settings menu** – enable/disable effects, choose left/right presets, reload, import and save
  copies from the BSML settings page.

## Install

Install the built `FreeSabers.qmod` with QuestPatcher or copy `libFreeSabers.so` to your mods
folder. The mod targets Quest Beat Saber **1.40.8_7379** and depends on beatsaber-hook,
custom-types, BSML and paper2_scotland2.

## Preset folders

On the headset, FreeSabers uses:

```
/sdcard/ModData/com.beatgames.beatsaber/Mods/FreeSabers/
├── Presets/       .freesaber presets (Default.freesaber is created automatically)
├── ReeSabers/     drop .reesaber files here, then press Import in settings
├── settings.json  global toggles and selected presets
└── imported.txt   source-file -> preset-name import log
```

`.reesaber` files can also be dropped directly into `Presets/`. Import is non-destructive: a new
name is generated if a preset with the same name already exists. Use **Import .reesaber files** in
the settings menu after copying files; no restart is needed.

## `.freesaber` format

The root object is `"FreeSaber"` with `"formatVersion": 2`. A preset contains `name`, `author`,
`useSameForBoth` and `leftSaber` / `rightSaber`. Each saber has:

- `transform` – local position/rotation/scale
- `blade` – geometry, material, colour mappings, motion-blur settings and colour override
- `trails[]` – `trailType` (0 tip, 1 blade), `length`, `width`, `colorOverAge`, `alphaOverAge`,
  `widthOverAge`, `drivers`
- `sparks` – count, size, lifetime, velocity, spread, emission mode and colour curves
- `glow` – shell radius/intensity/colour/pulse
- `bolts` – optional hanging joints
- `drivers[]` – material drivers

Colours are `{ "r": 0-1, "g": 0-1, "b": 0-1, "a": 0-1 }`. Curves use control points with `time`
(0–1) and `value`; interpolation types match ReeSabers (`0` ease, `1` linear, `2` constant,
`3` smooth).

Example (shortened):

```json
{
  "FreeSaber": {
    "formatVersion": 2,
    "name": "My Saber",
    "useSameForBoth": true,
    "leftSaber": {
      "blade": {
        "radius": 0.0132,
        "baseColor": { "r": 0.1, "g": 0.8, "b": 1.0, "a": 1.0 },
        "blurFrames": 2.0
      },
      "trails": [
        {
          "trailType": 1,
          "length": 0.22,
          "colorOverAge": {
            "points": [
              { "time": 0.0, "value": { "r": 0.4, "g": 0.95, "b": 1.0, "a": 1.0 } },
              { "time": 1.0, "value": { "r": 0.0, "g": 0.25, "b": 0.6, "a": 0.0 } }
            ]
          }
        }
      ],
      "sparks": { "enabled": true, "count": 96 }
    },
    "rightSaber": { }
  }
}
```

`rightSaber` is mirrored from `leftSaber` when `useSameForBoth` is true.

## Import mapping

- Blade `MeshSettings.radius`, `stretchBias`, `saberProfile` (including the built-in
  Diamond/Pride/Basic profiles), caps and resolutions become the procedural blade.
- Blade `MaterialSettings` colours, PBR values, `blurFrames`, `blurSensitivity` and mask
  `lengthMappings` become blade material/mapping data.
- `simple-trail` `TrailLength`, `width`, `offset`, colour/width mappings and drivers become
  trails. ReeSabers "blade" trails are rendered as ribbons between both blade edges.
- `sparks-vfx` lifetime, velocity, spread, damping, gravity, turbulence, emission mode and
  colour gradients become the cut particle system. Counts are capped at 160 for Quest performance.
- Colour override/hue drivers are applied to the blade, glow and vanilla saber tint.
- Custom textures and binary mesh assets are not imported; procedural shapes and plain materials
  are used instead.

## Building

```sh
qpm restore
qpm build        # or: pwsh ./scripts/build.ps1
qpm qmod
```

The CMake project targets the Quest NDK through QPM. The pure config/importer layer also has a
standalone smoke test:

```sh
g++ -std=c++20 -I include -I <rapidjson>/include \
    tests/import_test.cpp src/config/SaberConfig.cpp src/config/JsonHelpers.cpp \
    src/config/ReeSabersImport.cpp -o import_test
./import_test path/to/preset.reesaber ./
```

## Limitations

- Custom textures, shaders and embedded `BinaryAssets` are approximated with the game's saber
  material because Quest shader assets are not available to the mod at runtime.
- Trails use a custom ribbon mesh rather than Unity's stripped `TrailRenderer`.
- Blade profiles are approximated from the ReeSabers preset list; arbitrary user-made profile
  files are not loaded.
- The `alwaysOnTop` trail option changes render queue/depth test but may not match PC behaviour.
