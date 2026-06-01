# echolocation

Assistive keyboard bridge for visually impaired users. A standard keyboard connects to the device; every keypress is spoken aloud. When a key is held for a configurable duration, a single keypress is sent to a connected computer (USB or Bluetooth).

See [SPEC.md](SPEC.md) for the full product specification.

## Repository layout

| Path | Purpose |
|------|---------|
| [firmware/](firmware/) | PlatformIO C++ firmware for M5Stack CoreS3 |
| [scripts/generate-tts/](scripts/generate-tts/) | macOS `say` script to build microSD audio files |
| [assets/audio-manifest.json](assets/audio-manifest.json) | HID key labels for TTS generation |

## Quick start

1. **Generate audio (macOS):** see [scripts/generate-tts/README.md](scripts/generate-tts/README.md)
2. **Copy `sd-card/` to a FAT32 microSD** and insert into CoreS3
3. **Build and flash firmware:** see [firmware/README.md](firmware/README.md)

## Hardware

- M5Stack CoreS3
- M5GO Battery Bottom3
- USB Module with MAX3421E v1.2
- M5Stack Audio Module (STM32G030)
