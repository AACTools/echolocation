#include "usb_hid_computer.h"

#include "key_event.h"

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include "esp32-hal-tinyusb.h"
#endif

namespace echo {

#ifndef NATIVE_TEST
static USBHIDKeyboard g_usb_keyboard;
#endif

static uint8_t hidUsageToKeyCode(uint8_t hid_usage) {
  if (hid_usage >= 0x04 && hid_usage <= 0x1D) {
    return static_cast<uint8_t>('a' + (hid_usage - 0x04));
  }
  switch (hid_usage) {
    case 0x28:
      return KEY_RETURN;
    case 0x29:
      return KEY_ESC;
    case 0x2A:
      return KEY_BACKSPACE;
    case 0x2B:
      return KEY_TAB;
    case 0x2C:
      return ' ';
    case kLeftControl:
      return KEY_LEFT_CTRL;
    case kLeftShift:
      return KEY_LEFT_SHIFT;
    case kLeftAlt:
      return KEY_LEFT_ALT;
    case kLeftGui:
      return KEY_LEFT_GUI;
    case kRightControl:
      return KEY_RIGHT_CTRL;
    case kRightShift:
      return KEY_RIGHT_SHIFT;
    case kRightAlt:
      return KEY_RIGHT_ALT;
    case kRightGui:
      return KEY_RIGHT_GUI;
    default:
      return 0;
  }
}

#ifndef NATIVE_TEST
static void pressModifierMask(uint8_t modifier_mask) {
  if (modifier_mask & 0x01) g_usb_keyboard.press(KEY_LEFT_CTRL);
  if (modifier_mask & 0x02) g_usb_keyboard.press(KEY_LEFT_SHIFT);
  if (modifier_mask & 0x04) g_usb_keyboard.press(KEY_LEFT_ALT);
  if (modifier_mask & 0x08) g_usb_keyboard.press(KEY_LEFT_GUI);
  if (modifier_mask & 0x10) g_usb_keyboard.press(KEY_RIGHT_CTRL);
  if (modifier_mask & 0x20) g_usb_keyboard.press(KEY_RIGHT_SHIFT);
  if (modifier_mask & 0x40) g_usb_keyboard.press(KEY_RIGHT_ALT);
  if (modifier_mask & 0x80) g_usb_keyboard.press(KEY_RIGHT_GUI);
}
#endif

void usbHidComputerBegin() {
#ifndef NATIVE_TEST
  g_usb_keyboard.begin();
#endif
}

void usbHidComputerTick(bool& connected) {
#ifndef NATIVE_TEST
  connected = tud_mounted();
#else
  connected = false;
#endif
}

void usbHidComputerSendTap(uint8_t hid_usage, uint8_t modifier_mask) {
#ifndef NATIVE_TEST
  const uint8_t key_code = hidUsageToKeyCode(hid_usage);
  if (key_code == 0) {
    return;
  }
  pressModifierMask(modifier_mask);
  g_usb_keyboard.press(key_code);
  delay(5);
  g_usb_keyboard.release(key_code);
  g_usb_keyboard.releaseAll();
#else
  (void)hid_usage;
  (void)modifier_mask;
#endif
}

}  // namespace echo
