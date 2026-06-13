# Generate TTS Audio

## Requirements
- A local `piper` binary available in `PATH` (or set `PIPER_BIN`).
- A TTS model path passed via `--voice`, or the `en_GB` key in `voices.json`.

## Install Piper
- macOS (Homebrew):
  - `brew install piper-tts`
- Linux (prebuilt):
  - Download the latest release from [https://github.com/rhasspy/piper/releases](https://github.com/rhasspy/piper/releases)
  - Extract and add the `piper` binary to your `PATH`
- Optional:
  - Set `PIPER_BIN=/absolute/path/to/piper` if the binary is not in `PATH`

## Download Voices
1. Pick a voice/model from the Piper releases page (look for `.onnx` model files).
2. Download the `.onnx` file (and the matching `.onnx.json` metadata if provided).
3. Place the model somewhere on disk (e.g. `~/models/piper/`).
4. Pass the model path to `--voice` or map it in `voices.json`.

## Usage
```
npm run generate -- --manifest ../../assets/audio-manifest.json --out ../../sd-card/audio --voice ~/models/en_GB-alba-medium.onnx --force
```

The script writes WAV files to the output directory for every entry in the manifest.

### Regenerate Existing Files
Pass `--force` to overwrite existing WAVs:
```
npm run generate -- --manifest ../../assets/audio-manifest.json --out ../../sd-card/audio --voice en_US --force
```
