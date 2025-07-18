/*
added by initialed85
*/

//
// in_sdl2.c: SDL2 sound implementation
//

#include <SDL.h>
#include <SDL_audio.h>
#include <stdio.h>

#include "quakedef.h"

extern int desired_speed;
extern int desired_bits;

void audio_callback(void *userdata, Uint8 *stream, int len) {
  if (shm == NULL)
    return;

  // give snd_dma.c somewhere to write audio
  shm->buffer = stream;

  // so snd_dma.c knows how much audio to write
  shm->samplepos += len / (shm->samplebits / 8) / 2;

  // tell snd_mix.c to write some audio
  S_PaintChannels(shm->samplepos);
}

qboolean SNDDMA_Init(void) {
  SDL_AudioSpec desired, obtained;

  desired.freq = desired_speed;

  desired.format = AUDIO_U8;

  // platform will determine SDL_BYTEORDER
  if (desired_bits == 16)
    desired.format =
        SDL_BYTEORDER == SDL_LIL_ENDIAN ? AUDIO_S16LSB : AUDIO_S16MSB;
  else if (desired_bits == 32)
    desired.format =
        SDL_BYTEORDER == SDL_LIL_ENDIAN ? AUDIO_F32LSB : AUDIO_F32MSB;

  desired.channels = 2;
  desired.samples = 512;

  // callback to feed audio- I guess this gets called regularly by SDL
  desired.callback = audio_callback;

  if (SDL_OpenAudio(&desired, NULL) < 0) {
    SDL_Log("Failed to SDL_OpenAudio(&desired, &obtained): %s", SDL_GetError());
    return false;
  }

  // TODO: we're just ignoring all this for now
  // if (obtained.freq != desired.freq)
  // {
  // 	SDL_Log("Failed because obtained.freq: %d != desired.freq: %d",
  // obtained.freq, desired.freq); 	return false;
  // }

  // TODO: we're just ignoring all this for now
  // if (obtained.format != desired.format)
  // {
  // 	SDL_Log("Failed because obtained.format: %d != desired.format: %d",
  // obtained.format, desired.format); 	return false;
  // }

  // TODO: we're just ignoring all this for now
  // if (obtained.channels != desired.channels)
  // {
  // 	SDL_Log("Failed because obtained.channels: %d != desired.channels: %d",
  // obtained.channels, desired.channels); 	return false;
  // }

  // TODO: we're just ignoring all this for now
  // if (obtained.samples != desired.samples)
  // {
  // 	SDL_Log("Failed because obtained.samples: %d != desired.samples: %d",
  // obtained.samples, desired.samples); 	return false;
  // }

  // causes SDL use the regularly-triggered-callback approach
  SDL_PauseAudio(0);

  int size;
  size = 32768 + sizeof(dma_t); // ?
  shm = malloc(size);
  memset((void *)shm, 0, size);

  shm->splitbuffer = 0;
  shm->samplebits =
      (desired.format & 0xFF); // the first 8-bits is the sample bit size
  shm->speed = desired.freq;
  shm->channels = desired.channels;
  shm->samples = desired.samples * shm->channels;
  shm->samplepos = 0;
  shm->submission_chunk = 1;
  shm->buffer = NULL;

  return true;
}

int SNDDMA_GetDMAPos(void) { return shm->samplepos; }

void SNDDMA_Shutdown(void) { SDL_CloseAudio(); }

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void) {
  // noop
}
