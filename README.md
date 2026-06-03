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

## Installing from a release

Published builds are on [GitHub Releases](https://github.com/AACTools/echolocation/releases). Each release includes:

| Asset | Purpose |
|-------|---------|
| `echolocation-sd-audio-*.zip` | Speech files for the microSD card |
| `echolocation-cores3-*.zip` | Firmware binaries and flashing notes |
| `firmware.bin` | Application image only (use the zip for a full flash) |

You need both the audio zip and the firmware zip (or a dev build from source) before the device is ready to use.

### Load speech files (microSD)

1. Format a microSD card as **FAT32** (32 GB or smaller cards are usually pre-formatted).
2. Download and unzip `echolocation-sd-audio-*.zip`.
3. Copy everything inside the unzipped folder to the **root** of the card. After copying, the card should contain `audio/keys/*.wav` at the top level (not an extra parent folder).
4. Eject the card, insert it into the CoreS3 SD slot, and power on.

If the card is missing or the layout is wrong, the device shows an on-screen error.

### Flash firmware (CoreS3)

Connect the CoreS3 to your computer with USB-C. Use one of these methods:

**Option A — PlatformIO (recommended if you build from source)**

```bash
cd firmware
pio run -e m5stack-cores3 -t upload
```

**Option B — esptool (from the release zip)**

1. Download and unzip `echolocation-cores3-*.zip`.
2. Install [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/) if needed (`pip install esptool`).
3. From inside the unzipped folder, flash all three binaries (replace `PORT` with your serial port, e.g. `/dev/cu.usbmodem*` on macOS or `COM3` on Windows):

```bash
esptool.py --chip esp32s3 -p PORT write_flash \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin
```

The same commands are in `FLASHING.txt` inside the firmware zip.

After flashing, open a serial monitor if you want boot logs: `pio device monitor` from `firmware/`, or any 115200 baud terminal on the USB port.

### Order of setup

1. Flash firmware first (or confirm a matching version is already on the device).
2. Prepare the microSD card and insert it.
3. Connect the keyboard to the USB module and the computer to the CoreS3 USB-C port when you are ready to use the bridge.

Hardware assembly and DIP switch settings are in [firmware/README.md](firmware/README.md).

## Hardware

- M5Stack CoreS3
- M5GO Battery Bottom3
- USB Module with MAX3421E v1.2
- M5Stack Audio Module (STM32G030)
