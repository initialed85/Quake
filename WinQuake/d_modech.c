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
// d_modech.c: called when mode has just changed

#include "quakedef.h"
#include "d_local.h"

int d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

int d_y_aspect_shift, d_pix_min, d_pix_max;
float d_pix_scale;

int d_scantable[MAXHEIGHT];
short *zspantable[MAXHEIGHT];

/*
================
D_Patch
================
*/
void D_Patch(void) {
#if id386

  static qboolean protectset8 = false;

  if (!protectset8) {
    Sys_MakeCodeWriteable((int)D_PolysetAff8Start,
                          (int)D_PolysetAff8End - (int)D_PolysetAff8Start);
    protectset8 = true;
  }

#endif // id386
}

/*
================
D_ViewChanged
================
*/
void D_ViewChanged(void) {
  int rowbytes;

  if (r_dowarp)
    rowbytes = WARP_WIDTH;
  else
    rowbytes = vid.rowbytes;

  scale_for_mip = xscale;
  if (yscale > xscale)
    scale_for_mip = yscale;

  d_zrowbytes = vid.width * 2;
  d_zwidth = vid.width;

  // particle pixel size: lock to the 320-wide baseline so particles stay
  // ~1px regardless of resolution (original Quake scaled these with width
  // to keep a constant angular size, which makes particles grow huge at
  // high resolutions)
  // particle pixel size: scales linearly with resolution so particles
  // maintain a consistent physical size on screen. at 320-wide, particles
  // are ~0.66x the size of the original 2x tweak (i.e. ~1.33x the original
  // Quake baseline); at 1920-wide they scale up 6x proportionally.
  // the original Quake used a bit-shift (powers of 2) which made particles
  // grow exponentially with resolution — far too aggressively at high res.
  float pscale = (float)r_refdef.vrect.width / 320.0;
  d_pix_min = (int)(1.33 * pscale + 0.5);
  if (d_pix_min < 1)
    d_pix_min = 1;
  d_pix_max = (int)(5.33 * pscale + 0.5);
  if (d_pix_max < 1)
    d_pix_max = 1;
  d_pix_scale = (1.33f * pscale) / 128.0f;

  if (pixelAspect > 1.4)
    d_y_aspect_shift = 1;
  else
    d_y_aspect_shift = 0;

  d_vrectx = r_refdef.vrect.x;
  d_vrecty = r_refdef.vrect.y;
  d_vrectright_particle = r_refdef.vrectright - d_pix_max;
  d_vrectbottom_particle =
      r_refdef.vrectbottom - (d_pix_max << d_y_aspect_shift);

  {
    int i;

    for (i = 0; i < vid.height; i++) {
      d_scantable[i] = i * rowbytes;
      zspantable[i] = d_pzbuffer + i * d_zwidth;
    }
  }

  D_Patch();
}
