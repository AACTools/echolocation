import fs from "fs";
import os from "os";
import path from "path";
import { execFileSync } from "child_process";

const args = process.argv.slice(2);
const getArg = (name, fallback) => {
  const index = args.indexOf(name);
  if (index === -1 || index === args.length - 1) {
    return fallback;
  }
  return args[index + 1];
};

const manifestPath = getArg("--manifest", "../../assets/audio-manifest.json");
const outputDir = getArg("--out", "../../sd-card/audio");
const voice = getArg("--voice", "en_GB");
const piperBin = process.env.PIPER_BIN || "piper";
const voicesPath = getArg("--voices", "./voices.json");
const force = args.includes("--force");

const resolvedManifest = path.resolve(manifestPath);
const resolvedOutput = path.resolve(outputDir);
const resolvedVoices = path.resolve(voicesPath);

if (!fs.existsSync(resolvedManifest)) {
  console.error(`Manifest not found: ${resolvedManifest}`);
  process.exit(1);
}

fs.mkdirSync(resolvedOutput, { recursive: true });

const manifest = JSON.parse(fs.readFileSync(resolvedManifest, "utf-8"));
const voices = fs.existsSync(resolvedVoices)
  ? JSON.parse(fs.readFileSync(resolvedVoices, "utf-8"))
  : {};

const expandHome = (filePath) => {
  if (!filePath.startsWith("~")) {
    return filePath;
  }
  return path.join(os.homedir(), filePath.slice(1));
};

const resolveVoice = (voiceKey) => {
  const expanded = expandHome(voiceKey);
  if (expanded.endsWith(".onnx")) {
    return expanded;
  }
  const mapped = voices[voiceKey];
  if (!mapped) {
    console.error(`Voice not found: ${voiceKey}`);
    console.error(`Add it to ${resolvedVoices} or pass a .onnx path.`);
    process.exit(1);
  }
  return expandHome(mapped);
};

const resolvedVoice = resolveVoice(voice);

for (const entry of manifest.entries) {
  const outFile = path.join(resolvedOutput, entry.filename);
  if (!force && fs.existsSync(outFile)) {
    continue;
  }

  const text = entry.text;
  console.log(`Generating ${outFile} for "${text}"`);

  try {
    execFileSync(
      piperBin,
      ["--model", resolvedVoice, "--output_file", outFile, text],
      { stdio: "inherit" },
    );
  } catch (error) {
    console.error(`Failed to generate ${outFile}:`, error.message);
  }
}

