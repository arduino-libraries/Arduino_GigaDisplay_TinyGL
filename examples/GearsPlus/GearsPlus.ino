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
extern "C" void init_mech();
extern "C" void display_mech();
extern "C" void idle_mech();
extern "C" void mech_rotate(float x, float y, float z);
extern "C" GLenum key_mech(int key, GLenum mask);
extern "C" void reshape_gears(int width, int height);
extern "C" void reshape_mech(int width, int height);
extern "C" void gears_rotate(float x, float y, float z);


#define GEARS     0
#define MECH      1

//#define MODEL     GEARS
#define MODEL   MECH

#if MODEL == GEARS
#define RESHAPE   reshape_gears
#define INIT   init_gears
#define IDLE   idle_gears
#define DRAW   draw_gears
#define ROTATE   gears_rotate
#define LANDSCAPE true
#endif

#if MODEL == MECH
#define RESHAPE   reshape_mech
#define INIT   init_mech
#define IDLE   idle_mech
#define DRAW   display_mech
#define ROTATE   mech_rotate
#define LANDSCAPE false
#endif


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
    ROTATE(prev_points[0].x - points[0].x,
                 prev_points[0].y - points[0].y,
                 0);
  }
  if (contacts == 2) {
    // pinch to zoom
    if (abs(abs(prev_points[0].x - prev_points[1].x) * abs(prev_points[0].y - prev_points[1].y) - abs(points[0].x - points[1].x) * abs(points[0].y - points[1].y)) < 5) {
      goto save;
    }

    if (abs(abs((uint32_t)points[0].x - (uint32_t)points[1].x) * abs((uint32_t)points[0].y - (uint32_t)points[1].y)) <
        abs(abs((uint32_t)prev_points[0].x - (uint32_t)prev_points[1].x) * abs((uint32_t)prev_points[0].y - (uint32_t)prev_points[1].y))) {
      zoom_scale = 1.05f;
    } else {
      zoom_scale = 0.95f;
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

static void anim(lv_timer_t * timer) {
  DRAW();
  IDLE();
  lv_obj_invalidate(canvas);
}

#include "DigitalOut.h"
mbed::DigitalOut swire(PB_12, 1);

void setup() {
  Serial.begin(115200);
  //while (!Serial);

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

  INIT();
  RESHAPE(WINDOWX, WINDOWY);

  lv_timer_create(anim, 35, NULL);
}

void loop() {
  if (Serial.available()) {
    key_mech(Serial.read(), 0);
  }
#ifdef ARDUINO_GIGA
  i2c_touch_loop();
#endif
#if LVGL_VERSION_MAJOR > 7
  lv_timer_handler();
#else
  lv_task_handler();
#endif
}
