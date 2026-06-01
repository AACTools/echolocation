# Generate speech files for microSD

This script uses the macOS built-in `say` command to generate WAV files listed in [`assets/audio-manifest.json`](../../assets/audio-manifest.json).

## Prerequisites

- macOS with `say`
- `afconvert` (included with macOS) or `ffmpeg` for WAV conversion
- Node.js 18+

## Usage

```bash
cd scripts/generate-tts
npm run voices          # list available macOS voices
npm run generate        # generate WAV files into sd-card/
npm run generate -- --voice Alex --rate 170
npm run generate -- --force   # regenerate existing files
```

Copy the generated `sd-card/` folder contents to the root of a FAT32 microSD card, then insert the card into the CoreS3.
