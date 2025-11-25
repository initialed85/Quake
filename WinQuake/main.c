/*
added by initialed85
*/

//
// main.c: main() extracted from sys_sdl2.c (which is mostly a copy of
// sys_linux.c)
//
// TODO: will probably only work for Linux / BSD
//

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
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
#include <stdbool.h>

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "quakedef.h"
#include "sys_sdl2.h"

#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif

char *basedir = ".";
char *cachedir = "/tmp";

typedef struct {
  double time, oldtime, newtime;
} userdata_t;

userdata_t __userData = {0};

void main_iter(void *_userData) {
  extern int vcrFile;
  extern int recording;

  userdata_t *userData = (userdata_t *)_userData;

  // find time spent rendering last frame
  ((*userData)).newtime = Sys_FloatTime();

  (*userData).time = (*userData).newtime - (*userData).oldtime;

  if (cls.state == ca_dedicated) { // play vcrfiles at max speed
    if ((*userData).time < sys_ticrate.value && (vcrFile == -1 || recording)) {
      Sys_Sleep();
      return; // not time to run a server only tic yet
    }
    (*userData).time = sys_ticrate.value;
  }

  if ((*userData).time > sys_ticrate.value * 2)
    (*userData).oldtime = (*userData).newtime;
  else
    (*userData).oldtime += (*userData).time;

  Host_Frame((*userData).time);

  // graphic debugging aids
  if (sys_linerefresh.value)
    Sys_LineRefresh();
}

int main(int c, char **v) {
  userdata_t *userData = &__userData;

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(main_iter, userData, -1, false);
#endif

  quakeparms_t parms;
  int j;

  //	static char cwd[1024];

  //	signal(SIGFPE, floating_point_exception_handler);
  signal(SIGFPE, SIG_IGN);

  memset(&parms, 0, sizeof(parms));

  COM_InitArgv(c, v);

  parms.argc = com_argc;
  parms.argv = com_argv;

#ifdef GLQUAKE
  parms.memsize = 32 * 1024 * 1024;
#else
  parms.memsize = 32 * 1024 * 1024;
#endif

  j = COM_CheckParm("-mem");
  if (j)
    parms.memsize = (int)(Q_atof(com_argv[j + 1]) * 1024 * 1024 * 8);
  parms.membase = malloc(parms.memsize);

  parms.basedir = basedir;
  // caching is disabled by default, use -cachedir to enable
  //	parms.cachedir = cachedir;

  fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

  isDedicated = (COM_CheckParm("-dedicated") != 0);

  Host_Init(&parms);

  Sys_Init();

  if (COM_CheckParm("-nostdout"))
    nostdout = 1;
  else {
    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);
    printf("SDL2 Quake -- Version %0.3f\n", SDL2_VERSION);
  }

  __userData.oldtime = Sys_FloatTime() - 0.1;

#ifndef __EMSCRIPTEN__
  while (1)
    main_iter(userData);
#endif
}
