#include "usb_keyboard_source.h"

#include "boot_key_map.h"

#ifndef NATIVE_TEST
#include <SPI.h>
#include <usbhub.h>
#include <hidboot.h>

#if ECHOLOCATION_USB_HOST_PINS_CORES3
#define USB_HOST_MOSI 36
#define USB_HOST_MISO 37
#define USB_HOST_SCK 35
#define USB_HOST_CS 1
#define USB_HOST_INT 10
#else
#define USB_HOST_MOSI 18
#define USB_HOST_MISO 23
#define USB_HOST_SCK 19
#define USB_HOST_CS 5
#define USB_HOST_INT 35
#endif

namespace usb_host_shield {

echo::KeyboardEventCallback* g_keyboard_callback = nullptr;

class EcholocationKeyboardParser : public KeyboardReportParser {
 protected:
  void OnKeyDown(uint8_t modifier, uint8_t key) override {
    emit(modifier, key, true);
  }

  void OnKeyUp(uint8_t modifier, uint8_t key) override {
    emit(modifier, key, false);
  }

  void OnControlKeysChanged(uint8_t before, uint8_t after) override {
    const uint8_t changed = before ^ after;
    for (uint8_t bit = 0x01; bit != 0; bit <<= 1) {
      if ((changed & bit) == 0) {
        continue;
      }
      const bool pressed = (after & bit) != 0;
      const uint8_t usage = echo::bootModifierToHidUsage(bit);
      if (usage == 0 || !g_keyboard_callback || !*g_keyboard_callback) {
        continue;
      }
      echo::KeyEvent event;
      event.hid_usage = usage;
      event.modifier_mask = after;
      event.pressed = pressed;
      event.timestamp_ms = millis();
      (*g_keyboard_callback)(event);
    }
  }

 private:
  void emit(uint8_t modifier, uint8_t key, bool pressed) {
    if (!g_keyboard_callback || !*g_keyboard_callback || key == 0) {
      return;
    }
    const uint8_t usage = echo::bootKeyToHidUsage(key);
    if (usage == 0) {
      return;
    }
    echo::KeyEvent event;
    event.hid_usage = usage;
    event.modifier_mask = modifier;
    event.pressed = pressed;
    event.timestamp_ms = millis();
    (*g_keyboard_callback)(event);
  }
};

USB Usb;
USBHub Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
EcholocationKeyboardParser KeyboardParser;

}  // namespace usb_host_shield

#endif

namespace echo {

void UsbKeyboardSource::begin(KeyboardEventCallback callback) {
  callback_ = std::move(callback);
#ifndef NATIVE_TEST
  usb_host_shield::g_keyboard_callback = &callback_;
  SPI.begin(USB_HOST_SCK, USB_HOST_MISO, USB_HOST_MOSI, USB_HOST_CS);
  if (usb_host_shield::Usb.Init() == -1) {
    connected_ = false;
    return;
  }
  delay(200);
  usb_host_shield::HidKeyboard.SetReportParser(
      0, &usb_host_shield::KeyboardParser);
  connected_ = true;
#endif
}

void UsbKeyboardSource::tick(uint32_t now_ms) {
  (void)now_ms;
#ifndef NATIVE_TEST
  usb_host_shield::Usb.Task();
  connected_ = usb_host_shield::HidKeyboard.isReady();
#endif
}

bool UsbKeyboardSource::isKeyboardConnected() const { return connected_; }

}  // namespace echo
