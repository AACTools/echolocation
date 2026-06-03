#include "lvgl_port.h"

#ifndef NATIVE_TEST

#include <M5GFX.h>
#include <M5Unified.h>
#include <lvgl.h>

namespace echo {

namespace {

constexpr int kHorRes = 320;
constexpr int kVerRes = 240;
constexpr int kDrawBufLines = 80;

lv_disp_draw_buf_t draw_buf;
lv_color_t* buf1 = nullptr;
lv_color_t* buf2 = nullptr;
lv_disp_drv_t disp_drv;
lv_indev_drv_t indev_drv;

void dispFlush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;

  M5.Display.startWrite();
  M5.Display.setAddrWindow(area->x1, area->y1, w, h);
  M5.Display.writePixels(reinterpret_cast<lgfx::swap565_t*>(&color_p->full),
                         static_cast<uint32_t>(w) * static_cast<uint32_t>(h));
  M5.Display.endWrite();
  lv_disp_flush_ready(disp_drv);
}

void touchRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
  (void)indev_drv;
  lgfx::touch_point_t tp[1];

  M5.update();
  data->state = LV_INDEV_STATE_REL;
  if (M5.Display.getTouchRaw(tp, 1) == 0) {
    return;
  }

  data->state = LV_INDEV_STATE_PR;
  data->point.x = tp[0].x;
  data->point.y = tp[0].y;
}

}  // namespace

void lvglPortInit() {
  lv_init();

  const size_t buf_pixels =
      static_cast<size_t>(kHorRes) * static_cast<size_t>(kDrawBufLines);
  const size_t buf_bytes = buf_pixels * sizeof(lv_color_t);

  buf1 = static_cast<lv_color_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM));
  buf2 = static_cast<lv_color_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM));
  if (buf1 == nullptr) {
    buf1 = static_cast<lv_color_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_8BIT));
  }
  if (buf2 == nullptr) {
    buf2 = buf1;
  }

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = kHorRes;
  disp_drv.ver_res = kVerRes;
  disp_drv.flush_cb = dispFlush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchRead;
  lv_indev_drv_register(&indev_drv);
}

void lvglPortTick() {
  static uint32_t last_ms = 0;
  const uint32_t now = millis();
  if (last_ms == 0) {
    last_ms = now;
  }
  lv_tick_inc(now - last_ms);
  last_ms = now;
  lv_timer_handler();
}

void lvglPortForceRefresh() {
  lv_refr_now(nullptr);
}

}  // namespace echo

#endif
