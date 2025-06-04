//
// sys_sdl2.h: SDL2 system implementation; exposing a few things made "public"
//

extern int nostdout;

extern cvar_t sys_linerefresh;

extern float my;
extern float mx;

void Sys_Init(void);

void Sys_LineRefresh(void);
