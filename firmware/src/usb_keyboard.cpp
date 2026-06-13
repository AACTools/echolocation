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
