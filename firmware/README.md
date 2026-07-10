# echolocation firmware

Firmware for the [M5Stack CoreS3](https://docs.m5stack.com/en/core/CoreS3)-based echolocation assistive keyboard device.

## Hardware

| Part | Purpose |
|------|---------|
| [M5Stack CoreS3](https://shop.m5stack.com/products/m5stack-cores3-esp32s3-iotdevelopment-kit) | Main controller and screen |
| [M5GO Battery Bottom3](https://shop.m5stack.com/products/m5go-battery-bottom3-for-cores3-only) | Battery pack |
| [USB module (MAX3421E)](https://shop.m5stack.com/products/usb-module-with-max3421e-v1-2) | USB keyboard host |
| [Audio module (ES8388)](https://shop.m5stack.com/products/m5stack-audio-module-es8388) | Speech output |
| microSD card (FAT32) | Pre-generated key speech WAV files |
| USB-C data cable | Build, flash, and USB computer connection |

Stack the modules per M5Stack docs before flashing. The microSD card is required for key speech but not for flashing firmware.

## Build and upload

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) (or use the [PlatformIO IDE extension](https://docs.platformio.org/en/latest/integration/ide/index.html) for VS Code).

Connect the CoreS3 with a data-capable USB-C cable, then:

```bash
cd firmware
pio run -e m5stack-cores3              # build
pio run -e m5stack-cores3 -t upload    # build and upload
```

Other useful commands:

```bash
pio device list                        # find the serial port
pio device monitor -b 115200           # serial logs
```

The first build downloads the ESP32 platform and libraries and may take several minutes.

### Debug build

To show the Debug settings screen and use USB serial only (the device will not act as a USB keyboard to your computer):

```bash
pio run -e m5stack-cores3-debug -t upload
pio device monitor -b 115200
```

## microSD card

Key speech is read from WAV files on a FAT32 microSD card at `/audio/*.wav`.

Generate the files with Piper TTS — see [scripts/generate-tts/README.md](../scripts/generate-tts/README.md). Run `npm run build-manifest` first, then `npm run generate`. Copy the `audio` folder to the root of the card, insert it into the CoreS3 SD slot, and reboot.

## Flash pre-built binaries

GitHub [releases](https://github.com/AACTools/echolocation/releases) include `echolocation-cores3-<tag>.zip` with `bootloader.bin`, `partitions.bin`, and `firmware.bin`.

Put the CoreS3 in download mode (long-press **RST** until the screen goes blank), then flash with [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html):

```bash
esptool.py --chip esp32s3 -p PORT write_flash \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin
```

Replace `PORT` with your serial device.

## Troubleshooting upload

Upload often fails when the device is already acting as a USB keyboard:

1. Close the serial monitor (it locks the port).
2. Long-press **RST** on the bottom of the CoreS3 until the screen goes blank.
3. Run `pio run -e m5stack-cores3 -t upload` again within a few seconds.
4. If it still fails, unplug USB, plug back in, repeat step 2, then upload.

Use a data-capable USB-C cable connected directly to your computer (not through an unpowered hub).

## Bluetooth

Keyboard input and computer output use the same ESP32 BLE radio but are independent in code, settings, and UI. Both can be enabled at the same time.

### Bluetooth keyboard input

Settings → **Bluetooth** → **Keyboard** connects the device **to an external BLE keyboard** (NimBLE central role).

- Toggle **on** — starts scan/reconnect; status shows **Searching...** until connected, then **Connected** (green).
- Toggle **off** — stops scan, disconnects, and hides status.
- **Search for keyboard** — live scan of HID keyboards; tap a device to pair. While already connected, search shows a blocked message (no list) until you forget the current device.
- **See paired devices** — bonded keyboards; green tick on the connected device; tap → **Forget device** to remove the bond.
- On boot, if the toggle is on and a bonded keyboard exists, the device auto-reconnects in the background.
- USB keyboard input takes priority when a USB keyboard is also plugged in.

Serial logs use the `[ble-kb]` prefix. See [test-plan.md](../test-plan.md) §2 for manual validation steps (including Apple Magic Keyboard and simultaneous computer output).

### Bluetooth computer output

Settings → **Bluetooth** → **Computer / Output** enables a BLE HID keyboard named **echolocation** (NimBLE peripheral role). When the toggle is on, pair from your phone, tablet, or Mac in Bluetooth settings. Held keys (after the configured hold duration) are sent to the connected BLE host. USB computer output takes priority when the USB-C cable is connected to a host.

Output advertising pauses while the keyboard input module is actively scanning for new keyboards; an existing output connection is not dropped.

## Project layout

```
firmware/
  platformio.ini   # board and library configuration
  src/             # application source
  include/         # headers and LVGL config
```

See [spec.md](../spec.md) for product behaviour and [test-plan.md](../test-plan.md) for manual validation steps.
