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
//	DOOM graphics stuff for SFML
//
//-----------------------------------------------------------------------------

#include "neapolitan.h"
#include <stdlib.h>
#include <unistd.h>

#include <SFML/Graphics.h>
#include <SFML/System.h>
#include <SFML/Window.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_input.h"

#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"
#include <locale.h>

static const char window_title[] = "Neapolitan Doom";


sfRenderWindow* window;
sfTexture* texture;
sfSprite* image;

sfEvent event;
extern int lastPressedKey;

float windowScale = 1;

//convert sfKey to ascii
int sfKeyAscii(void)
{
	return event.text.unicode;
}


void I_ShutdownGraphics(void)
{
	if(window)
		sfRenderWindow_destroy(window);
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // TODO: do joystick things here
}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;

void PreserveAspectRatio()
{
	float aspect = SCREENWIDTH / (SCREENHEIGHT * 1.2);
	unsigned int m_width = event.size.width;
	unsigned int m_height = event.size.height;
	float new_width = aspect * m_height;
	float new_height = m_width / aspect;

	sfView* view = sfRenderWindow_getView(window);
	float offset_width = (m_width - new_width) / 2.0;
    float offset_height = (m_height - new_height) / 2.0;


	if (m_width >= aspect * m_height) {
		sfFloatRect rect;
		rect.left = offset_width / m_width;
		rect.top = 0;
		rect.width = new_width / m_width;
		rect.height = 1.0;
		sfView_setViewport(view, rect);

    } else {
		sfFloatRect rect;
		rect.left = 0.0;
		rect.top = offset_height / m_height;
		rect.width = 1.0;
		rect.height = new_height / m_height;
		sfView_setViewport(view, rect);
    }

	windowScale = (1.0/SCREENWIDTH) * sfView_getSize(view).x;// height;

	sfRenderWindow_setView(window, view);
}

void I_GetEvent(void)
{
	mouseEvX = 0;
	mouseEvY = 0;
	I_HandleMouse(window, windowScale);	
    while(sfRenderWindow_pollEvent(window, &event))
	{
		event_t d_event;
		switch (event.type)
		{
		case sfEvtTextEntered:
			d_event.type = ev_textentered;		
			int ascii = sfKeyAscii();
			d_event.data2 =  ascii;
			d_event.data3 = ascii;
			D_PostEvent(&d_event);
			return;
			break;

		case sfEvtKeyPressed:
			d_event.type = ev_keydown;
			int key = event.key.code;
			d_event.data1 = key;
			lastPressedKey = key;
			D_PostEvent(&d_event);

			break;

		case sfEvtKeyReleased:
			d_event.type = ev_keyup;
			int keyreleased = event.key.code;
			if(keyreleased >= 0 && keyreleased < 256)
			{
				d_event.data1 = keyreleased;
				D_PostEvent(&d_event);
			}
			break;

		case sfEvtResized:
			PreserveAspectRatio();
			break;

		case sfEvtClosed:
			I_Quit();
			break;
		default:
			break;
		}
	}
		

}


//
// I_StartTic
//
void I_StartTic (void)
{

    if (!window)
		return;
	sfRenderWindow_clear(window, sfColor_fromRGB(0,0,0));
	
	I_GetEvent();


    mousemoved = false;

}

//
// I_FinishUpdate
//

static byte	colors[768];

void I_FinishUpdate (void)
{
    byte argb_buffer[SCREENWIDTH * SCREENHEIGHT * 4];
	for(int i = 0; i < SCREENWIDTH * SCREENHEIGHT; i++)
	{
		unsigned short colorIndex = 3 * screens[0][i];
		byte* c = &colors[colorIndex];

		unsigned long pixelIndex = 4 * i;
		argb_buffer[pixelIndex++] = *c++;
        argb_buffer[pixelIndex++] = *c++;
        argb_buffer[pixelIndex++] = *c++;
        argb_buffer[pixelIndex++] = 255;
	}

	sfTexture_updateFromPixels(texture, argb_buffer, SCREENWIDTH, SCREENHEIGHT, 0, 0);
	sfSprite_setTexture(image, texture, true);

	sfRenderWindow_drawSprite(window, image, NULL);
	sfRenderWindow_display(window);
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
	memcpy(colors, palette, 768);
}

void I_InitGraphics(void)
{
    static int		firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    signal(SIGINT, (void (*)(int)) I_Quit);
  
	printf("starting sfwindow..\n");

	sfVideoMode mode;
	mode.width = SCREENWIDTH;
	mode.height = TRUEHEIGHT;

	char title[64];
	sprintf(title, "%s %s", window_title, NEAPOLITAN_VERSION);

	window = sfRenderWindow_create(mode, title, sfDefaultStyle, NULL);
	sfRenderWindow_setFramerateLimit(window, 35);
	texture = sfTexture_create(SCREENWIDTH, SCREENHEIGHT);

	image = sfSprite_create();

	//apply aspect ratio correction
	sfVector2f spritescale;
	spritescale.x = 1;
	spritescale.y = 1.2;

	sfSprite_setScale(image, spritescale);
	
	screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
}
