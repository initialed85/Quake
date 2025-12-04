/*
added by initialed85
*/

//
// sys_sdl2.c: SDL2 video implementation
//

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "quakedef.h"
#include "d_local.h"

#define BASEWIDTH 320
#define BASEHEIGHT 200

byte *vid_buffer;
short *zbuffer;

// byte vid_buffer[BASEWIDTH * BASEHEIGHT];
// short zbuffer[BASEWIDTH * BASEHEIGHT];
byte surfcache[256 * 1024];

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Surface *quake_surface;
static SDL_Texture *texture = NULL;
static int screen_width = BASEWIDTH;
static int screen_height = BASEHEIGHT;

void VID_SetPalette(unsigned char *palette) {
  SDL_Color sdl_palette[256];

  int i;

  // ref.: https://quakewiki.org/wiki/Quake_palette#palette.lmp
  // basically the palette is 256 RGB colours (so 3 bytes each)
  // for 768 bytes in total
  for (i = 0; i < 256; ++i) {
    // this effectively iterates in steps of 3, grabbing
    // the applicable offset
    sdl_palette[i].r = palette[i * 3 + 0];
    sdl_palette[i].g = palette[i * 3 + 1];
    sdl_palette[i].b = palette[i * 3 + 2];
    sdl_palette[i].a = 255; // always 100% opaque

    // no idea- thx chad gibbidy; something to convert 8-bit colours (Quake) to
    // 24-bit colours (SDL) I guess?
    d_8to24table[i] = (sdl_palette[i].r << 0) | (sdl_palette[i].g << 8) |
                      (sdl_palette[i].b << 16) | (sdl_palette[i].a << 24);
  }

  if (SDL_SetPaletteColors(quake_surface->format->palette, sdl_palette, 0,
                           256) < 0) {
    SDL_Log("Failed to SDL_SetPaletteColors(quake_surface->format->palette, "
            "sdl_palette, 0, 256): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }
}

void VID_ShiftPalette(unsigned char *palette) {
  // if you don't implement this gamma doesn't work- not sure what other impact
  // it has
  VID_SetPalette(palette);
}

void VID_Init(unsigned char *palette) {
  int pnum;
  SDL_WindowFlags window_flags = SDL_WINDOW_SHOWN;

  if (!COM_CheckParm("-nosound")) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
      SDL_Log("Failed to SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO): %s",
              SDL_GetError());
#ifdef __EMSCRIPTEN__
      emscripten_force_exit(1);
#else
      exit(1);
#endif
    }
  } else {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      SDL_Log("Failed to SDL_Init(SDL_INIT_VIDEO): %s", SDL_GetError());
#ifdef __EMSCRIPTEN__
      emscripten_force_exit(1);
#else
      exit(1);
#endif
    }
  }

  // these two seem to cause the mouse cursor to disappear favourably
  SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1",
                          SDL_HINT_OVERRIDE);
  if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0) {
    SDL_Log("Failed to SDL_SetRelativeMouseMode(SDL_TRUE): %s", SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  };

  if ((pnum = COM_CheckParm("-width"))) {
    if (pnum >= com_argc - 1)
      Sys_Error("VID: -width <width>\n");
    screen_width = Q_atoi(com_argv[pnum + 1]);
    if (!screen_width)
      Sys_Error("VID: Bad window width\n");
  }

  if ((pnum = COM_CheckParm("-height"))) {
    if (pnum >= com_argc - 1)
      Sys_Error("VID: -height <height>\n");
    screen_height = Q_atoi(com_argv[pnum + 1]);
    if (!screen_height)
      Sys_Error("VID: Bad window height\n");
  }

  if ((pnum = COM_CheckParm("-height"))) {
    if (pnum >= com_argc - 1)
      Sys_Error("VID: -height <height>\n");
    screen_height = Q_atoi(com_argv[pnum + 1]);
    if (!screen_height)
      Sys_Error("VID: Bad window height\n");
  }

  if ((pnum = COM_CheckParm("-fullscreen"))) {
    window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }

  // window is the literal window to display what we render
  window =
      SDL_CreateWindow("Quake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       screen_width, screen_height, window_flags);
  if (!window) {
    SDL_Log("Failed to SDL_CreateWindow(\"Quake\", SDL_WINDOWPOS_CENTERED, "
            "SDL_WINDOWPOS_CENTERED, screen_width, screen_height, "
            "window_flags): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // renderer is what paints onto the window I guess
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    SDL_Log("Failed to SDL_CreateRenderer(window, -1, "
            "SDL_RENDERER_PRESENTVSYNC): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // the surface is what we paint the video buffer onto
  quake_surface = SDL_CreateRGBSurfaceWithFormat(0, screen_width, screen_height,
                                                 8, SDL_PIXELFORMAT_INDEX8);
  if (!quake_surface) {
    SDL_Log("Failed to SDL_CreateRGBSurfaceWithFormat(0, screen_width, "
            "screen_height, 8, SDL_PIXELFORMAT_INDEX8): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // texture seems to be a buffer we can write to that the renderer
  // will read from
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                              SDL_TEXTUREACCESS_STREAMING, screen_width,
                              screen_height);
  if (!texture) {
    SDL_Log("Failed to SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, "
            "SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  //
  // Set up Quake's video state
  //

  vid_buffer = calloc(screen_width * screen_height, sizeof(byte));
  zbuffer = calloc(screen_width * screen_height, sizeof(short));

  vid.width = vid.conwidth = screen_width;
  vid.height = vid.conheight = screen_height;

  if (vid.width > MAXWIDTH || vid.height > MAXHEIGHT) {
    SDL_Log("Failed VID_Init(unsigned char *palette) because vid.width (%d) > MAXWIDTH (%d) and / or vid.height (%d) > MAXHEIGHT (%d))",
            vid.width, MAXWIDTH, vid.height, MAXHEIGHT);
  }

  // the WARP_WIDTH and WARP_HEIGHT constants are used elsewhere relating to
  // warping (water / lava?) in a way that affects memory alignment, so these
  // struct properties must be set this way to avoid segfaults
  vid.maxwarpwidth = WARP_WIDTH;
  vid.maxwarpheight = WARP_HEIGHT;

  vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 200.0);
  vid.numpages = 2;
  vid.colormap = host_colormap;
  vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
  vid.buffer = vid.conbuffer = vid_buffer;
  vid.rowbytes = vid.conrowbytes = screen_width;

  d_pzbuffer = zbuffer;
  D_InitCaches(surfcache, sizeof(surfcache));

  // Apply the initial palette
  if (palette)
    VID_SetPalette(palette);
}

void VID_Shutdown(void) {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void VID_Update(vrect_t *rects) {
  // so here we slam the video buffer onto the surface
  memcpy(quake_surface->pixels, vid.buffer, screen_width * screen_height);

  // suck the quake surface pixels into a var
  byte *surfPixels = (byte *)quake_surface->pixels;

  void *pixels;
  int pitch;

  // connect the pixels var above to the texture
  if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0) {
    SDL_Log("Failed SDL_LockTexture(texture, NULL, &pixels, &pitch): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // get a casted version of the pixels var above
  uint32_t *texPixels = (uint32_t *)pixels;

  int i;

  // suck the quake surface pixels out of the var, via the 8-bit colour to
  // 24-bit colour convesion table and onto casted version of the pixels var
  // that's connected to the texture
  for (i = 0; i < screen_width * screen_height; i++) {
    texPixels[i] = d_8to24table[surfPixels[i]];
  }

  SDL_UnlockTexture(texture);

  // clear some renderer buffer I guess
  if (SDL_RenderClear(renderer) < 0) {
    SDL_Log("Failed SDL_RenderClear(renderer): %s", SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // replace the renderer buffer with the texture
  if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
    SDL_Log("Failed SDL_RenderCopy(renderer, texture, NULL, NULL): %s",
            SDL_GetError());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
  }

  // slam that on the screen
  SDL_RenderPresent(renderer);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height) {
  // noop
}

/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect(int x, int y, int width, int height) {
  // noop
}
