#include "usb_keyboard.h"

#include "ui.h"

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
    displayed_key_ = key;
    uiSetPressedKey(label);
  }

  void OnKeyUp(uint8_t mod, uint8_t key) override {
    (void)mod;
    if (key == displayed_key_) {
      displayed_key_ = 0;
      uiSetPressedKey(nullptr);
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

  uint8_t displayed_key_ = 0;
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

  const bool connected = HidKeyboard.isReady();
  if (connected == last_reported_connected) {
    return;
  }

  last_reported_connected = connected;
  uiSetKeyboardConnected(connected);
}

bool usbKeyboardIsConnected() { return HidKeyboard.isReady(); }
