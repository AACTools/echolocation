#include "usb_keyboard.h"

#include "ui.h"
#include "key_audio.h"

#include <Arduino.h>
#include <SPI.h>
#include <Usb.h>
#include <hidboot.h>

namespace {

constexpr int kLcdCs = 3;
constexpr int kUsbHostInt = 10;

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);

bool usb_host_ready = false;
bool last_reported_connected = false;
uint8_t displayed_key = 0;
uint8_t displayed_mod = 0;
uint8_t held_key = 0;
uint8_t held_mod = 0;
unsigned long key_pressed_at = 0;
bool box_shown = false;

const char* hidKeyName(uint8_t key) {
  switch (key) {
    case 0x28:
      return "Enter";
    case 0x29:
      return "Esc";
    case 0x2A:
      return "Backspace";
    case 0x2B:
      return "Tab";
    case 0x2C:
      return "Space";
    case 0x39:
      return "Caps";
    case 0x3A:
      return "F1";
    case 0x3B:
      return "F2";
    case 0x3C:
      return "F3";
    case 0x3D:
      return "F4";
    case 0x3E:
      return "F5";
    case 0x3F:
      return "F6";
    case 0x40:
      return "F7";
    case 0x41:
      return "F8";
    case 0x42:
      return "F9";
    case 0x43:
      return "F10";
    case 0x44:
      return "F11";
    case 0x45:
      return "F12";
    case 0x4F:
      return "Right";
    case 0x50:
      return "Left";
    case 0x51:
      return "Down";
    case 0x52:
      return "Up";
    case 0x53:
      return "Num";
    default:
      return nullptr;
  }
}

class KeyboardParser : public KeyboardReportParser {
 public:
  void OnKeyDown(uint8_t mod, uint8_t key) override {
    char label[16];
    keyToLabel(mod, key, label, sizeof(label));
    if (label[0] == '\0') {
      return;
    }

    const bool is_new_key = (key != displayed_key || mod != displayed_mod);
    if (is_new_key) {
      displayed_key = key;
      displayed_mod = mod;
      box_shown = false;
      uiSetKeyBoxOutline(false);
      uiSetPressedKey(label);
    }

    if (held_key != key || held_mod != mod) {
      held_key = key;
      held_mod = mod;
      key_pressed_at = millis();
      if (!is_new_key) {
        box_shown = false;
        uiSetKeyBoxOutline(false);
      }
    }

    keyAudioPlayForLabel(label);
  }

  void OnKeyUp(uint8_t mod, uint8_t key) override {
    if (key == held_key && mod == held_mod) {
      held_key = 0;
      held_mod = 0;
    }
  }

 private:
  void keyToLabel(uint8_t mod, uint8_t key, char* out, size_t out_len) {
    if (key == 0 || out_len == 0) {
      if (out_len > 0) {
        out[0] = '\0';
      }
      return;
    }

    const uint8_t ascii = OemToAscii(mod, key);
    if (ascii >= 0x20 && ascii <= 0x7E) {
      out[0] = static_cast<char>(ascii);
      out[1] = '\0';
      return;
    }

    const char* name = hidKeyName(key);
    if (name != nullptr) {
      strncpy(out, name, out_len - 1);
      out[out_len - 1] = '\0';
      return;
    }

    out[0] = '\0';
  }
};

KeyboardParser keyboard_parser;

int8_t initMax3421SharedSpi() {
  pinMode(kLcdCs, OUTPUT);
  digitalWrite(kLcdCs, HIGH);

  P1::SetDirWrite();
  P1::Set();

  pinMode(kUsbHostInt, INPUT_PULLUP);

  Usb.regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL));
  if (Usb.reset() == 0) {
    return -1;
  }

  Usb.regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST);
  Usb.regWr(rHIEN, bmCONDETIE | bmFRAMEIE);
  Usb.regWr(rHCTL, bmSAMPLEBUS);
  while ((Usb.regRd(rHCTL) & bmSAMPLEBUS) == 0) {
  }
  Usb.busprobe();
  Usb.regWr(rHIRQ, bmCONDETIRQ);
  Usb.regWr(rCPUCTL, 0x01);
  return 0;
}

}  // namespace

void usbKeyboardBegin() {
  if (usb_host_ready) {
    return;
  }
  usb_host_ready = (initMax3421SharedSpi() == 0);
  if (usb_host_ready) {
    HidKeyboard.SetReportParser(0, &keyboard_parser);
  }
}

void usbKeyboardTick() {
  if (!usb_host_ready) {
    return;
  }

  if (digitalRead(kUsbHostInt) == LOW) {
    Usb.IntHandler();
  }
  Usb.Task();

  if (held_key != 0 && !box_shown && held_key == displayed_key &&
      held_mod == displayed_mod &&
      millis() - key_pressed_at >= uiGetHoldDurationMs()) {
    box_shown = true;
    uiSetKeyBoxOutline(true);
  }

  const bool connected = HidKeyboard.isReady();
  if (connected == last_reported_connected) {
    return;
  }

  last_reported_connected = connected;
  uiSetKeyboardConnected(connected);
}

bool usbKeyboardIsConnected() { return HidKeyboard.isReady(); }
