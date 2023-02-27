#include "TinyGL.h"
extern "C" {
#include "zbuffer.h"
#include "GL/gl.h"
}
#include "Portenta_lvgl.h"
#include "dsi.h"
#include "SDRAM.h"
#include "Goodix.h"  // Arduino_GT911_Library

void LCD_ST7701_Init();

uint16_t touchpad_x;
uint16_t touchpad_y;

void i2c_touch_setup();
void i2c_touch_loop();
extern "C" void init_gears();
extern "C" void draw_gears();
extern "C" void idle_gears();
extern "C" void reshape(int width, int height);
extern "C" void gears_rotate(float x, float y, float z);

extern "C" void tkSwapBuffers(void)
{

}

GTPoint prev_points[3];
float zoom_scale = 1.0f;
uint32_t first_touch = 0;

void handleTouch(int8_t contacts, GTPoint *points) {
  if (first_touch == 0 || millis() - first_touch > 500) {
    first_touch = millis();
    goto save;
  }
  if (contacts == 1) {
    // rotate the scene
    if (abs(prev_points[0].x - points[0].x) + abs(prev_points[0].y - points[0].y) > 50) {
      goto save;
    }
    gears_rotate(prev_points[0].x - points[0].x,
                 prev_points[0].y - points[0].y,
                 0);
  }
  if (contacts == 2) {
    // pinch to zoom
    if (abs(abs(prev_points[0].x - prev_points[1].x) * abs(prev_points[0].y - prev_points[1].y) - abs(points[0].x - points[1].x) * abs(points[0].y - points[1].y)) < 5) {
      goto save;
    }

    auto area_now = abs((int)points[0].x - (int)points[1].x) * abs((int)points[0].y - (int)points[1].y);
    auto area_before = abs((int)prev_points[0].x - (int)prev_points[1].x) * abs((int)prev_points[0].y - (int)prev_points[1].y);

    if (area_now < area_before) {
      zoom_scale = 0.95f;
    } else {
      zoom_scale = 1.05f;
    }
    glScalef(zoom_scale, zoom_scale, zoom_scale);
  }
  if (contacts == 3) {
    // slide the scene
  }
save:
  for (int i = 0; i < contacts; i++) {
    prev_points[i].x = points[i].x;
    prev_points[i].y = points[i].y;
  }
}

ZBuffer* frameBuffer;

int WINDOWX = 800;
int WINDOWY = 480;

static lv_obj_t * canvas;
static lv_color_t* cbuf;

#define LANDSCAPE true

static void anim(lv_timer_t * timer) {
  draw_gears();
  idle_gears();
  lv_obj_invalidate(canvas);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

#ifdef ARDUINO_GIGA
  i2c_touch_setup();
  giga_init_video(LANDSCAPE);
  LCD_ST7701_Init();
#else
  portenta_init_video();
#endif

  if (LANDSCAPE) {
    WINDOWX = stm32_getYSize();
    WINDOWY = stm32_getXSize();
  } else {
    WINDOWX = stm32_getXSize();
    WINDOWY = stm32_getYSize();
  }

  cbuf = (lv_color_t*)ea_malloc(WINDOWX * WINDOWY * 4);

  canvas = lv_canvas_create(lv_scr_act());
  lv_canvas_set_buffer(canvas, cbuf, WINDOWX, WINDOWY, LV_IMG_CF_TRUE_COLOR);

  auto frameBuffer = ZB_open(WINDOWX, WINDOWY, ZB_MODE_5R6G5B, 0, NULL, NULL, cbuf);
  glInit(frameBuffer);

  init_gears();
  reshape(WINDOWX, WINDOWY);

  lv_timer_create(anim, 10, NULL);
}

void loop() {
#ifdef ARDUINO_GIGA
  i2c_touch_loop();
#endif
#if LVGL_VERSION_MAJOR > 7
  lv_timer_handler();
#else
  lv_task_handler();
#endif
}
