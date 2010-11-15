//Author: Westy92
//Title: Metronome.cpp
//Date: 12/31/2008
//Version: 1.2
//Description: Homebrew Metronome For The Nintendo Wii.
#include <iostream>
#include <gccore.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <mp3player.h>
#include "strongbeat_mp3.h" //MP3 File

using namespace std; //DECLARE NAMESPACE

// GLOBAL VARIABLES
int bpm;

// SETS UP THE WII'S VIDEO HARDWARE
void initVideo()
{
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if(rmode->viTVMode&VI_NON_INTERLACE) 
        VIDEO_WaitVSync();
}

void returnToLoader()
{
    // STOP MP3 PLAYING IF NECCESSARY
    if (MP3Player_IsPlaying())
	MP3Player_Stop();
	
    cout<<"\n\n\n     Returning to loader...";

    // EXIT REPORTING 0 ERRORS
    exit(0);	
}

// FUNCTION THAT CONSTANTLY PLAY'S MP3 (BEEP) THEN PAUSES THEN BEEPS THEN PAUSES ETC
static void *beepPauseBeepPause(void *arg)
{
int ms;

    while (1)
    {
	if (!MP3Player_IsPlaying())
	    MP3Player_PlayBuffer(strongbeat_mp3, strongbeat_mp3_size, NULL);

	ms = (60000000.0 / bpm);
        usleep(ms);     
    } 
}
 
int main(int argc, char **argv)
{
int ms;
char tempo_marking[15];
lwp_t MP3thread;
 
    // INIT VIDEO
    initVideo();

    // SETUP WIIMOTE
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

    // INIT MUSIC
    bpm = 120;
    MP3Player_Init();

    // AND START THE MP3 PLAYING THREAD (beepPauseBeepPause)
    LWP_CreateThread(&MP3thread, beepPauseBeepPause, NULL, NULL, 0, 80);
	
    while (1)
    {
	WPAD_ScanPads();
	//GET  INPUT TO CHANGE BPM
	if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
	    returnToLoader(); 
	if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_LEFT)
	    bpm = bpm - 1;
	if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_RIGHT)
	    bpm = bpm + 1;
	if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_UP)
	    bpm = bpm + 10;
	if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_DOWN)
	    bpm = bpm - 10;
        if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_ONE)
    	    bpm = 250;
        if (WPAD_ButtonsDown(0) & WIIMOTE_BUTTON_TWO)
	    bpm = 30;
     //IF GREATER THAN MAXIMUM OR LESS THAN MINIMUM, SET TO MAX OR MINIMUM
	if (bpm >= 250) 
	    bpm = 250;
	if (bpm <= 30) 
	    bpm = 30;
     //SET TEMPO MARKINGS
     if (bpm <= 60) strcpy (tempo_marking, "Largo");
     if (bpm >= 61 && bpm <= 66) strcpy (tempo_marking, "Larghetto");
     if (bpm >= 67 && bpm <= 76) strcpy (tempo_marking, "Adagio");
     if (bpm >= 77 && bpm <= 108) strcpy (tempo_marking, "Andante");
     if (bpm >= 109 && bpm <= 120) strcpy (tempo_marking, "Moderato");
     if (bpm >= 121 && bpm <= 168) strcpy (tempo_marking, "Allegro");
     if (bpm >= 169 && bpm <= 200) strcpy (tempo_marking, "Presto");
     if (bpm >= 201) strcpy (tempo_marking, "Prestissimo");
	// PRINT OUT THE CURRENT BPM
	ms = (60000000.0/bpm);
	cout<<"\x1b[2J\x1b[2;0H";
	cout<<"\n\n\n     BPM = "<<" "<<bpm;
	cout<<"\n\n\n     1 beat every "<<(ms / 1000)<<" milliseconds!" ;
	cout<<"\n\n\n     The tempo marking is "<<tempo_marking;
        
        VIDEO_WaitVSync();
    }
    return 0;
}
