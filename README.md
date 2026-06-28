# echolocation

Assistive hardware that connects to a keyboard, speaks each key aloud, and forwards held keys to a computer as a standard USB or Bluetooth keyboard.

Built for the M5Stack CoreS3. See [spec.md](spec.md) for full product requirements.

## Get started on your own hardware

### 1. Assemble the device

You need a CoreS3, battery bottom, USB host module, audio module, and a FAT32 microSD card with speech files. Part list: [firmware/README.md](firmware/README.md#hardware).

### 2. Build and flash firmware

Install [PlatformIO](https://platformio.org/), then from the `firmware` directory:

```bash
cd firmware
pio run -e m5stack-cores3 -t upload
```

Connect the CoreS3 with a USB-C **data** cable. The first build downloads dependencies and may take a few minutes.

If upload fails, long-press **RST** until the screen goes blank and try again. See [firmware/README.md](firmware/README.md) for details.

### 3. Load speech onto the microSD card

Generate WAV files with Piper TTS — see [scripts/generate-tts/README.md](scripts/generate-tts/README.md). Copy the `audio` folder to the root of a FAT32 microSD card, insert it into the CoreS3, and reboot.

## Releases

Pre-built firmware is published on [GitHub Releases](https://github.com/AACTools/echolocation/releases). Each CoreS3 zip includes `bootloader.bin`, `partitions.bin`, and `firmware.bin` plus flashing instructions.

## Repository layout

```
firmware/           CoreS3 firmware (PlatformIO / Arduino)
scripts/generate-tts  TTS audio generation for the microSD card
assets/             Audio manifest for speech generation
spec.md             Product specification (source of truth)
test-plan.md        Manual test checklist
```
