#pragma once

namespace echo {

/** Initialize LVGL display and touch drivers (call after M5.begin and display rotation). */
void lvglPortInit();

/** Call every loop iteration: advances LVGL tick and runs the task handler. */
void lvglPortTick();

/** Force an immediate LVGL redraw (call after building screens). */
void lvglPortForceRefresh();

}  // namespace echo
