//
// sys_sdl2.c: SDL2 system implementation; mostly just copypasta from
// sys_linux.c
//

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "quakedef.h"
#include "sys_sdl2.h"

// it seems that newer versions of Linux have deprecated FNDELAY in favour of
// O_NDELAY
#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif

int nostdout = 0;

cvar_t sys_linerefresh = {"sys_linerefresh", "0"}; // set for entity display

float mx = 0.0;
float my = 0.0;

float old_mx = 0.0;
float old_my = 0.0;

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val) {}

/*
void Sys_Printf (char *fmt, ...)
{
        va_list		argptr;
        char		text[1024];

        va_start (argptr,fmt);
        vsprintf (text,fmt,argptr);
        va_end (argptr);
        fprintf(stderr, "%s", text);

        Con_Print (text);
}

void Sys_Printf (char *fmt, ...)
{

        va_list     argptr;
        char        text[1024], *t_p;
        int         l, r;

        if (nostdout)
                return;

        va_start (argptr,fmt);
        vsprintf (text,fmt,argptr);
        va_end (argptr);

        l = strlen(text);
        t_p = text;

// make sure everything goes through, even though we are non-blocking
        while (l)
        {
                r = write (1, text, l);
                if (r != l)
                        sleep (0);
                if (r > 0)
                {
                        t_p += r;
                        l -= r;
                }
        }

}
*/

void Sys_Printf(char *fmt, ...) {
  va_list argptr;
  char text[1024];
  unsigned char *p;

  va_start(argptr, fmt);
  vsprintf(text, fmt, argptr);
  va_end(argptr);

  if (strlen(text) > sizeof(text))
    Sys_Error("memory overwrite in Sys_Printf");

  if (nostdout)
    return;

  for (p = (unsigned char *)text; *p; p++) {
    *p &= 0x7f;
    if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
      printf("[%02x]", *p);
    else
      putc(*p, stdout);
  }
}

#if 0
static char end1[] =
"\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m\x1b[1;1H                QUAKE: The Doomed Dimension \x1b[33mby \x1b[44mid\x1b[41m Software                      \x1b[2;1H  ----------------------------------------------------------------------------  \x1b[3;1H           CALL 1-800-IDGAMES TO ORDER OR FOR TECHNICAL SUPPORT                 \x1b[4;1H             PRICE: $45.00 (PRICES MAY VARY OUTSIDE THE US.)                    \x1b[5;1H                                                                                \x1b[6;1H  \x1b[37mYes! You only have one fourth of this incredible epic. That is because most   \x1b[7;1H   of you have paid us nothing or at most, very little. You could steal the     \x1b[8;1H   game from a friend. But we both know you'll be punished by God if you do.    \x1b[9;1H        \x1b[33mWHY RISK ETERNAL DAMNATION? CALL 1-800-IDGAMES AND BUY NOW!             \x1b[10;1H             \x1b[37mRemember, we love you almost as much as He does.                   \x1b[11;1H                                                                                \x1b[12;1H            \x1b[33mProgramming: \x1b[37mJohn Carmack, Michael Abrash, John Cash                \x1b[13;1H       \x1b[33mDesign: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits         \x1b[14;1H                     \x1b[33mArt: \x1b[37mAdrian Carmack, Kevin Cloud                           \x1b[15;1H               \x1b[33mBiz: \x1b[37mJay Wilbur, Mike Wilson, Donna Jackson                      \x1b[16;1H            \x1b[33mProjects: \x1b[37mShawn Green   \x1b[33mSupport: \x1b[37mBarrett Alexander                  \x1b[17;1H              \x1b[33mSound Effects: \x1b[37mTrent Reznor and Nine Inch Nails                   \x1b[18;1H  For other information or details on ordering outside the US, check out the    \x1b[19;1H     files accompanying QUAKE or our website at http://www.idsoftware.com.      \x1b[20;1H    \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.        \x1b[21;1H     All rights reserved. NIN logo is a registered trademark licensed to        \x1b[22;1H                 Nothing Interactive, Inc. All rights reserved.                 \x1b[40m\x1b[23;1H\x1b[0m";
static char end2[] =
"\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m\x1b[1;1H        QUAKE \x1b[33mby \x1b[44mid\x1b[41m Software                                                    \x1b[2;1H -----------------------------------------------------------------------------  \x1b[3;1H        \x1b[37mWhy did you quit from the registered version of QUAKE? Did the          \x1b[4;1H        scary monsters frighten you? Or did Mr. Sandman tug at your             \x1b[5;1H        little lids? No matter! What is important is you love our               \x1b[6;1H        game, and gave us your money. Congratulations, you are probably         \x1b[7;1H        not a thief.                                                            \x1b[8;1H                                                           Thank You.           \x1b[9;1H        \x1b[33;44mid\x1b[41m Software is:                                                         \x1b[10;1H        PROGRAMMING: \x1b[37mJohn Carmack, Michael Abrash, John Cash                    \x1b[11;1H        \x1b[33mDESIGN: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits        \x1b[12;1H        \x1b[33mART: \x1b[37mAdrian Carmack, Kevin Cloud                                        \x1b[13;1H        \x1b[33mBIZ: \x1b[37mJay Wilbur, Mike Wilson     \x1b[33mPROJECTS MAN: \x1b[37mShawn Green              \x1b[14;1H        \x1b[33mBIZ ASSIST: \x1b[37mDonna Jackson        \x1b[33mSUPPORT: \x1b[37mBarrett Alexander             \x1b[15;1H        \x1b[33mSOUND EFFECTS AND MUSIC: \x1b[37mTrent Reznor and Nine Inch Nails               \x1b[16;1H                                                                                \x1b[17;1H        If you need help running QUAKE refer to the text files in the           \x1b[18;1H        QUAKE directory, or our website at http://www.idsoftware.com.           \x1b[19;1H        If all else fails, call our technical support at 1-800-IDGAMES.         \x1b[20;1H      \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.      \x1b[21;1H        All rights reserved. NIN logo is a registered trademark licensed        \x1b[22;1H             to Nothing Interactive, Inc. All rights reserved.                  \x1b[23;1H\x1b[40m\x1b[0m";

#endif
void Sys_Quit(void) {
  Host_Shutdown();
  fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
#if 0
	if (registered.value)
		printf("%s", end2);
	else
		printf("%s", end1);
#endif
  fflush(stdout);
  exit(0);
}

void Sys_Init(void) {
#if id386
  Sys_SetFPCW();
#endif
}

void Sys_Error(char *error, ...) {
  va_list argptr;
  char string[1024];

  // change stdin to non blocking
  fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);

  va_start(argptr, error);
  vsprintf(string, error, argptr);
  va_end(argptr);
  fprintf(stderr, "Error: %s\n", string);

  Host_Shutdown();
#ifdef __EMSCRIPTEN__
  emscripten_force_exit(1);
#else
  exit(1);
#endif
}

void Sys_Warn(char *warning, ...) {
  va_list argptr;
  char string[1024];

  va_start(argptr, warning);
  vsprintf(string, warning, argptr);
  va_end(argptr);
  fprintf(stderr, "Warning: %s", string);
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime(char *path) {
  struct stat buf;

  if (stat(path, &buf) == -1)
    return -1;

  return buf.st_mtime;
}

void Sys_mkdir(char *path) { mkdir(path, 0777); }

int Sys_FileOpenRead(char *path, int *handle) {
  int h;
  struct stat fileinfo;

  h = open(path, O_RDONLY, 0666);
  *handle = h;
  if (h == -1)
    return -1;

  if (fstat(h, &fileinfo) == -1)
    Sys_Error("Error fstating %s", path);

  return fileinfo.st_size;
}

int Sys_FileOpenWrite(char *path) {
  int handle;

  umask(0);

  handle = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);

  if (handle == -1)
    Sys_Error("Error opening %s: %s", path, strerror(errno));

  return handle;
}

int Sys_FileWrite(int handle, void *src, int count) {
  return write(handle, src, count);
}

void Sys_FileClose(int handle) { close(handle); }

void Sys_FileSeek(int handle, int position) {
  lseek(handle, position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count) {
  return read(handle, dest, count);
}

void Sys_DebugLog(char *file, char *fmt, ...) {
  va_list argptr;
  static char data[1024];
  int fd;

  va_start(argptr, fmt);
  vsprintf(data, fmt, argptr);
  va_end(argptr);
  //    fd = open(file, O_WRONLY | O_BINARY | O_CREAT | O_APPEND, 0666);
  fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
  write(fd, data, strlen(data));
  close(fd);
}

void Sys_EditFile(char *filename) {

  char cmd[256];
  char *term;
  char *editor;

  term = getenv("TERM");
  if (term && !strcmp(term, "xterm")) {
    editor = getenv("VISUAL");
    if (!editor)
      editor = getenv("EDITOR");
    if (!editor)
      editor = getenv("EDIT");
    if (!editor)
      editor = "vi";
    sprintf(cmd, "xterm -e %s %s", editor, filename);
    system(cmd);
  }
}

double Sys_FloatTime(void) {
  struct timeval tp;
  static int secbase;

  gettimeofday(&tp, NULL);

  if (!secbase) {
    secbase = tp.tv_sec;
    return tp.tv_usec / 1000000.0;
  }

  return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x) { oktogo = 1; }

void Sys_LineRefresh(void) {}

void floating_point_exception_handler(int whatever) {
  //	Sys_Warn("floating point exception\n");
  signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput(void) {
  static char text[256];
  int len;
  fd_set fdset;
  struct timeval timeout;

  if (cls.state == ca_dedicated) {
    FD_ZERO(&fdset);
    FD_SET(0, &fdset); // stdin
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (select(1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
      return NULL;

    len = read(0, text, sizeof(text));
    if (len < 1)
      return NULL;
    text[len - 1] = 0; // rip off the /n and terminate

    return text;
  }
  return NULL;
}

#if !id386
void Sys_HighFPPrecision(void) {}

void Sys_LowFPPrecision(void) {}
#endif

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length) {

  int r;
  unsigned long addr;
  int psize = getpagesize();

  addr = (startaddr & ~(psize - 1)) - psize;

  //	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
  //			addr, startaddr+length, length);

  r = mprotect((char *)addr, length + startaddr - addr + psize, 7);

  if (r < 0)
    Sys_Error("Protection change failed\n");
}

/*
================
Sys_SendKeyEvents - this is where we handle the SDL2 keys, mouse buttons and
mouse motion
================
*/

void Sys_SendKeyEvents(void) {
  qboolean had_mouse_events = false;

  SDL_Event event;

  // Sys_SendKeyEvents() is called regularly (maybe per frame? idk) and there
  // may be one or more SDL-side events to be handled
  while (SDL_PollEvent(&event)) {
    // self-explanatory
    if (event.type == SDL_QUIT) {
      Sys_Quit();
      return;
    }

    // as in down as in pressed, up as in released
    if ((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP)) {
      // a bunch of the Quake-side key codes are just their ASCII
      // values and so this catch-all picks up most of those
      int k = event.key.keysym.sym;

      // but most of the special keys don't match, so they need
      // special treatment
      switch (event.key.keysym.scancode) {
      case SDL_SCANCODE_BACKSPACE:
        k = K_BACKSPACE;
        break;
      case SDL_SCANCODE_UP:
        k = K_UPARROW;
        break;
      case SDL_SCANCODE_DOWN:
        k = K_DOWNARROW;
        break;
      case SDL_SCANCODE_LEFT:
        k = K_LEFTARROW;
        break;
      case SDL_SCANCODE_RIGHT:
        k = K_RIGHTARROW;
        break;
      case SDL_SCANCODE_LALT:
        k = K_ALT;
        break;
      case SDL_SCANCODE_RALT:
        k = K_ALT;
        break;
      case SDL_SCANCODE_LCTRL:
        k = K_CTRL;
        break;
      case SDL_SCANCODE_RCTRL:
        k = K_CTRL;
        break;
      case SDL_SCANCODE_LSHIFT:
        k = K_SHIFT;
        break;
      case SDL_SCANCODE_RSHIFT:
        k = K_SHIFT;
        break;
      case SDL_SCANCODE_F1:
        k = K_F1;
        break;
      case SDL_SCANCODE_F2:
        k = K_F2;
        break;
      case SDL_SCANCODE_F3:
        k = K_F3;
        break;
      case SDL_SCANCODE_F4:
        k = K_F4;
        break;
      case SDL_SCANCODE_F5:
        k = K_F5;
        break;
      case SDL_SCANCODE_F6:
        k = K_F6;
        break;
      case SDL_SCANCODE_F7:
        k = K_F7;
        break;
      case SDL_SCANCODE_F8:
        k = K_F8;
        break;
      case SDL_SCANCODE_F9:
        k = K_F9;
        break;
      case SDL_SCANCODE_F10:
        k = K_F10;
        break;
      case SDL_SCANCODE_F11:
        k = K_F11;
        break;
      case SDL_SCANCODE_F12:
        k = K_F12;
        break;
      case SDL_SCANCODE_INSERT:
        k = K_INS;
        break;
      case SDL_SCANCODE_DELETE:
        k = K_DEL;
        break;
      case SDL_SCANCODE_PAGEDOWN:
        k = K_PGDN;
        break;
      case SDL_SCANCODE_PAGEUP:
        k = K_PGUP;
        break;
      case SDL_SCANCODE_HOME:
        k = K_HOME;
        break;
      case SDL_SCANCODE_END:
        k = K_END;
        break;
      case SDL_SCANCODE_PAUSE:
        k = K_PAUSE;
        break;
      default:
        break;
      };

      // safeguard in case we somehow end up outside the Quake-side range (e.g.
      // a key is pressed that we don't support)
      if (k >= 0 && k <= 255) {
        Key_Event(k, event.type == SDL_KEYDOWN);
      }
    }

    // Quake-side mouse buttons are just treated like keys
    if ((event.type == SDL_MOUSEBUTTONDOWN) ||
        (event.type == SDL_MOUSEBUTTONUP)) {
      Key_Event(event.button.button + 199, event.type == SDL_MOUSEBUTTONDOWN);
    }

    // in keeping with the pattern, most of the mouse motion handling work is
    // done in another event handler- but being that this is our SDL-side event
    // handler, we need to at least store the values of the relative mouse
    // position here (relative mouse position is stuff like x: -2, y: 5 where
    // the values are relative to the mouse position from the previous mouse
    // motion event)
    if (event.type == SDL_MOUSEMOTION) {
      mx = (float)(event.motion.xrel);
      my = (float)(event.motion.yrel);
      had_mouse_events = true;
    }
  }

  // this is important; when you stop moving the mouse, there is no mouse motion
  // event (which makes sense really)- so if we're on the way out of
  // Sys_SendKeyEvents() without having seen any SDL2 events, we need to tell
  // Quake that the relative mouse movement was zero (otherwise it will be the
  // last value we told it and the practical impact of that is that the mouse
  // will slowly track across the screen even though you're not touching it)
  if (!had_mouse_events) {
    mx = (float)(event.motion.xrel);
    my = (float)(event.motion.yrel);
  }
}

/*
================
Sys_Sleep - I guess we yield? idk
================
*/
void Sys_Sleep(void) {
#ifndef __EMSCRIPTEN__
  SDL_Delay(1);
#endif
}
