# Generate TTS Audio

## Requirements
- A local `piper` binary available in `PATH` (or set `PIPER_BIN`).
- A TTS model path passed via `--voice`.

## Usage
```
npm run generate -- --manifest ../../assets/audio-manifest.json --out ../../sd-card/audio --voice en_US
```

The script writes WAV files to the output directory for every entry in the manifest.
