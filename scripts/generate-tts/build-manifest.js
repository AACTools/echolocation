#!/usr/bin/env node
/**
 * Builds assets/audio-manifest.json from the keyboard token vocabulary.
 * Run: node scripts/generate-tts/build-manifest.js
 */

import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "../..");
const outPath = path.join(repoRoot, "assets/audio-manifest.json");

const letters = "abcdefghijklmnopqrstuvwxyz".split("");
const digits = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"];
const digitWords = [
  "Zero",
  "One",
  "Two",
  "Three",
  "Four",
  "Five",
  "Six",
  "Seven",
  "Eight",
  "Nine",
];

const symbolTokens = [
  ["exclamation", "Exclamation mark"],
  ["at", "At"],
  ["hash", "Hash"],
  ["pound", "Pound"],
  ["dollar", "Dollar"],
  ["percent", "Percent"],
  ["caret", "Caret"],
  ["ampersand", "Ampersand"],
  ["asterisk", "Asterisk"],
  ["left_paren", "Left parenthesis"],
  ["right_paren", "Right parenthesis"],
  ["minus", "Minus"],
  ["underscore", "Underscore"],
  ["equals", "Equals"],
  ["plus", "Plus"],
  ["left_bracket", "Left bracket"],
  ["right_bracket", "Right bracket"],
  ["left_brace", "Left brace"],
  ["right_brace", "Right brace"],
  ["backslash", "Backslash"],
  ["pipe", "Pipe"],
  ["semicolon", "Semicolon"],
  ["colon", "Colon"],
  ["single_quote", "Single quote"],
  ["double_quote", "Double quote"],
  ["comma", "Comma"],
  ["less_than", "Less than"],
  ["period", "Period"],
  ["greater_than", "Greater than"],
  ["slash", "Slash"],
  ["question", "Question"],
  ["backtick", "Backtick"],
  ["tilde", "Tilde"],
  ["not_sign", "Not sign"],
];

const namedKeys = [
  ["enter", "Enter"],
  ["escape", "Escape"],
  ["backspace", "Backspace"],
  ["tab", "Tab"],
  ["space", "Space"],
  ["caps_lock", "Caps lock"],
  ["f1", "F 1"],
  ["f2", "F 2"],
  ["f3", "F 3"],
  ["f4", "F 4"],
  ["f5", "F 5"],
  ["f6", "F 6"],
  ["f7", "F 7"],
  ["f8", "F 8"],
  ["f9", "F 9"],
  ["f10", "F 10"],
  ["f11", "F 11"],
  ["f12", "F 12"],
  ["print_screen", "Print screen"],
  ["scroll_lock", "Scroll lock"],
  ["pause", "Pause"],
  ["insert", "Insert"],
  ["home", "Home"],
  ["page_up", "Page up"],
  ["delete", "Delete"],
  ["end", "End"],
  ["page_down", "Page down"],
  ["arrow_right", "Right arrow"],
  ["arrow_left", "Left arrow"],
  ["arrow_down", "Down arrow"],
  ["arrow_up", "Up arrow"],
  ["num_lock", "Num lock"],
  ["numpad_divide", "Numpad divide"],
  ["numpad_multiply", "Numpad multiply"],
  ["numpad_minus", "Numpad minus"],
  ["numpad_plus", "Numpad plus"],
  ["numpad_enter", "Numpad enter"],
  ["numpad_1", "Numpad one"],
  ["numpad_2", "Numpad two"],
  ["numpad_3", "Numpad three"],
  ["numpad_4", "Numpad four"],
  ["numpad_5", "Numpad five"],
  ["numpad_6", "Numpad six"],
  ["numpad_7", "Numpad seven"],
  ["numpad_8", "Numpad eight"],
  ["numpad_9", "Numpad nine"],
  ["numpad_0", "Numpad zero"],
  ["numpad_decimal", "Numpad decimal"],
  ["menu", "Menu"],
  ["numpad_equals", "Numpad equals"],
  ["f13", "F 13"],
  ["f14", "F 14"],
  ["f15", "F 15"],
  ["f16", "F 16"],
  ["f17", "F 17"],
  ["f18", "F 18"],
  ["f19", "F 19"],
  ["f20", "F 20"],
  ["f21", "F 21"],
  ["f22", "F 22"],
  ["f23", "F 23"],
  ["f24", "F 24"],
];

const modifiers = [
  ["left_control", "Left control"],
  ["right_control", "Right control"],
  ["left_shift", "Left shift"],
  ["right_shift", "Right shift"],
  ["left_alt", "Left alt"],
  ["right_alt", "Right alt"],
  ["left_gui", "Left command"],
  ["right_gui", "Right command"],
];

const physicalKeys = [
  ["a", "A"], ["b", "B"], ["c", "C"], ["d", "D"], ["e", "E"], ["f", "F"],
  ["g", "G"], ["h", "H"], ["i", "I"], ["j", "J"], ["k", "K"], ["l", "L"],
  ["m", "M"], ["n", "N"], ["o", "O"], ["p", "P"], ["q", "Q"], ["r", "R"],
  ["s", "S"], ["t", "T"], ["u", "U"], ["v", "V"], ["w", "W"], ["x", "X"],
  ["y", "Y"], ["z", "Z"],
  ["1", "One"], ["2", "Two"], ["3", "Three"], ["4", "Four"], ["5", "Five"],
  ["6", "Six"], ["7", "Seven"], ["8", "Eight"], ["9", "Nine"], ["0", "Zero"],
  ["minus", "Minus"], ["equals", "Equals"], ["left_bracket", "Left bracket"],
  ["right_bracket", "Right bracket"], ["backslash", "Backslash"], ["hash", "Hash"],
  ["single_quote", "Single quote"], ["semicolon", "Semicolon"], ["slash", "Slash"],
  ["backtick", "Backtick"], ["comma", "Comma"], ["period", "Period"],
  ["enter", "Enter"], ["escape", "Escape"], ["backspace", "Backspace"],
  ["tab", "Tab"], ["space", "Space"], ["caps_lock", "Caps lock"],
  ["f1", "F 1"], ["f2", "F 2"], ["f3", "F 3"], ["f4", "F 4"], ["f5", "F 5"],
  ["f6", "F 6"], ["f7", "F 7"], ["f8", "F 8"], ["f9", "F 9"], ["f10", "F 10"],
  ["f11", "F 11"], ["f12", "F 12"], ["print_screen", "Print screen"],
  ["scroll_lock", "Scroll lock"], ["pause", "Pause"], ["insert", "Insert"],
  ["home", "Home"], ["page_up", "Page up"], ["delete", "Delete"], ["end", "End"],
  ["page_down", "Page down"], ["arrow_right", "Right arrow"],
  ["arrow_left", "Left arrow"], ["arrow_down", "Down arrow"],
  ["arrow_up", "Up arrow"], ["num_lock", "Num lock"],
  ["numpad_divide", "Numpad divide"], ["numpad_multiply", "Numpad multiply"],
  ["numpad_minus", "Numpad minus"], ["numpad_plus", "Numpad plus"],
  ["numpad_enter", "Numpad enter"], ["numpad_1", "Numpad one"],
  ["numpad_2", "Numpad two"], ["numpad_3", "Numpad three"],
  ["numpad_4", "Numpad four"], ["numpad_5", "Numpad five"],
  ["numpad_6", "Numpad six"], ["numpad_7", "Numpad seven"],
  ["numpad_8", "Numpad eight"], ["numpad_9", "Numpad nine"],
  ["numpad_0", "Numpad zero"], ["numpad_decimal", "Numpad decimal"],
  ["menu", "Menu"], ["numpad_equals", "Numpad equals"],
  ["f13", "F 13"], ["f14", "F 14"], ["f15", "F 15"], ["f16", "F 16"],
  ["f17", "F 17"], ["f18", "F 18"], ["f19", "F 19"], ["f20", "F 20"],
  ["f21", "F 21"], ["f22", "F 22"], ["f23", "F 23"], ["f24", "F 24"],
];

function entry(token, text) {
  const filename = `${token}.wav`;
  return { token, text, filename };
}

const entries = [];
const seen = new Set();

function add(token, text) {
  if (seen.has(token)) {
    return;
  }
  seen.add(token);
  entries.push(entry(token, text));
}

for (const letter of letters) {
  add(letter, letter.toUpperCase());
}

digits.forEach((digit, index) => {
  add(digit, digitWords[index]);
});

for (const [token, text] of symbolTokens) {
  add(token, text);
}

for (const [token, text] of namedKeys) {
  add(token, text);
}

for (const [token, text] of modifiers) {
  add(token, text);
}

for (const [suffix, text] of physicalKeys) {
  add(`key_${suffix}`, `${text} key`);
}

const manifest = { entries };
fs.writeFileSync(outPath, `${JSON.stringify(manifest, null, 2)}\n`);
console.log(`Wrote ${entries.length} entries to ${outPath}`);
