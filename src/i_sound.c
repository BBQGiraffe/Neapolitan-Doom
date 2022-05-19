// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_unix.c,v 1.5 1997/02/03 22:45:10 b1 Exp $";
#include "mus2mid.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fluidsynth.h>
#include <SFML/Audio.h>

#include "z_zone.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "doomdef.h"

#define SAMPLECOUNT		512
#define SFX_SAMPLERATE 11025	// Hz
#define MUS_SAMPLERATE 44100


//fluidsynth stuff
fluid_settings_t *settings;
fluid_synth_t *synth;
fluid_audio_driver_t *adriver;
fluid_player_t *player = NULL;
sfSoundStream* midiStream;

#define SFMIDI_LOADERFRAMES 2048
#define MIDI_BUFFERSIZE SFMIDI_LOADERFRAMES * 2

static unsigned char midi[1024*1024];
int midiLength;

//stores rendered MIDI output
sfUint16* midiPCMData = NULL;


// The actual lengths of all sound effects.
int 		lengths[NUMSFX];

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void* getsfx( char* sfxname, int* len)
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;

    
    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);
    
    size = W_LumpLength( sfxlump );

    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)malloc( paddedsize+8);

    // Now copy and pad.
    memcpy(paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;
    
    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
}
 
void I_SetSfxVolume(int volume)
{
  snd_SfxVolume = volume;
}


#include <SFML/Audio.h>
sfSound* sounds[NUMSFX];

int cursong = -1;

void I_SetMusicVolume(int volume)
{
  snd_MusicVolume = volume;
  sfSoundStream_setVolume(midiStream, (100.0 / 15) * (float)volume);
}


//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}



int I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{
  sfSound* sound = sounds[id];
  sfSound_setVolume(sound,  (100.0 / snd_SfxVolume) * (float)vol);
  
  if(snd_DoPitchShift)
    sfSound_setPitch(sound, (1.0 / 255.0) * (float)pitch);

  sfSound_play(sound);
}

void I_UpdateSoundParams()
{
  if(!snd_DoPitchShift)
    for(int i = 0; i < NUMSFX; i++)
      sfSound_setPitch(sounds[i], 1);
}

//TODO
void I_StopSound (int handle)
{
  // You need the handle returned by StartSound.
  // Would be looping all channels,
  //  tracking down the handle,
  //  an setting the channel to zero.
  
  // UNUSED.
  handle = 0;
}


int I_SoundIsPlaying(int handle)
{
    // Ouch.
    return gametic < handle;
}


void I_UpdateSound( void )
{
  int doneplaying = fluid_player_get_status(player) == FLUID_PLAYER_DONE;


  if(doneplaying)
  {
    fluid_player_seek(player, 0);
    fluid_player_play(player);
    sfSoundStream_play(midiStream); 
  }
}

void I_ShutdownSound(void)
{    

}


void
I_InitSound()
{ 
  I_InitMusic();
  for (int i=1 ; i<NUMSFX ; i++)
  { 
    if (!S_sfx[i].link)
    {
      // Load data from WAD file.
      S_sfx[i].data = getsfx( S_sfx[i].name, &lengths[i] );

      sfInt16 data[lengths[i]];



      //TODO: try to remember what this does so I can make a better comment
      byte* raw = (byte*)S_sfx[i].data;
      for(int k = 0; k < lengths[i]; k++)
      {
        data[k] = (sfInt16)((raw[k] - 128) << 8);
      }


      sfSoundBuffer* soundbuffer = sfSoundBuffer_createFromSamples(data, lengths[i], 1, SFX_SAMPLERATE);
      sfSound* sound = sfSound_create();
      sfSound_setBuffer(sound, soundbuffer);
      sounds[i] = sound;
    }	
  }

}



void I_LoadSoundFont(char* filename)
{
  fluid_synth_sfload(synth, filename, 1);
}

sfBool sfMidiOnGetData(sfSoundStreamChunk* chunk, void* data)
{
  chunk->sampleCount = SFMIDI_LOADERFRAMES;

  fluid_synth_write_s16(synth, SFMIDI_LOADERFRAMES / 2, midiPCMData, 0, 2, midiPCMData, 1, 2);

  if(!chunk->samples)
    chunk->samples = midiPCMData;
}

void sfMidiOnSeek(sfTime time, void* data)
{
  //this doesn't really do anything but it doesn't let you pass NULL, yay dummy function!
}

void I_InitMusic(void)
{
  settings = new_fluid_settings();

  midiStream = sfSoundStream_create(&sfMidiOnGetData, &sfMidiOnSeek, 2, MUS_SAMPLERATE, NULL);  
  
  synth = new_fluid_synth(settings);
  fluid_synth_set_sample_rate(synth, MUS_SAMPLERATE);
  

  fluid_settings_setstr(settings, "audio.driver", "alsa");
  fluid_settings_setint(settings, "audio.period-size", 0);
  midiPCMData = malloc(MIDI_BUFFERSIZE);
}

//TODO
void I_ShutdownMusic(void)
{
  
}

static int	looping=0;
static int	musicdies=-1;


int songid = 0;


void I_PlaySong(int handle, int looping)
{
  if(player)
    delete_fluid_player(player);

  player = new_fluid_player(synth);

  fluid_player_add_mem(player, midi, midiLength);
  fluid_player_play(player);

  sfSoundStream_play(midiStream); 
  sfSoundStream_setLoop(midiStream, looping);
}

void I_PauseSong (int handle)
{
  sfSoundStream_pause(midiStream);
}



void I_ResumeSong (int handle)
{
  sfSoundStream_play(midiStream);
}

void I_StopSong(int handle)
{
  sfSoundStream_stop(midiStream);
}

char musheader[3] = {'M','U','S'};
boolean IsMus(char* data)
{
  boolean mus = true;
  for(int i = 0; i < 3; i++)
  {
    if(data[i] != musheader[i])
    {
      mus = false;
    }
  }
  return mus;
}

int I_RegisterSong(void *data, int lumplength)
{  
  if(IsMus(data))
  {
    Mus2Midi(data, midi, &midiLength);
  }else{
    memcpy(midi, data, lumplength);
    midiLength = lumplength;
  }

  songid++;
  return songid - 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  // UNUSED.
  handle = 0;
  return looping || musicdies > gametic;
}
