#include "usb_keyboard_source.h"

#include "boot_key_map.h"

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <SPI.h>
#include <Usb.h>
#include <hidboot.h>
#include <usbhub.h>

#if defined(ARDUINO_M5STACK_CORES3)
#define USB_HOST_SCK 36
#define USB_HOST_MISO 35
#define USB_HOST_MOSI 37
#define USB_HOST_CS 1
#else
#define USB_HOST_MOSI 18
#define USB_HOST_MISO 23
#define USB_HOST_SCK 19
#define USB_HOST_CS 5
#endif

namespace usb_host_shield {

echo::KeyboardEventCallback* g_keyboard_callback = nullptr;

bool isVbusAttached(uint8_t vbus_state) {
  return vbus_state == FSHOST || vbus_state == LSHOST;
}

class EcholocationKeyboardParser : public KeyboardReportParser {
 public:
  void resetPrevState() {
    for (uint8_t i = 0; i < sizeof(prevState.bInfo); ++i) {
      prevState.bInfo[i] = 0;
    }
  }

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
  KeyboardEventCallback user_callback = std::move(callback);
  callback_ = [this, user_callback](const KeyEvent& event) {
    connected_ = true;
    last_activity_ms_ = millis();
    if (user_callback) {
      user_callback(event);
    }
  };
#ifndef NATIVE_TEST
  usb_host_shield::g_keyboard_callback = &callback_;
  SPI.begin(USB_HOST_SCK, USB_HOST_MISO, USB_HOST_MOSI, USB_HOST_CS);
  usb_host_init_ok_ = (usb_host_shield::Usb.Init() != -1);
  if (!usb_host_init_ok_) {
    connected_ = false;
    hid_ready_ = false;
    return;
  }
  delay(10);
  usb_host_shield::HidKeyboard.SetReportParser(0, &usb_host_shield::KeyboardParser);
  connected_ = false;
  hid_ready_ = false;
  prev_hid_ready_ = false;
  last_activity_ms_ = 0;
  last_reinit_ms_ = millis();
#endif
}

void UsbKeyboardSource::tick(uint32_t now_ms) {
#ifndef NATIVE_TEST
  usb_host_shield::Usb.Task();
  usb_task_state_ = usb_host_shield::Usb.getUsbTaskState();
  usb_vbus_state_ = usb_host_shield::Usb.getVbusState();
  hid_ready_ = usb_host_shield::HidKeyboard.isReady();

  if (hid_ready_ && !prev_hid_ready_) {
    usb_host_shield::KeyboardParser.resetPrevState();
    usb_host_shield::HidKeyboard.SetReportParser(0, &usb_host_shield::KeyboardParser);
  }
  prev_hid_ready_ = hid_ready_;

  connected_ = usb_host_shield::isVbusAttached(usb_vbus_state_);

  if (usb_task_state_ == USB_DETACHED_SUBSTATE_ILLEGAL &&
      (now_ms - last_reinit_ms_ >= kIllegalStateReinitMs)) {
    usb_host_init_ok_ = (usb_host_shield::Usb.Init() != -1);
    if (usb_host_init_ok_) {
      usb_host_shield::HidKeyboard.SetReportParser(0, &usb_host_shield::KeyboardParser);
      prev_hid_ready_ = false;
    }
    last_reinit_ms_ = now_ms;
  }
#else
  (void)now_ms;
#endif
}

bool UsbKeyboardSource::isKeyboardConnected() const { return connected_; }
uint8_t UsbKeyboardSource::usbTaskState() const { return usb_task_state_; }
bool UsbKeyboardSource::isHidReady() const { return hid_ready_; }
uint32_t UsbKeyboardSource::lastActivityMs() const { return last_activity_ms_; }
bool UsbKeyboardSource::usbHostInitOk() const { return usb_host_init_ok_; }
uint8_t UsbKeyboardSource::usbVbusState() const { return usb_vbus_state_; }

}  // namespace echo
