/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libwiigui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "demo.h"

int ExitRequested = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

int
main(int argc, char *argv[])
{
	InitVideo(); // Initialize video
	SetupPads(); // Initialize input
	InitAudio(); // Initialize audio
	InitFreeType((u8*)font_ttf, font_ttf_size); // Initialize font system
	InitGUIThreads(); // Initialize GUI

	MainMenu(MENU_SETTINGS);
}
