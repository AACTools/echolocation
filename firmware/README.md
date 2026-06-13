# Echolocation Firmware

## Build
1. Install PlatformIO.
2. From `firmware/`, run `pio run -t upload` to flash the CoreS3.
3. Use `pio device monitor` for serial logs.

## Notes
- Audio files are expected on the microSD card under `audio/`.
- Logging can be toggled with `ECHOLOCATION_ENABLE_LOGS` in `platformio.ini`.
