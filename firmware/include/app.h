#pragma once

namespace echo {

class App {
 public:
  void setup();
  void loop();

 private:
  void handleKeyEvent(const struct KeyEvent& event);
  void refreshUi();
  void applySettingsIfNeeded();
};

void appSetup();
void appLoop();

}  // namespace echo
