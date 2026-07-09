# echolocation — Manual Test Plan

This plan validates that echolocation remains reliable when connections are made, broken, and remade repeatedly. It follows the feature outline in [spec.md](spec.md).

The primary risk under test is **connection state drift**: stale UI, stuck “Connecting…”, silent keys, duplicate computer keypresses, or BLE/USB stacks failing to recover after many cycles.

---

## Test environment

### Hardware

- M5Stack CoreS3 with M5GO Battery Bottom3
- USB module (MAX3421E) attached — required for USB keyboard host
- Audio module attached — required for TTS output
- microSD card loaded with generated `/audio` WAV files

### Peripherals

| Role | Recommended device | Alternate |
|------|-------------------|-----------|
| USB keyboard | Any USB HID keyboard | — |
| Bluetooth keyboard | Apple Magic Keyboard | Any BLE HID keyboard |
| USB computer | Mac, iPad (USB), or Linux PC | Any host that accepts USB HID |
| Bluetooth computer | iPad, Mac, or phone | Any host that can pair a BLE keyboard named **echolocation** |

### Software / build

- Flash the latest production firmware (not `ECHOLOCATION_BLE_DEBUG` unless debugging a specific failure).
- Optional: keep a serial monitor open at 115200 baud to capture `[ble-kb]`, `[speaker]`, and boot logs during failures.

### Baseline checks (run once before stress cycles)

1. Power on — device boots to the main screen without hanging.
2. Settings → open each menu (Volume, Hold Duration, Bluetooth) — navigation is responsive.
3. Main screen shows connection flow indicator (`input → output` icons) and battery level.
4. Press a key with **no** keyboard connected — nothing should crash; no spurious audio.
5. Settings → Bluetooth → **Keyboard** — toggle off shows only **See paired devices**; toggle on shows **Searching...** (or **Connected** if already paired).
6. Settings → Bluetooth → **Computer / Output** — USB and Bluetooth status labels render.

Record firmware version / git commit and the peripheral models used. Re-run relevant sections after any firmware change.

---

## Pass / fail criteria (apply after every cycle)

Mark a cycle **PASS** only if **all** of the following hold:

| Check | Expected |
|-------|----------|
| UI status | Main-screen flow indicator and the relevant settings screen match actual connection state within a few seconds |
| Speech | Each keypress is spoken; audio starts within ~50 ms of key down |
| Speech interrupt | Pressing a new key while audio is playing stops the current clip and speaks the new key |
| Screen | Main screen shows the pressed key label; hold outline appears after the configured hold duration |
| Computer output | Holding a key sends **exactly one** keypress to the connected computer (USB and/or BLE as applicable) |
| Recovery | After disconnect, the device is ready for the next connect without reboot |
| Stability | No freeze, watchdog reset, or permanent “Connecting…” / “Connection failed” without cause |

Mark **FAIL** on first violation; note the cycle number, what was connected, and whether a reboot recovers.

---

## 1. USB keyboard — repeated connect / disconnect

**Goal:** USB host stack and keyboard input path survive many plug/unplug cycles.

**Setup:** Bluetooth keyboard **disabled** (Settings → Bluetooth → Keyboard Connection → toggle off). No computer connected yet.

| Step | Action |
|------|--------|
| 1 | Plug USB keyboard into the USB module |
| 2 | Confirm main-screen input icon shows USB; press `a`, `b`, `1`, `Space` — each is spoken and shown on screen |
| 3 | Tap keys quickly — each new key interrupts prior audio |
| 4 | Unplug USB keyboard |
| 5 | Confirm input icon shows disconnected; keypresses do nothing |
| 6 | Repeat steps 1–5 **20 times** |

**Variations (5 cycles each after the main 20):**

- Unplug while a key is held and audio is playing.
- Unplug while the hold outline is visible (key was held past hold duration).
- Leave unplugged for 30 s, then reconnect.

---

## 2. Bluetooth keyboard — repeated connect / disconnect

**Goal:** BLE central (NimBLE client), scan, HID subscription, bonding, and auto-reconnect behave after many connect/disconnect cycles.

**Setup:** Settings → Bluetooth → **Keyboard** → toggle **on**. Have a BLE keyboard (e.g. Apple Magic Keyboard) powered and in pairing/discoverable mode for the first pairing.

### 2a. First pairing via search

| Step | Action |
|------|--------|
| 1 | Main Keyboard screen shows **Searching...** (blue, animated dots) and **See paired devices** + **Search for keyboard** |
| 2 | Tap **Search for keyboard** — search screen shows **Searching...** and a live device list |
| 3 | Tap your keyboard in the list — spinner appears beside the name; button is disabled |
| 4 | On success, screen returns to main Keyboard screen with **Connected** (green) |
| 5 | Press keys — each is spoken within ~50 ms and shown on the main screen |
| 6 | **See paired devices** — connected keyboard shows a green tick beside its name |

### 2b. Search blocked while connected

| Step | Action |
|------|--------|
| 1 | With keyboard connected, tap **Search for keyboard** |
| 2 | Search screen shows **Connected** (green) and instructional text — no device list, no scanning |
| 3 | Message explains you must forget the current device or turn the keyboard off to pair another |

### 2c. Forget and re-pair

| Step | Action |
|------|--------|
| 1 | **See paired devices** → tap the connected keyboard → **Forget device** |
| 2 | Returns to main Keyboard screen; status returns to **Searching...** if toggle still on |
| 3 | **Search for keyboard** → discover → connect again (repeat 2a steps 2–5) |

### 2d. Repeated disconnect / reconnect (20 cycles)

| Step | Action |
|------|--------|
| 1 | With keyboard connected and working, **Disconnect A** — power off the keyboard (or move out of BLE range >10 m) |
| 2 | Main screen input icon clears; Keyboard screen shows **Searching...** |
| 3 | **Reconnect** — power keyboard back on; wait for auto-reconnect (or tap device in **See paired devices**) |
| 4 | Status returns to **Connected**; press keys — still works |
| 5 | Repeat steps 1–4 **20 times** |

**Disconnect method B (5 cycles):** Settings → toggle Bluetooth Keyboard **off**, then **on** — reconnect via search or paired list.

**Disconnect method C (5 cycles):** Pair a **different** BLE keyboard (forget first), then switch back to the original.

### 2e. Simultaneous BLE computer output

With BLE keyboard connected, enable Settings → Bluetooth → **Computer / Output** and pair a host to **echolocation**:

| Step | Action |
|------|--------|
| 1 | Both input (Bluetooth icon) and output (Bluetooth icon) show on main screen |
| 2 | Short tap keys — spoken only |
| 3 | Hold a key past hold duration — exactly one keypress sent to the BLE host |
| 4 | During an active **Search for keyboard** session (not connected), Computer/Output advertising pauses; resumes when search stops |

**Failure modes to watch:**

- Stuck connecting (spinner beside device name) for >15 s with no recovery
- Keys silent while UI still shows **Connected**
- Auto-reconnect loops without ever succeeding (serial `[ble-kb]` logs)
- Search list visible while already connected (should be blocked)

---

## 3. USB computer — repeated connect / disconnect

**Goal:** USB HID device enumeration to the host survives many cable connect/disconnect cycles.

**Setup:** USB keyboard connected (either path). Set a short hold duration (e.g. 300 ms) for faster iteration.

| Step | Action |
|------|--------|
| 1 | Connect CoreS3 to computer via USB data port (device acts as USB keyboard to host) |
| 2 | Settings → Bluetooth → Computer Connection — “USB: Connected” |
| 3 | Main-screen output icon shows USB |
| 4 | Open a text field on the host; hold `a` on the physical keyboard until outline appears — exactly one `a` appears on the host |
| 5 | Continue holding — no additional `a` characters |
| 6 | Short tap `b` — spoken but **not** sent to host |
| 7 | Disconnect USB cable from host (or hub) |
| 8 | Confirm “USB: Not connected”; output icon clears |
| 9 | Repeat steps 1–8 **20 times** |

**Variations (5 cycles each):**

- Disconnect during an active key hold (outline visible).
- Reconnect while BLE computer is also paired (see §5) — USB should take output when mounted.

---

## 4. Bluetooth computer — repeated pair / disconnect

**Goal:** BLE HID server, pairing, and advertising recover after many host-side disconnects.

**Setup:** USB keyboard connected. Note the BLE name on Computer Connection screen (default **echolocation**).

| Step | Action |
|------|--------|
| 1 | On the host, open Bluetooth settings and pair/connect to **echolocation** |
| 2 | Computer Connection screen shows “Bluetooth: Connected”; main output icon shows Bluetooth |
| 3 | Hold a key — exactly one character on the host |
| 4 | On the host, **disconnect** or **forget** the echolocation keyboard |
| 5 | Device shows “Bluetooth: Not connected”; device should resume advertising (re-pairable) |
| 6 | Pair again from the host |
| 7 | Hold a different key — works again |
| 8 | Repeat steps 4–7 **20 times** |

**Variations (5 cycles each):**

- Host sleeps / locks while connected, then wakes — connection may drop; verify reconnect works.
- Multiple hosts: pair to host A, disconnect, pair to host B (if available).

---

## 5. Dual connections — keyboard + computer stress

**Goal:** Simultaneous input and output paths stay correct when either side is cycled.

**Setup:** USB keyboard + USB computer both connected (simplest path). Then repeat key scenarios with BLE on either side.

### 5a. Cycle computer while keyboard stays connected (10 cycles)

1. With keyboard working, disconnect/reconnect **computer** (USB or BLE per section 3 or 4).
2. After each reconnect, verify: speech always works; hold-to-send works only when computer is connected.

### 5b. Cycle keyboard while computer stays connected (10 cycles)

1. With computer working, disconnect/reconnect **keyboard** (USB unplug or BLE power cycle).
2. After each reconnect, verify hold-to-send still sends exactly one key.

### 5c. Full stack rotation (10 cycles)

Each cycle, change one link in this order:

```
USB keyboard  → USB computer   → press & hold keys
USB keyboard  → BLE computer   → press & hold keys
BLE keyboard  → BLE computer   → press & hold keys
BLE keyboard  → USB computer   → press & hold keys
disconnect all → idle 10 s → start next cycle
```

---

## 6. Switching between keyboard types (USB ↔ BLE)

**Goal:** USB input takes priority over BLE; switching does not leave ghost input or missed keys.

**Spec reference:** “Seamlessly switch between different keyboards.”

| Step | Action |
|------|--------|
| 1 | Connect BLE keyboard only — press keys, confirm speech |
| 2 | Plug in USB keyboard **without** disconnecting BLE |
| 3 | Press keys on USB keyboard — speech follows USB |
| 4 | Press keys on BLE keyboard — should be **ignored** while USB is connected |
| 5 | Unplug USB — BLE resumes within a few seconds |
| 6 | Repeat steps 1–5 **10 times** |

Also toggle BLE off/on while USB is connected (5 times) — USB should remain unaffected.

---

## 7. Switching between computer types (USB ↔ BLE)

**Goal:** Key output routes correctly when both USB and BLE computer links are available.

| Step | Action |
|------|--------|
| 1 | Connect USB computer only — hold sends to USB host |
| 2 | Also pair BLE computer — with USB still mounted, hold sends via USB (verify on USB host) |
| 3 | Disconnect USB — hold sends via BLE |
| 4 | Reconnect USB — USB regains output |
| 5 | Repeat **10 times** |

---

## 8. External speaker — repeated plug / unplug

**Goal:** Audio routing switches cleanly without silencing TTS permanently.

**Setup:** Keyboard connected; press keys periodically during this test.

| Step | Action |
|------|--------|
| 1 | With no jack plugged, confirm built-in speaker icon (grey) and audio from CoreS3 speaker |
| 2 | Press keys — speech audible from built-in speaker |
| 3 | Plug headphones/speaker into audio module jack |
| 4 | Icon turns accent colour; audio moves to external output |
| 5 | Press keys — speech on external output |
| 6 | Unplug jack — returns to built-in |
| 7 | Repeat steps 3–6 **20 times** |

**Variations:**

- Unplug jack mid-word (during playback) — next key should still speak.
- Rapid plug/unplug 10 times in under 30 s — audio should settle within a few seconds.

---

## 9. Settings toggles under load

**Goal:** Disabling BLE or changing settings mid-session does not corrupt connection state.

| Scenario | Cycles |
|----------|--------|
| BLE keyboard connected → toggle Bluetooth Keyboard off → on → reconnect | 5 |
| BLE computer connected → toggle host Bluetooth radio off → on → reconnect | 5 |
| Change hold duration while connected — new duration applies on next hold | 3 |
| Change volume while keys are speaking — level changes without crash | 3 |

---

## 10. Persistence across reboot

**Goal:** Saved settings and last-known BLE keyboard survive power cycle.

| Step | Action |
|------|--------|
| 1 | Set volume and hold duration to non-default values |
| 2 | Connect BLE keyboard (paired via search) |
| 3 | Power off device completely; power on |
| 4 | Volume and hold duration restored; Bluetooth Keyboard toggle still on |
| 5 | BLE keyboard auto-reconnects in background; main screen shows **Connected** when link is up, **Searching...** while reconnecting |
| 6 | Press keys — speech works |
| 7 | Repeat power cycle **5 times** without reconfiguring |

---

## 11. Functional regression spot-checks

Run once after completing §§1–10 (or after any FAIL + fix). These are spec behaviours that should hold regardless of connection path.

### Speech and layout

- [ ] US or UK layout keys speak the expected character when layout is known
- [ ] Unknown layout keys speak physical name (e.g. “left bracket key”), not a guessed character
- [ ] Modifier keys (Shift, Ctrl, etc.) speak on press; hold behaviour matches normal keys

### Hold-to-send

- [ ] One hold = one computer keypress (including modifier combos)
- [ ] Release and re-hold same key = another single keypress
- [ ] Hold duration slider min/max both work

### UI

- [ ] Battery indicator updates; shows charging when plugged in
- [ ] Errors (e.g. missing audio module) show clearly on main screen
- [ ] Screen touches respond within ~50 ms

### Audio / TTS

- [ ] Missing or corrupt SD `/audio` — debug build shows failure on Debug screen; production should degrade gracefully without crash

---

## 12. Soak test (optional, pre-release)

Run for **30–60 minutes** unattended:

1. Connect USB keyboard + BLE computer (or your most common real-world pairing).
2. Every 60 s: press a random key (short tap).
3. Every 5 min: hold a key for send.
4. Every 10 min: disconnect and reconnect one link (alternate keyboard/computer).

**Pass:** No crash, no memory-related slowdown, speech latency stays acceptable.

---

## Test log template

Copy per session:

```
Date:
Firmware commit:
Tester:

| Section | Cycles planned | Cycles passed | FAIL at cycle # | Notes |
|---------|----------------|---------------|-----------------|-------|
| 1 USB keyboard | 20 | | | |
| 2 BLE keyboard | 20 | | | |
| 3 USB computer | 20 | | | |
| 4 BLE computer | 20 | | | |
| 5 Dual connections | 10+10+10 | | | |
| 6 USB↔BLE keyboard | 10 | | | |
| 7 USB↔BLE computer | 10 | | | |
| 8 Speaker plug/unplug | 20 | | | |
| 9 Settings under load | — | | | |
| 10 Reboot persistence | 5 | | | |
| 11 Spot-checks | — | | | |
| 12 Soak | — | | | |
```

---

## Out of scope / not yet in firmware

- Automated hardware-in-the-loop tests — this document is manual only; unit tests in the repo complement but do not replace these hardware cycles.

**Factory reset:** Settings → **Factory Defaults** clears keyboard bonds independently of computer/output bonds. After reset, re-pair the BLE keyboard via **Search for keyboard**.
