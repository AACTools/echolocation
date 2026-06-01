#!/usr/bin/env node

const { execSync, spawnSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const repoRoot = path.resolve(__dirname, "../..");
const manifestPath = path.join(repoRoot, "assets/audio-manifest.json");
const outputRoot = path.join(repoRoot, "sd-card/audio/keys");
const tmpDir = path.join(__dirname, "tmp");
const voicesConfig = JSON.parse(
  fs.readFileSync(path.join(__dirname, "voices.json"), "utf8")
);

function parseArgs(argv) {
  const args = {
    voice: voicesConfig.defaultVoice,
    rate: 180,
    force: false,
    listVoices: false,
  };
  for (let i = 2; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === "--force") args.force = true;
    if (arg === "--list-voices") args.listVoices = true;
    if (arg === "--voice" && argv[i + 1]) {
      args.voice = argv[++i];
    }
    if (arg === "--rate" && argv[i + 1]) {
      args.rate = Number(argv[++i]);
    }
  }
  return args;
}

function listVoices() {
  const output = execSync("say -v '?'", { encoding: "utf8" });
  console.log(output);
}

function usageToFilename(usageHex) {
  const value = Number.parseInt(usageHex, 16);
  return `u${value.toString(16).padStart(3, "0")}.wav`;
}

/** Text passed to macOS `say` (avoids e.g. "Capital A" for label "A"). */
function speakTextForEntry(entry) {
  if (entry.speak) {
    return entry.speak;
  }
  const { label } = entry;
  if (label.length === 1) {
    const code = label.toUpperCase().charCodeAt(0);
    if (
      (code >= 0x41 && code <= 0x5a) ||
      (code >= 0x30 && code <= 0x39)
    ) {
      return `[[char U+${code.toString(16).padStart(4, "0").toUpperCase()}]]`;
    }
  }
  return label;
}

function runSay(entry, voice, rate, aiffPath) {
  const text = speakTextForEntry(entry);
  spawnSync(
    "say",
    ["-v", voice, "-r", String(rate), "-o", aiffPath, text],
    { stdio: "inherit" }
  );
}

function convertToWav(aiffPath, wavPath) {
  try {
    execSync(
      `afconvert -f WAVE -d LEI16@16000 "${aiffPath}" "${wavPath}"`,
      { stdio: "pipe" }
    );
    return;
  } catch (_error) {
    execSync(
      `ffmpeg -y -i "${aiffPath}" -ar 16000 -ac 1 -sample_fmt s16 "${wavPath}"`,
      { stdio: "inherit" }
    );
  }
}

function main() {
  const args = parseArgs(process.argv);
  if (args.listVoices) {
    listVoices();
    return;
  }

  if (process.platform !== "darwin") {
    console.error("This script requires macOS built-in say.");
    process.exit(1);
  }

  const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
  fs.mkdirSync(outputRoot, { recursive: true });
  fs.mkdirSync(tmpDir, { recursive: true });

  let created = 0;
  let skipped = 0;

  for (const entry of manifest.entries) {
    const filename = usageToFilename(entry.usage);
    const wavPath = path.join(outputRoot, filename);
    if (fs.existsSync(wavPath) && !args.force) {
      skipped += 1;
      continue;
    }

    const aiffPath = path.join(tmpDir, `${filename}.aiff`);
    console.log(`Generating ${entry.label} -> ${filename}`);
    runSay(entry, args.voice, args.rate, aiffPath);
    convertToWav(aiffPath, wavPath);
    fs.unlinkSync(aiffPath);
    created += 1;
  }

  console.log(`Done. Created ${created}, skipped ${skipped}.`);
  console.log(`Copy ${path.join(repoRoot, "sd-card")} to a FAT32 microSD card.`);
}

main();
