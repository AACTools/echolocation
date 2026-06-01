# echolocation firmware

Firmware for M5Stack CoreS3 with Module USB v1.2 (keyboard host) and Module Audio (optional headphone output).

## Hardware stack (bottom to top)

1. M5GO Battery Bottom3
2. M5Stack CoreS3
3. M5Stack Audio Module (I2S config B)
4. USB Module v1.2 (DIP switches set for CoreS3)

## Connections

- **Keyboard:** USB-A on the USB module
- **Computer:** CoreS3 USB-C port (device presents as a keyboard)
- **Audio:** Built-in speaker by default; headphones on the Audio module TRRS jack
- **Speech files:** FAT32 microSD in the CoreS3 slot (`/audio/keys/*.wav`)

## USB module DIP switches (CoreS3)

Set per M5 documentation for CoreS3 SPI pins (SS on GPIO1, INT on GPIO10).

## Build and upload

```bash
cd firmware
pio run -e m5stack-cores3
pio run -e m5stack-cores3 -t upload
pio device monitor
```

## Unit tests (host)

```bash
cd firmware
pio test -e native
```

## Manual test checklist

- [ ] USB keyboard: each key is spoken immediately
- [ ] Hold key for configured duration: one keypress sent to computer
- [ ] Release without hold: no computer keypress
- [ ] Modifier-only hold behaves like other keys
- [ ] New key during speech stops previous audio
- [ ] Keyboard and computer connected together: speak always, hold sends to computer
- [ ] Swap USB keyboard without reboot
- [ ] Settings persist across reboot
- [ ] Factory reset clears settings
- [ ] Headphone insert switches audio output
- [ ] Missing microSD or WAV shows a clear screen error
