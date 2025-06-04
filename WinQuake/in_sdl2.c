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
//
// in_sdl2.c: SDL2 mouse input implementation
//

#include <SDL.h>

#include "quakedef.h"
#include "sys_sdl2.h"

cvar_t m_filter = {"m_filter", "1"};

float mouse_x, mouse_y;
float old_mouse_x, old_mouse_y;

void IN_Init(void) { Cvar_RegisterVariable(&m_filter); }

void IN_Shutdown(void) {}

void IN_Commands(void) {}

/*
===========
IN_Move - basically all copypasta from in_dos.c
===========
*/
void IN_Move(usercmd_t *cmd) {
  //
  // note: no ready check or register read (the equivalent of the register read
  // is handled over in sys_sdl2.c::Sys_SendKeyEvents()
  //

  if (m_filter.value) {
    mouse_x = (mx + old_mouse_x) * 0.5;
    mouse_y = (my + old_mouse_y) * 0.5;
  } else {
    mouse_x = mx;
    mouse_y = my;
  }
  old_mouse_x = mx;
  old_mouse_y = my;

  mouse_x *= sensitivity.value;
  mouse_y *= sensitivity.value;

  if (in_mlook.state & 1)
    V_StopPitchDrift();

  // add mouse X/Y movement to cmd
  if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
    cmd->sidemove += m_side.value * mouse_x;
  else
    cl.viewangles[YAW] -= m_yaw.value * mouse_x;

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
