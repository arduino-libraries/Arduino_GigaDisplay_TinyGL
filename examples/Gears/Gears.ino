#include "TinyGL.h"
extern "C" {
#include "zbuffer.h"
#include "GL/gl.h"
}
#include "Portenta_lvgl.h"
#include "SDRAM.h"


void LCD_ST7701_Init();

uint16_t touchpad_x;
uint16_t touchpad_y;
bool touchpad_pressed = false;

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
  if (touchpad_pressed) {
    data->point.x = touchpad_x;
    data->point.y = touchpad_y;
    data->state = LV_INDEV_STATE_PRESSED;
    touchpad_pressed = false;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void i2c_touch_setup();
void i2c_touch_loop();
extern "C" void init_gears();
extern "C" void draw_gears();
extern "C" void idle_gears();
extern "C" void reshape(int width, int height);

extern "C" void tkSwapBuffers(void)
{

}

ZBuffer* frameBuffer;

#ifdef ARDUINO_GIGA
#define WINDOWX 800
#define WINDOWY 480
#else
#define WINDOWX 720
#define WINDOWY 480
#endif

static lv_obj_t * canvas;
static lv_color_t* cbuf;

static void anim(lv_timer_t * timer) {
  draw_gears();
  idle_gears();
  lv_obj_invalidate(canvas);
}

void setup() {
  Serial.begin(115200);
  //while (!Serial);

  //i2c_touch_setup();
#ifdef ARDUINO_GIGA
  giga_init_video(true);
  LCD_ST7701_Init();
#else
  portenta_init_video();
#endif

  cbuf = (lv_color_t*)ea_malloc(WINDOWX * WINDOWY * 4);

  canvas = lv_canvas_create(lv_scr_act());
  lv_canvas_set_buffer(canvas, cbuf, WINDOWX, WINDOWY, LV_IMG_CF_TRUE_COLOR);

  auto frameBuffer = ZB_open(WINDOWX, WINDOWY, ZB_MODE_5R6G5B, 0, NULL, NULL, cbuf);
  glInit(frameBuffer);

  init_gears();
  reshape(WINDOWX, WINDOWY);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_input_read;
  /*Register the driver in LVGL and save the created input device object*/
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);

  lv_timer_create(anim, 32, NULL);
}

void loop() {
  //i2c_touch_loop();
#if LVGL_VERSION_MAJOR > 7
  lv_timer_handler();
#else
  lv_task_handler();
#endif
}