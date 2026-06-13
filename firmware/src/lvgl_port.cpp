#include "lvgl_port.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

namespace {

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;

uint32_t lvglTick() { return millis(); }

void displayFlush(lv_display_t* display, const lv_area_t* area,
                  uint8_t* px_map) {
  const uint32_t width = area->x2 - area->x1 + 1;
  const uint32_t height = area->y2 - area->y1 + 1;
  lv_draw_sw_rgb565_swap(px_map, width * height);
  M5.Display.pushImageDMA(area->x1, area->y1, width, height,
                          reinterpret_cast<uint16_t*>(px_map));
  lv_display_flush_ready(display);
}

void touchRead(lv_indev_t* indev, lv_indev_data_t* data) {
  (void)indev;
  lgfx::touch_point_t touch_point;
  if (M5.Display.getTouch(&touch_point)) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touch_point.x;
    data->point.y = touch_point.y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

}  // namespace

void lvglPortInit() {
  M5.Display.setRotation(1);
  M5.Display.setColorDepth(16);

  lv_init();
  lv_tick_set_cb(lvglTick);

  const size_t draw_buf_pixels =
      static_cast<size_t>(kDisplayWidth) * kDisplayHeight / 10;
  const size_t draw_buf_bytes = draw_buf_pixels * sizeof(lv_color_t);
  void* draw_buf = heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_INTERNAL);
  if (draw_buf == nullptr) {
    draw_buf = heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_SPIRAM);
  }

  lv_display_t* display = lv_display_create(kDisplayWidth, kDisplayHeight);
  lv_display_set_flush_cb(display, displayFlush);
  lv_display_set_buffers(display, draw_buf, nullptr, draw_buf_bytes,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_theme_t* theme = lv_theme_default_init(
      display, lv_color_hex(0x0066FF), lv_color_hex(0xFF4444), true,
      LV_FONT_DEFAULT);
  lv_display_set_theme(display, theme);

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchRead);
}

void lvglPortTick() { lv_timer_handler(); }
