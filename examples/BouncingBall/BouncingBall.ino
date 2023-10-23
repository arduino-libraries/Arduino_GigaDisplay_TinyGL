#include "TinyGL.h"
extern "C" {
#include "zbuffer.h"
#include "GL/gl.h"
#include "GL/glu.h"
}
#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"
#include "dsi.h"
#include "lvgl.h"
#include "SDRAM.h"

static GLuint TexObj[2];
static GLfloat Angle = 0.0f;

static int cnt = 0, v = 0;


uint16_t touchpad_x;
uint16_t touchpad_y;

extern "C" void init_gears();
extern "C" void draw_gears();
extern "C" void idle_gears();
extern "C" void reshape(int width, int height);
extern "C" void gears_rotate(float x, float y, float z);

extern "C" void tkSwapBuffers(void)
{

}

GDTpoint_t prev_points[3];
float zoom_scale = 1.0f;
uint32_t first_touch = 0;

void handleTouch(uint8_t contacts, GDTpoint_t *points) {
  if (first_touch == 0 || millis() - first_touch > 500) {
    first_touch = millis();
    goto save;
  }
  if (contacts == 1) {
    // rotate the scene
    if (abs(prev_points[0].x - points[0].x) + abs(prev_points[0].y - points[0].y) > 50) {
      goto save;
    }
    glRotatef(atan2f(prev_points[0].y - points[0].y, prev_points[0].x - points[0].x), 0.0, 0.0, -1.0);

    /*
      gears_rotate(prev_points[0].x - points[0].x,
                 prev_points[0].y - points[0].y,
                 0);
    */
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
GLUquadricObj *quad;

#define LANDSCAPE true

static void anim(lv_timer_t * timer) {
  Angle += 2.0;

  if (++cnt == 5) {
    cnt = 0;
    v = !v;
  }
  draw();
  idle();
  lv_obj_invalidate(canvas);
}

#ifdef ARDUINO_GIGA
#ifdef LANDSCAPE
Arduino_H7_Video display(800, 480, GigaDisplayShield);
#else
Arduino_H7_Video display(480, 800, GigaDisplayShield);
#endif
Arduino_GigaDisplayTouch touch;
#else
Arduino_H7_Video display(720, 480, USBCVideo);
#endif

void
draw_glu(void)
{
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glColor3f(1.0, 1.0, 1.0);

  /* draw first polygon */
  glPushMatrix();
  glTranslatef(-1.0, 0.0, 0.0);
  glRotatef(Angle, 0.0, 0.0, 1.0);
  glBindTexture(GL_TEXTURE_2D, TexObj[v]);

  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(-1.0, -1.0);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(1.0, -1.0);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(1.0, 1.0);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(-1.0, 1.0);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();

  /* draw second polygon */
  glPushMatrix();
  glTranslatef(1.0, 0.0, 0.0);
  glRotatef(Angle - 90.0, 0.0, 1.0, 0.0);

  glBindTexture(GL_TEXTURE_2D, TexObj[1 - v]);

  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(-1.0, -1.0);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(1.0, -1.0);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(1.0, 1.0);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(-1.0, 1.0);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  glPopMatrix();

  tkSwapBuffers();
}


/* new window size or exposure */
void
reshape_glu(int width, int height)
{
  glViewport(0, 0, (GLint) width, (GLint) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  /* glOrtho( -3.0, 3.0, -3.0, 3.0, -10.0, 10.0 ); */
  glFrustum(-2.0, 2.0, -2.0, 2.0, 6.0, 20.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -8.0);
}


void bind_texture(int texobj, int image)
{
  static int width = 8, height = 8;
  static int color[2][3] = {
    {255, 0, 0},
    {0, 255, 0},
  };
  GLubyte tex[64][3];
  static GLubyte texchar[2][8 * 8] = {
    {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 1, 1, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 1, 1, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
    },
    {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 2, 2, 0, 0, 0,
      0, 0, 2, 0, 0, 2, 0, 0,
      0, 0, 0, 0, 0, 2, 0, 0,
      0, 0, 0, 0, 2, 0, 0, 0,
      0, 0, 0, 2, 0, 0, 0, 0,
      0, 0, 2, 2, 2, 2, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0
    }
  };

  int i, j;

  glBindTexture(GL_TEXTURE_2D, texobj);

  /* red on white */
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      int p = i * width + j;
      if (texchar[image][(height - i - 1) * width + j]) {
        tex[p][0] = color[image][0];
        tex[p][1] = color[image][1];
        tex[p][2] = color[image][2];
      } else {
        tex[p][0] = 255;
        tex[p][1] = 255;
        tex[p][2] = 255;
      }
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  /* end of texture object */
}


#define COS(X)   cos( (X) * 3.14159/180.0 )
#define SIN(X)   sin( (X) * 3.14159/180.0 )


GLuint Ball;
GLenum Mode;
GLfloat Zrot = 0.0, Zstep = 4.0;
GLfloat Xpos = 0.0, Ypos = 1.0;
GLfloat Xvel = 0.1, Yvel = 0.0;
GLfloat Xmin = -4.0, Xmax = 4.0;
GLfloat Ymin = -3.8, Ymax = 4.0;
GLfloat G = -0.05;



static GLuint make_ball( void )
{
  GLuint list;
  GLfloat a, b;
  GLfloat da = 18.0, db = 18.0;
  GLfloat radius = 1.0;
  GLuint color;
  GLfloat x, y, z;

  list = glGenLists( 1 );

  glNewList( list, GL_COMPILE );

  color = 0;
  for (a = -90.0; a + da <= 90.0; a += da) {

    glBegin( GL_QUAD_STRIP );
    for (b = 0.0; b <= 360.0; b += db) {

      if (color) {
        glColor3f(1, 0, 0); /* TK_SETCOLOR( Mode, TK_RED ); */
      }
      else {
        glColor3f(1, 1, 1); /* TK_SETCOLOR( Mode, TK_WHITE ); */
      }

      x = COS(b) * COS(a);
      y = SIN(b) * COS(a);
      z = SIN(a);
      glVertex3f( x, y, z );

      x = radius * COS(b) * COS(a + da);
      y = radius * SIN(b) * COS(a + da);
      z = radius * SIN(a + da);
      glVertex3f( x, y, z );

      color = 1 - color;
    }
    glEnd();

  }

  glEndList();

  return list;
}



void reshape( int width, int height )
{
  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho( -6.0, 6.0, -6.0, 6.0, -6.0, 6.0 );
  glMatrixMode(GL_MODELVIEW);
}



void draw( void )
{
  GLint i;

  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glColor3f(0, 1, 1); /* TK_SETCOLOR( Mode, TK_CYAN ); */
  glBegin( GL_LINES );
  for (i = -5; i <= 5; i++) {
    glVertex2f( i, -5 );   glVertex2f( i, 5 );
  }
  for (i = -5; i <= 5; i++) {
    glVertex2f( -5, i );   glVertex2f( 5, i );
  }
  for (i = -5; i <= 5; i++) {
    glVertex2f( i, -5 );  glVertex2f( i * 1.15, -5.9 );
  }
  glVertex2f( -5.3, -5.35 );    glVertex2f( 5.3, -5.35 );
  glVertex2f( -5.75, -5.9 );     glVertex2f( 5.75, -5.9 );
  glEnd();

  glPushMatrix();
  glTranslatef( Xpos, Ypos, 0.0 );
  glScalef( 2.0, 2.0, 2.0 );
  glRotatef( 8.0, 0.0, 0.0, 1.0 );
  glRotatef( 90.0, 1.0, 0.0, 0.0 );
  glRotatef( Zrot, 0.0, 0.0, 1.0 );


  glCallList( Ball );

  glPopMatrix();

  glFlush();
  //swap_buffers();
}


void idle( void )
{
  static float vel0 = -100.0;

  Zrot += Zstep;

  Xpos += Xvel;
  if (Xpos >= Xmax) {
    Xpos = Xmax;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }
  if (Xpos <= Xmin) {
    Xpos = Xmin;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }

  Ypos += Yvel;
  Yvel += G;
  if (Ypos < Ymin) {
    Ypos = Ymin;
    if (vel0 == -100.0)  vel0 = fabs(Yvel);
    Yvel = vel0;
  }
  draw();
}


void init_ball(void)
{
  Ball = make_ball();
  glCullFace( GL_BACK );
  glEnable( GL_CULL_FACE );
  glDisable( GL_DITHER );
  glShadeModel( GL_FLAT );
}

void setup() {
  Serial.begin(115200);

  display.begin();
#ifdef ARDUINO_GIGA
  touch.begin();
  touch.onDetect(handleTouch);
#endif

  WINDOWX = display.width();
  WINDOWY = display.height();

  cbuf = (lv_color_t*)ea_malloc(WINDOWX * WINDOWY * 4);

  canvas = lv_canvas_create(lv_scr_act());
  lv_canvas_set_buffer(canvas, cbuf, WINDOWX, WINDOWY, LV_IMG_CF_TRUE_COLOR);

  auto frameBuffer = ZB_open(WINDOWX, WINDOWY, ZB_MODE_5R6G5B, 0, NULL, NULL, cbuf);
  glInit(frameBuffer);

  reshape(WINDOWX, WINDOWY);
  init_ball();

  glScalef(0.5, 0.5, 0.5);

#if 0
  glEnable(GL_DEPTH_TEST);

  /* Setup texturing */
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  .
  /* generate texture object IDs */
  glGenTextures(2, TexObj);
  bind_texture(TexObj[0], 0);
  bind_texture(TexObj[1], 1);
#endif

  lv_timer_create(anim, 10, NULL);
}

void loop() {
#if LVGL_VERSION_MAJOR > 7
  lv_timer_handler();
#else
  lv_task_handler();
#endif
}
