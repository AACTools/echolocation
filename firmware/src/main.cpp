#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>

#include "device_settings_store.h"
#include "ble_keyboard_input.h"
#include "computer_output.h"
#include "key_audio.h"
#include "lvgl_port.h"
#include "speaker_detect.h"
#include "speaker_route.h"
#include "key_config.h"
#include "ui.h"
#include "usb_keyboard.h"

namespace {

constexpr uint32_t kBatteryUpdateIntervalMs = 2000;
constexpr uint32_t kLoadingPumpSliceMs = 30;

// CoreS3 shared SPI chip-selects (idle high).
constexpr int kLcdCsPin = 3;
constexpr int kSdCsPin = 4;
constexpr int kUsbHostSsPin = 1;  // MAX3421 SS on USB module

// AW9523B — bus / boost gates for stacked modules.
constexpr uint8_t kAw9523Addr = 0x58;
constexpr uint8_t kAw9523Port0 = 0x02;
constexpr uint8_t kAw9523Port1 = 0x03;
constexpr uint8_t kBusOutEnBit = 0b00000010;     // BUS_OUT 5V to M-Bus
constexpr uint8_t kBoostEnBit = 0b10000000;      // SY7088 BOOST_EN
constexpr uint32_t kAw9523I2cHz = 400000;

// AXP2101 — CoreS3 backlight (DLDO1).
constexpr uint8_t kAxp2101Addr = 0x34;
constexpr uint8_t kAxpLdoOnOffReg = 0x90;
constexpr uint8_t kAxpDldo1Bit = 0x80;
constexpr uint8_t kAxpDldo1VoltageReg = 0x99;
constexpr uint32_t kAxpI2cHz = 400000;

void pumpLoadingUi() {
  const uint32_t end_ms = millis() + kLoadingPumpSliceMs;
  while (millis() < end_ms) {
    uiPump();
    delay(5);
  }
}

void updateBatteryStatus() {
  static uint32_t last_update_ms = 0;
  const uint32_t now_ms = millis();
  if (now_ms - last_update_ms < kBatteryUpdateIntervalMs) {
    return;
  }
  last_update_ms = now_ms;

  const int level = M5.Power.getBatteryLevel();
  const bool charging =
      M5.Power.isCharging() == m5::Power_Class::is_charging;
  uiSetBattery(level, charging);
}

// On battery the USB host module can brown out and load the shared SPI bus
// (MOSI/SCK/MISO), which corrupts LCD traffic after a soft reset. Force
// BOOST + BUS_OUT 5V and hold every CS high before talking to the panel.
void prepareSharedSpiBus() {
  M5.Power.setExtOutput(true);
  M5.In_I2C.bitOn(kAw9523Addr, kAw9523Port1, kBoostEnBit, kAw9523I2cHz);
  M5.In_I2C.bitOn(kAw9523Addr, kAw9523Port0, kBusOutEnBit, kAw9523I2cHz);

  pinMode(kLcdCsPin, OUTPUT);
  digitalWrite(kLcdCsPin, HIGH);
  pinMode(kSdCsPin, OUTPUT);
  digitalWrite(kSdCsPin, HIGH);
  pinMode(kUsbHostSsPin, OUTPUT);
  digitalWrite(kUsbHostSsPin, HIGH);

  // Battery → 5V boost needs a moment after RST before SPI is clean.
  delay(80);
}

void enableBacklight(uint8_t brightness) {
  const uint8_t voltage =
      brightness == 0 ? 0
                      : static_cast<uint8_t>((brightness + 641) >> 5);
  if (brightness == 0) {
    M5.In_I2C.bitOff(kAxp2101Addr, kAxpLdoOnOffReg, kAxpDldo1Bit, kAxpI2cHz);
  } else {
    M5.In_I2C.bitOn(kAxp2101Addr, kAxpLdoOnOffReg, kAxpDldo1Bit, kAxpI2cHz);
  }
  M5.In_I2C.writeRegister8(kAxp2101Addr, kAxpDldo1VoltageReg, voltage,
                           kAxpI2cHz);
  M5.Display.setBrightness(brightness);
}

void writeLcdCommand(uint8_t cmd) { M5.Display.writeCommand(cmd); }

void writeLcdData(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    M5.Display.writeData(data[i]);
  }
}

void writeLcdCommandWithData(uint8_t cmd, const uint8_t* data, size_t len) {
  writeLcdCommand(cmd);
  writeLcdData(data, len);
}

void sendIli9342InitCommands() {
  static constexpr uint8_t kSetExtc[] = {0xFF, 0x93, 0x42};
  static constexpr uint8_t kPwctr1[] = {0x12, 0x12};
  static constexpr uint8_t kPwctr2[] = {0x03};
  static constexpr uint8_t kVmctr1[] = {0xF2};
  static constexpr uint8_t kB0[] = {0xE0};
  static constexpr uint8_t kF6[] = {0x01, 0x00, 0x00};
  static constexpr uint8_t kGmctrp1[] = {0x00, 0x0C, 0x11, 0x04, 0x11, 0x08,
                                         0x37, 0x89, 0x4C, 0x06, 0x0C, 0x0A,
                                         0x2E, 0x34, 0x0F};
  static constexpr uint8_t kGmctrn1[] = {0x00, 0x0B, 0x11, 0x05, 0x13, 0x09,
                                         0x33, 0x67, 0x48, 0x07, 0x0E, 0x0B,
                                         0x2E, 0x33, 0x0F};
  static constexpr uint8_t kDfunctr[] = {0x08, 0x82, 0x1D, 0x04};

  M5.Display.startWrite();
  writeLcdCommandWithData(0xC8, kSetExtc, sizeof(kSetExtc));
  writeLcdCommandWithData(0xC0, kPwctr1, sizeof(kPwctr1));
  writeLcdCommandWithData(0xC1, kPwctr2, sizeof(kPwctr2));
  writeLcdCommandWithData(0xC5, kVmctr1, sizeof(kVmctr1));
  writeLcdCommandWithData(0xB0, kB0, sizeof(kB0));
  writeLcdCommandWithData(0xF6, kF6, sizeof(kF6));
  writeLcdCommandWithData(0xE0, kGmctrp1, sizeof(kGmctrp1));
  writeLcdCommandWithData(0xE1, kGmctrn1, sizeof(kGmctrn1));
  writeLcdCommandWithData(0xB6, kDfunctr, sizeof(kDfunctr));
  writeLcdCommand(0x38);  // IDMOFF
  writeLcdCommand(0x29);  // DISPON
  writeLcdCommand(0x11);  // SLPOUT
  M5.Display.endWrite();
  delay(120);
}

void reinitPanelOverLiveBus() {
  // SWRESET over SPI — do not pulse AW9523 LCD_RST / panel->init().
  M5.Display.startWrite();
  writeLcdCommand(0x01);
  M5.Display.endWrite();
  delay(200);

  sendIli9342InitCommands();

  M5.Display.invertDisplay(M5.Display.getInvert());
  M5.Display.setColorDepth(16);
  M5.Display.setRotation(1);
  enableBacklight(128);
  M5.Display.fillScreen(TFT_BLACK);
}

// Soft-reset display recovery. Unreliable on battery without BUS_OUT 5V
// because the stacked USB module loads the shared SPI lines.
void recoverDisplayAfterSoftReset() {
  Serial.printf("[boot] reset reason %d\n",
                static_cast<int>(esp_reset_reason()));

  prepareSharedSpiBus();
  enableBacklight(128);
  reinitPanelOverLiveBus();

  // One retry with a longer settle — battery RST is the flaky case.
  const bool likely_battery_only =
      M5.Power.isCharging() != m5::Power_Class::is_charging;
  if (likely_battery_only) {
    Serial.println("[boot] battery path: retrying display recovery");
    delay(100);
    prepareSharedSpiBus();
    reinitPanelOverLiveBus();
  }

  Serial.println("[boot] display recovery done");
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5.begin(cfg);
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[boot] echolocation");
#ifdef ECHOLOCATION_DEBUG
  Serial.println("[boot] debug build");
#endif
  recoverDisplayAfterSoftReset();

  lvglPortInit();
  uiInit();

  uiSetLoadingStatus("Loading settings...");
  pumpLoadingUi();

  uiSetLoadingStatus("Starting computer output...");
  pumpLoadingUi();
  computerOutputBegin();

  uiSetLoadingStatus("Starting Bluetooth keyboard...");
  pumpLoadingUi();
  bleKeyboardInputBegin();

  uiSetLoadingStatus("Loading settings...");
  pumpLoadingUi();
  deviceSettingsLoad();

  uiSetLoadingStatus("Starting USB keyboard...");
  pumpLoadingUi();
  usbKeyboardBegin();

  uiSetLoadingStatus("Loading audio...");
  pumpLoadingUi();
  keyAudioRefresh();
  pumpLoadingUi();

  keyConfigLoad();
  pumpLoadingUi();

  uiSetLoadingStatus("Starting speakers...");
  pumpLoadingUi();
  speakerDetectBegin();
  speakerRouteBegin();

  updateBatteryStatus();
  Serial.println("[boot] ready");
  uiFinishLoading();
}

void loop() {
  M5.update();
  updateBatteryStatus();
  lvglPortTick();
  computerOutputTick();
  bleKeyboardInputTick();
  uiRefreshConnectionFlow();
  uiRefreshBluetoothOutputStatus();
  uiRefreshBluetoothKeyboardStatus();
  if (speakerDetectPoll()) {
    speakerRouteApply();
    uiRefreshSpeakerOutput();
  }
  usbKeyboardTick();
  delay(5);
}
