/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <asm/io.h>
#include <dlfcn.h>

/*#include "vga.h" */
#include "vgakeyboard.h"
#include "vgamouse.h"

#include "quakedef.h"
#include "GL/fxmesa.h"

#define WARP_WIDTH 320
#define WARP_HEIGHT 200

static fxMesaContext fc = NULL;
#define stringify(m) {#m, m}

unsigned short d_8to16table[256];
unsigned d_8to24table[256];
unsigned char d_15to8table[65536];

int num_shades = 32;

struct {
  char *name;
  int num;
} mice[] = {
    stringify(MOUSE_MICROSOFT), stringify(MOUSE_MOUSESYSTEMS),
    stringify(MOUSE_MMSERIES),  stringify(MOUSE_LOGITECH),
    stringify(MOUSE_BUSMOUSE),  stringify(MOUSE_PS2),
};

static unsigned char scantokey[128];

int num_mice = sizeof(mice) / sizeof(mice[0]);

int d_con_indirect = 0;

int svgalib_inited = 0;
int UseMouse = 1;
int UseKeyboard = 1;

int mouserate = MOUSE_DEFAULTSAMPLERATE;

cvar_t vid_mode = {"vid_mode", "5", false};
cvar_t vid_redrawfull = {"vid_redrawfull", "0", false};
cvar_t vid_waitforrefresh = {"vid_waitforrefresh", "0", true};

char *framebuffer_ptr;

cvar_t mouse_button_commands[3] = {
    {"mouse1", "+attack"},
    {"mouse2", "+strafe"},
    {"mouse3", "+forward"},
};

int mouse_buttons;
int mouse_buttonstate;
int mouse_oldbuttonstate;
float mouse_x, mouse_y;
float old_mouse_x, old_mouse_y;
int mx, my;

cvar_t m_filter = {"m_filter", "1"};

int scr_width, scr_height;

/*-----------------------------------------------------------------------*/

// int		texture_mode = GL_NEAREST;
// int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
// int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int texture_mode = GL_LINEAR;
// int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
// int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int texture_extension_number = 1;

float gldepthmin, gldepthmax;

cvar_t gl_ztrick = {"gl_ztrick", "1"};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

void (*qgl3DfxSetPaletteEXT)(GLuint *);
void (*qglColorTableEXT)(int, int, int, int, int, const void *);

static float vid_gamma = 1.0;

qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height) {}

void D_EndDirectRect(int x, int y, int width, int height) {}

int matchmouse(int mouse, char *name) {
  int i;
  for (i = 0; i < num_mice; i++)
    if (!strcmp(mice[i].name, name))
      return i;
  return mouse;
}

#if 0

void vtswitch(int newconsole)
{

	int fd;
	struct vt_stat x;

// switch consoles and wait until reactivated
	fd = open("/dev/console", O_RDONLY);
	ioctl(fd, VT_GETSTATE, &x);
	ioctl(fd, VT_ACTIVATE, newconsole);
	ioctl(fd, VT_WAITACTIVE, x.v_active);
	close(fd);

}

#endif

void keyhandler(int scancode, int state) {

  int sc;

  sc = scancode & 0x7f;

  Key_Event(scantokey[sc], state == KEY_EVENTPRESS);
}

void VID_Shutdown(void) {
  if (!fc)
    return;

  fxMesaDestroyContext(fc);

  if (UseKeyboard)
    keyboard_close();
}

void signal_handler(int sig) {
  printf("Received signal %d, exiting...\n", sig);
  Sys_Quit();
  exit(0);
}

void InitSig(void) {
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGTRAP, signal_handler);
  signal(SIGIOT, signal_handler);
  signal(SIGBUS, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *p) {
  //	VID_SetPalette(p);
}

void VID_SetPalette(unsigned char *palette) {
  byte *pal;
  unsigned r, g, b;
  unsigned v;
  int r1, g1, b1;
  int j, k, l, m;
  unsigned short i;
  unsigned *table;
  FILE *f;
  char s[255];
  int dist, bestdist;
  static qboolean palflag = false;

  //
  // 8 8 8 encoding
  //
  pal = palette;
  table = d_8to24table;
  for (i = 0; i < 256; i++) {
    r = pal[0];
    g = pal[1];
    b = pal[2];
    pal += 3;

    v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
    *table++ = v;
  }
  d_8to24table[255] &= 0xffffff; // 255 is transparent

  // JACK: 3D distance calcs - k is last closest, l is the distance.
  for (i = 0; i < (1 << 15); i++) {
    /* Maps
    000000000000000
    000000000011111 = Red  = 0x1F
    000001111100000 = Blue = 0x03E0
    111110000000000 = Grn  = 0x7C00
    */
    r = ((i & 0x1F) << 3) + 4;
    g = ((i & 0x03E0) >> 2) + 4;
    b = ((i & 0x7C00) >> 7) + 4;
    pal = (unsigned char *)d_8to24table;
    for (v = 0, k = 0, bestdist = 10000 * 10000; v < 256; v++, pal += 4) {
      r1 = (int)r - (int)pal[0];
      g1 = (int)g - (int)pal[1];
      b1 = (int)b - (int)pal[2];
      dist = (r1 * r1) + (g1 * g1) + (b1 * b1);
      if (dist < bestdist) {
        k = v;
        bestdist = dist;
      }
    }
    d_15to8table[i] = k;
  }
}

void CheckMultiTextureExtensions(void) {
  void *prjobj;

  if (strstr(gl_extensions, "GL_SGIS_multitexture ") &&
      !COM_CheckParm("-nomtex")) {
    Con_Printf("Found GL_SGIS_multitexture...\n");

    if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
      Con_Printf("Unable to open symbol list for main program.\n");
      return;
    }

    qglMTexCoord2fSGIS = (void *)dlsym(prjobj, "glMTexCoord2fSGIS");
    qglSelectTextureSGIS = (void *)dlsym(prjobj, "glSelectTextureSGIS");

    if (qglMTexCoord2fSGIS && qglSelectTextureSGIS) {
      Con_Printf("Multitexture extensions found.\n");
      gl_mtexable = true;
    } else
      Con_Printf("Symbol not found, disabled.\n");

    dlclose(prjobj);
  }
}

/*
===============
GL_Init
===============
*/
void GL_Init(void) {
  gl_vendor = glGetString(GL_VENDOR);
  Con_Printf("GL_VENDOR: %s\n", gl_vendor);
  gl_renderer = glGetString(GL_RENDERER);
  Con_Printf("GL_RENDERER: %s\n", gl_renderer);

  gl_version = glGetString(GL_VERSION);
  Con_Printf("GL_VERSION: %s\n", gl_version);
  gl_extensions = glGetString(GL_EXTENSIONS);
  Con_Printf("GL_EXTENSIONS: %s\n", gl_extensions);

  //	Con_Printf ("%s %s\n", gl_renderer, gl_version);

  CheckMultiTextureExtensions();

  glClearColor(1, 0, 0, 0);
  glCullFace(GL_FRONT);
  glEnable(GL_TEXTURE_2D);

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.666);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glShadeModel(GL_FLAT);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering(int *x, int *y, int *width, int *height) {
  extern cvar_t gl_clear;

  *x = *y = 0;
  *width = scr_width;
  *height = scr_height;

  //    if (!wglMakeCurrent( maindc, baseRC ))
  //		Sys_Error ("wglMakeCurrent failed");

  //	glViewport (*x, *y, *width, *height);
}

void GL_EndRendering(void) {
  glFlush();
  fxMesaSwapBuffers();
}

void Init_KBD(void) {
  int i;

  if (COM_CheckParm("-nokbd"))
    UseKeyboard = 0;

  if (UseKeyboard) {
    for (i = 0; i < 128; i++)
      scantokey[i] = ' ';

    scantokey[42] = K_SHIFT;
    scantokey[54] = K_SHIFT;
    scantokey[72] = K_UPARROW;
    scantokey[103] = K_UPARROW;
    scantokey[80] = K_DOWNARROW;
    scantokey[108] = K_DOWNARROW;
    scantokey[75] = K_LEFTARROW;
    scantokey[105] = K_LEFTARROW;
    scantokey[77] = K_RIGHTARROW;
    scantokey[106] = K_RIGHTARROW;
    scantokey[29] = K_CTRL;
    scantokey[97] = K_CTRL;
    scantokey[56] = K_ALT;
    scantokey[100] = K_ALT;
    //		scantokey[58] = JK_CAPS;
    //		scantokey[69] = JK_NUM_LOCK;
    scantokey[71] = K_HOME;
    scantokey[73] = K_PGUP;
    scantokey[79] = K_END;
    scantokey[81] = K_PGDN;
    scantokey[82] = K_INS;
    scantokey[83] = K_DEL;
    scantokey[1] = K_ESCAPE;
    scantokey[28] = K_ENTER;
    scantokey[15] = K_TAB;
    scantokey[14] = K_BACKSPACE;
    scantokey[119] = K_PAUSE;
    scantokey[57] = ' ';

    scantokey[102] = K_HOME;
    scantokey[104] = K_PGUP;
    scantokey[107] = K_END;
    scantokey[109] = K_PGDN;
    scantokey[110] = K_INS;
    scantokey[111] = K_DEL;

    scantokey[2] = '1';
    scantokey[3] = '2';
    scantokey[4] = '3';
    scantokey[5] = '4';
    scantokey[6] = '5';
    scantokey[7] = '6';
    scantokey[8] = '7';
    scantokey[9] = '8';
    scantokey[10] = '9';
    scantokey[11] = '0';
    scantokey[12] = '-';
    scantokey[13] = '=';
    scantokey[41] = '`';
    scantokey[26] = '[';
    scantokey[27] = ']';
    scantokey[39] = ';';
    scantokey[40] = '\'';
    scantokey[51] = ',';
    scantokey[52] = '.';
    scantokey[53] = '/';
    scantokey[43] = '\\';

    scantokey[59] = K_F1;
    scantokey[60] = K_F2;
    scantokey[61] = K_F3;
    scantokey[62] = K_F4;
    scantokey[63] = K_F5;
    scantokey[64] = K_F6;
    scantokey[65] = K_F7;
    scantokey[66] = K_F8;
    scantokey[67] = K_F9;
    scantokey[68] = K_F10;
    scantokey[87] = K_F11;
    scantokey[88] = K_F12;
    scantokey[30] = 'a';
    scantokey[48] = 'b';
    scantokey[46] = 'c';
    scantokey[32] = 'd';
    scantokey[18] = 'e';
    scantokey[33] = 'f';
    scantokey[34] = 'g';
    scantokey[35] = 'h';
    scantokey[23] = 'i';
    scantokey[36] = 'j';
    scantokey[37] = 'k';
    scantokey[38] = 'l';
    scantokey[50] = 'm';
    scantokey[49] = 'n';
    scantokey[24] = 'o';
    scantokey[25] = 'p';
    scantokey[16] = 'q';
    scantokey[19] = 'r';
    scantokey[31] = 's';
    scantokey[20] = 't';
    scantokey[22] = 'u';
    scantokey[47] = 'v';
    scantokey[17] = 'w';
    scantokey[45] = 'x';
    scantokey[21] = 'y';
    scantokey[44] = 'z';

    scantokey[78] = '+';
    scantokey[74] = '-';

    if (keyboard_init())
      Sys_Error("keyboard_init() failed");
    keyboard_seteventhandler(keyhandler);
  }
}

#define NUM_RESOLUTIONS 16

static int resolutions[NUM_RESOLUTIONS][3] = {
    320,  200,  GR_RESOLUTION_320x200,   320,  240,  GR_RESOLUTION_320x240,
    400,  256,  GR_RESOLUTION_400x256,   400,  300,  GR_RESOLUTION_400x300,
    512,  384,  GR_RESOLUTION_512x384,   640,  200,  GR_RESOLUTION_640x200,
    640,  350,  GR_RESOLUTION_640x350,   640,  400,  GR_RESOLUTION_640x400,
    640,  480,  GR_RESOLUTION_640x480,   800,  600,  GR_RESOLUTION_800x600,
    960,  720,  GR_RESOLUTION_960x720,   856,  480,  GR_RESOLUTION_856x480,
    512,  256,  GR_RESOLUTION_512x256,   1024, 768,  GR_RESOLUTION_1024x768,
    1280, 1024, GR_RESOLUTION_1280x1024, 1600, 1200, GR_RESOLUTION_1600x1200};

int findres(int *width, int *height) {
  int i;

  for (i = 0; i < NUM_RESOLUTIONS; i++)
    if ((*width <= resolutions[i][0]) && (*height <= resolutions[i][1])) {
      *width = resolutions[i][0];
      *height = resolutions[i][1];
      return resolutions[i][2];
    }

  *width = 640;
  *height = 480;
  return GR_RESOLUTION_640x480;
}

qboolean VID_Is8bit(void) { return is8bit; }

void VID_Init8bitPalette(void) {
  // Check for 8bit Extensions and initialize them.
  int i;
  void *prjobj;

  if (COM_CheckParm("-no8bit"))
    return;

  if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
    Con_Printf("Unable to open symbol list for main program.\n");
    return;
  }

  if (strstr(gl_extensions, "3DFX_set_global_palette") &&
      (qgl3DfxSetPaletteEXT = dlsym(prjobj, "gl3DfxSetPaletteEXT")) != NULL) {
    GLubyte table[256][4];
    char *oldpal;

    Con_SafePrintf("... Using 3DFX_set_global_palette\n");
    glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
    oldpal = (char *)d_8to24table; // d_8to24table3dfx;
    for (i = 0; i < 256; i++) {
      table[i][2] = *oldpal++;
      table[i][1] = *oldpal++;
      table[i][0] = *oldpal++;
      table[i][3] = 255;
      oldpal++;
    }
    qgl3DfxSetPaletteEXT((GLuint *)table);
    is8bit = true;

  } else if (strstr(gl_extensions, "GL_EXT_shared_texture_palette") &&
             (qglColorTableEXT = dlsym(prjobj, "glColorTableEXT")) != NULL) {
    char thePalette[256 * 3];
    char *oldPalette, *newPalette;

    Con_SafePrintf("... Using GL_EXT_shared_texture_palette\n");
    glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
    oldPalette = (char *)d_8to24table; // d_8to24table3dfx;
    newPalette = thePalette;
    for (i = 0; i < 256; i++) {
      *newPalette++ = *oldPalette++;
      *newPalette++ = *oldPalette++;
      *newPalette++ = *oldPalette++;
      oldPalette++;
    }
    qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB,
                     GL_UNSIGNED_BYTE, (void *)thePalette);
    is8bit = true;
  }

  dlclose(prjobj);
}

static void Check_Gamma(unsigned char *pal) {
  float f, inf;
  unsigned char palette[768];
  int i;

  if ((i = COM_CheckParm("-gamma")) == 0) {
    if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
        (gl_vendor && strstr(gl_vendor, "3Dfx")))
      vid_gamma = 1;
    else
      vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
  } else
    vid_gamma = Q_atof(com_argv[i + 1]);

  for (i = 0; i < 768; i++) {
    f = pow((pal[i] + 1) / 256.0, vid_gamma);
    inf = f * 255 + 0.5;
    if (inf < 0)
      inf = 0;
    if (inf > 255)
      inf = 255;
    palette[i] = inf;
  }

  memcpy(pal, palette, sizeof(palette));
}

void VID_Init(unsigned char *palette) {
  int i;
  GLint attribs[32];
  char gldir[MAX_OSPATH];
  int width = 640, height = 480;

  Init_KBD();

  Cvar_RegisterVariable(&vid_mode);
  Cvar_RegisterVariable(&vid_redrawfull);
  Cvar_RegisterVariable(&vid_waitforrefresh);
  Cvar_RegisterVariable(&gl_ztrick);

  vid.maxwarpwidth = WARP_WIDTH;
  vid.maxwarpheight = WARP_HEIGHT;
  vid.colormap = host_colormap;
  vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));

  // interpret command-line params

  // set vid parameters
  attribs[0] = FXMESA_DOUBLEBUFFER;
  attribs[1] = FXMESA_ALPHA_SIZE;
  attribs[2] = 1;
  attribs[3] = FXMESA_DEPTH_SIZE;
  attribs[4] = 1;
  attribs[5] = FXMESA_NONE;

  if ((i = COM_CheckParm("-width")) != 0)
    width = atoi(com_argv[i + 1]);
  if ((i = COM_CheckParm("-height")) != 0)
    height = atoi(com_argv[i + 1]);

  if ((i = COM_CheckParm("-conwidth")) != 0)
    vid.conwidth = Q_atoi(com_argv[i + 1]);
  else
    vid.conwidth = 640;

  vid.conwidth &= 0xfff8; // make it a multiple of eight

  if (vid.conwidth < 320)
    vid.conwidth = 320;

  // pick a conheight that matches with correct aspect
  vid.conheight = vid.conwidth * 3 / 4;

  if ((i = COM_CheckParm("-conheight")) != 0)
    vid.conheight = Q_atoi(com_argv[i + 1]);
  if (vid.conheight < 200)
    vid.conheight = 200;

  fc = fxMesaCreateContext(0, findres(&width, &height), GR_REFRESH_75Hz,
                           attribs);
  if (!fc)
    Sys_Error("Unable to create 3DFX context.\n");

  InitSig(); // trap evil signals

  scr_width = width;
  scr_height = height;

  fxMesaMakeCurrent(fc);

  if (vid.conheight > height)
    vid.conheight = height;
  if (vid.conwidth > width)
    vid.conwidth = width;
  vid.width = vid.conwidth;
  vid.height = vid.conheight;

  vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
  vid.numpages = 2;

  GL_Init();

  sprintf(gldir, "%s/glquake", com_gamedir);
  Sys_mkdir(gldir);

  Check_Gamma(palette);
  VID_SetPalette(palette);

  // Check for 3DFX Extensions and initialize them.
  VID_Init8bitPalette();

  Con_SafePrintf("Video mode %dx%d initialized.\n", width, height);

  vid.recalc_refdef = 1; // force a surface cache flush
}

void Sys_SendKeyEvents(void) {
  if (UseKeyboard)
    while (keyboard_update())
      ;
}

void Force_CenterView_f(void) { cl.viewangles[PITCH] = 0; }

void mousehandler(int buttonstate, int dx, int dy) {
  mouse_buttonstate = buttonstate;
  mx += dx;
  my += dy;
}

void IN_Init(void) {

  int mtype;
  char *mousedev;
  int mouserate;

  if (UseMouse) {

    Cvar_RegisterVariable(&mouse_button_commands[0]);
    Cvar_RegisterVariable(&mouse_button_commands[1]);
    Cvar_RegisterVariable(&mouse_button_commands[2]);
    Cmd_AddCommand("force_centerview", Force_CenterView_f);

    mouse_buttons = 3;

    mtype = vga_getmousetype();

    mousedev = "/dev/mouse";
    if (getenv("MOUSEDEV"))
      mousedev = getenv("MOUSEDEV");
    if (COM_CheckParm("-mdev"))
      mousedev = com_argv[COM_CheckParm("-mdev") + 1];

    mouserate = 1200;
    if (getenv("MOUSERATE"))
      mouserate = atoi(getenv("MOUSERATE"));
    if (COM_CheckParm("-mrate"))
      mouserate = atoi(com_argv[COM_CheckParm("-mrate") + 1]);

    if (mouse_init(mousedev, mtype, mouserate)) {
      Con_Printf("No mouse found\n");
      UseMouse = 0;
    } else
      mouse_seteventhandler(mousehandler);
  }
}

void IN_Shutdown(void) {
  if (UseMouse)
    mouse_close();
}

/*
===========
IN_Commands
===========
*/
void IN_Commands(void) {
  if (UseMouse && cls.state != ca_dedicated) {
    // poll mouse values
    while (mouse_update())
      ;

    // perform button actions
    if ((mouse_buttonstate & MOUSE_LEFTBUTTON) &&
        !(mouse_oldbuttonstate & MOUSE_LEFTBUTTON))
      Key_Event(K_MOUSE1, true);
    else if (!(mouse_buttonstate & MOUSE_LEFTBUTTON) &&
             (mouse_oldbuttonstate & MOUSE_LEFTBUTTON))
      Key_Event(K_MOUSE1, false);

    if ((mouse_buttonstate & MOUSE_RIGHTBUTTON) &&
        !(mouse_oldbuttonstate & MOUSE_RIGHTBUTTON))
      Key_Event(K_MOUSE2, true);
    else if (!(mouse_buttonstate & MOUSE_RIGHTBUTTON) &&
             (mouse_oldbuttonstate & MOUSE_RIGHTBUTTON))
      Key_Event(K_MOUSE2, false);

    if ((mouse_buttonstate & MOUSE_MIDDLEBUTTON) &&
        !(mouse_oldbuttonstate & MOUSE_MIDDLEBUTTON))
      Key_Event(K_MOUSE3, true);
    else if (!(mouse_buttonstate & MOUSE_MIDDLEBUTTON) &&
             (mouse_oldbuttonstate & MOUSE_MIDDLEBUTTON))
      Key_Event(K_MOUSE3, false);

    mouse_oldbuttonstate = mouse_buttonstate;
  }
}

/*
===========
IN_Move
===========
*/
void IN_MouseMove(usercmd_t *cmd) {
  if (!UseMouse)
    return;

  // poll mouse values
  while (mouse_update())
    ;

  if (m_filter.value) {
    mouse_x = (mx + old_mouse_x) * 0.5;
    mouse_y = (my + old_mouse_y) * 0.5;
  } else {
    mouse_x = mx;
    mouse_y = my;
  }
  old_mouse_x = mx;
  old_mouse_y = my;
  mx = my = 0; // clear for next update

  mouse_x *= sensitivity.value;
  mouse_y *= sensitivity.value;

  // add mouse X/Y movement to cmd
  if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
    cmd->sidemove += m_side.value * mouse_x;
  else
    cl.viewangles[YAW] -= m_yaw.value * mouse_x;

  if (in_mlook.state & 1)
    V_StopPitchDrift();

  if ((in_mlook.state & 1) && !(in_strafe.state & 1)) {
    cl.viewangles[PITCH] += m_pitch.value * mouse_y;
    if (cl.viewangles[PITCH] > 80)
      cl.viewangles[PITCH] = 80;
    if (cl.viewangles[PITCH] < -70)
      cl.viewangles[PITCH] = -70;
  } else {
    if ((in_strafe.state & 1) && noclip_anglehack)
      cmd->upmove -= m_forward.value * mouse_y;
    else
      cmd->forwardmove -= m_forward.value * mouse_y;
  }
}

void IN_Move(usercmd_t *cmd) { IN_MouseMove(cmd); }
