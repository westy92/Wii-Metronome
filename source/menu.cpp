/****************************************************************************
 * Wii Metronome
 * Westy92 2009-2010
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "libwiigui/gui.h"
#include "menu.h"
#include "demo.h"
#include "input.h"
#include "filelist.h"

#define THREAD_SLEEP 100

using namespace std;

static GuiImageData *pointer[4];
static GuiImage *bgImg = NULL;
static GuiWindow *mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static lwp_t Beepthread;
static lwp_t Timerthread;
static bool guiHalt = true;
int bpm = 120;
int beat = 4;
int sec_elapsed = 0;
int min_elapsed = 0;
char tempo_marking_char[11] = "Moderato";
int maxbpm = 300;
int minbpm = 30;


/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *
UpdateGUI (void *arg)
{
	int i;

	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
		}
		else
		{
			UpdatePads();
			mainWindow->Draw();

			#ifdef HW_RVL
			for(i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i].wpad->ir.valid)
					Menu_DrawImg(userInput[i].wpad->ir.x-48, userInput[i].wpad->ir.y-48,
						96, 96, pointer[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
				DoRumble(i);
			}
			#endif

			Menu_Render();

			for(i=0; i < 4; i++)
				mainWindow->Update(&userInput[i]);

			if(ExitRequested)
			{
				for(i = 0; i < 255; i += 15)
				{
					mainWindow->Draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, i},1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}
	return NULL;
}


/****************************************************************************
 * Beep
 *
 * Make the metronome tick!
 ***************************************************************************/
static void *Beep(void *arg){
	GuiSound beepSound(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiSound clickSound(button_click_pcm, button_click_pcm_size, SOUND_PCM);
	int ms;
	int measure = 4;
	while (1){
		if (!beepSound.IsPlaying()){
			if(measure < beat){
				beepSound.Play();
				ms = (60000000.0 / bpm);
				usleep(ms);
				measure++;
			}else{
				clickSound.Play();
				ms = (60000000 / bpm);
				usleep(ms);
				measure = 1;
			}
		}
	}
	return 0;
}


/****************************************************************************
 * Timer
 *
 * Practice timer!
 ***************************************************************************/
static void *Timer(void *arg){
	while(1){
		usleep(1000000);	//1 second
		sec_elapsed++;
		if(sec_elapsed > 59){
			sec_elapsed -= 60;
			min_elapsed++;
		}
	}
	return 0;
}


/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads()
{
	LWP_CreateThread(&guithread, UpdateGUI, NULL, NULL, 0, 70);
	LWP_CreateThread(&Beepthread, Beep, NULL, NULL, 0, 120);
	LWP_CreateThread(&Timerthread, Timer, NULL, NULL, 0, 80);
}


/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings()
{
	int menu = MENU_NONE;

	GuiImageData bg(bg_png);
	GuiImage bgImg(&bg);
	bgImg.SetPosition(((screenwidth - (bgImg.GetWidth()))/2), ((screenheight - (bgImg.GetHeight()))/2));

	char BpmChar[3];
	sprintf(BpmChar, "%3d", bpm);

	GuiText bpm_text(BpmChar, 100, (GXColor){0, 0, 0, 255});
	bpm_text.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	bpm_text.SetPosition(309,196);

	char BeatChar[3];
	sprintf(BeatChar, "%d", beat);

	GuiText beat_text(BeatChar, 100, (GXColor){0, 0, 0, 255});
	beat_text.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	beat_text.SetPosition(211,196);

	char TimerChar[5];
	sprintf(TimerChar, "%02d:%02d", min_elapsed, sec_elapsed);

	GuiText timer_text(TimerChar, 30, (GXColor){0, 0, 0, 255});
	timer_text.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	timer_text.SetPosition(197,283);

	char MarkingChar[11];
	sprintf(MarkingChar, "%11s", tempo_marking_char);

	GuiText tempo_marking(MarkingChar, 30, (GXColor){0, 0, 0, 255});
	tempo_marking.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	tempo_marking.SetPosition(297,283);

	//Declare files used...
	GuiSound beepSound(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiSound clickSound(button_click_pcm, button_click_pcm_size, SOUND_PCM);
	GuiImageData btnExit(exit_png);
	GuiImageData btnExitOver(exit_over_png);
	GuiImageData btnTempoUp(up_png);
	GuiImageData btnTempoUpOver(up_over_png);
	GuiImageData btnTempoDown(down_png);
	GuiImageData btnTempoDownOver(down_over_png);
	GuiImageData btnRefresh(refresh_png);
	GuiImageData btnRefreshOver(refresh_over_png);
	GuiImageData btnPause(pause_png);
	GuiImageData btnPauseOver(pause_over_png);
	GuiImageData btnPlay(play_png);
	GuiImageData btnPlayOver(play_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);
	GuiTrigger trigPlus;

	GuiImage refreshImg(&btnRefresh);
	GuiImage refreshImgOver(&btnRefreshOver);
	GuiButton refresh(btnRefresh.GetWidth(), btnRefresh.GetHeight());
	refresh.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	refresh.SetPosition(85, 200);
	refresh.SetImage(&refreshImg);
	refresh.SetImageOver(&refreshImgOver);
	refresh.SetTrigger(&trigA);

	GuiImage pauseImg(&btnPause);
	GuiImage pauseImgOver(&btnPauseOver);
	GuiButton pause(btnPause.GetWidth(), btnPause.GetHeight());
	pause.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	pause.SetPosition(85, 270);
	pause.SetImage(&pauseImg);
	pause.SetImageOver(&pauseImgOver);
	pause.SetTrigger(&trigA);

	GuiImage playImg(&btnPlay);
	GuiImage playImgOver(&btnPlayOver);
	GuiButton play(btnPlay.GetWidth(), btnPlay.GetHeight());
	play.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	play.SetPosition(85, 270);
	play.SetImage(&playImg);
	play.SetImageOver(&playImgOver);
	play.SetTrigger(&trigA);

	GuiImage tempoUpImg(&btnTempoUp);
	GuiImage tempoUpImgOver(&btnTempoUpOver);
	GuiButton tempoUp(btnTempoUp.GetWidth(), btnTempoUp.GetHeight());
	tempoUp.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	tempoUp.SetPosition(347, 100);
	tempoUp.SetImage(&tempoUpImg);
	tempoUp.SetImageOver(&tempoUpImgOver);
	tempoUp.SetTrigger(&trigA);

	GuiImage tempoDownImg(&btnTempoDown);
	GuiImage tempoDownImgOver(&btnTempoDownOver);
	GuiButton tempoDown(btnTempoDown.GetWidth(), btnTempoDown.GetHeight());
	tempoDown.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	tempoDown.SetPosition(347, 350);
	tempoDown.SetImage(&tempoDownImg);
	tempoDown.SetImageOver(&tempoDownImgOver);
	tempoDown.SetTrigger(&trigA);

	GuiImage beatUpImg(&btnTempoUp);
	GuiImage beatUpImgOver(&btnTempoUpOver);
	GuiButton beatUp(btnTempoUp.GetWidth(), btnTempoUp.GetHeight());
	beatUp.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	beatUp.SetPosition(200, 100);
	beatUp.SetImage(&beatUpImg);
	beatUp.SetImageOver(&beatUpImgOver);
	beatUp.SetTrigger(&trigA);

	GuiImage beatDownImg(&btnTempoDown);
	GuiImage beatDownImgOver(&btnTempoDownOver);
	GuiButton beatDown(btnTempoDown.GetWidth(), btnTempoDown.GetHeight());
	beatDown.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	beatDown.SetPosition(200, 350);
	beatDown.SetImage(&beatDownImg);
	beatDown.SetImageOver(&beatDownImgOver);
	beatDown.SetTrigger(&trigA);

	GuiImage exitBtnImg(&btnExit);
	GuiImage exitBtnImgOver(&btnExitOver);
	GuiButton exitBtn(btnExit.GetWidth(), btnExit.GetHeight());
	exitBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	exitBtn.SetPosition(490, 200);
	exitBtn.SetImage(&exitBtnImg);
	exitBtn.SetImageOver(&exitBtnImgOver);
	exitBtn.SetTrigger(&trigA);
	exitBtn.SetTrigger(&trigHome);

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	w.SetPosition(0, 0);
	w.Append(&bgImg);
	w.Append(&bpm_text);
	w.Append(&beat_text);
	w.Append(&timer_text);
	w.Append(&tempo_marking);
	w.Append(&refresh);
	w.Append(&pause);
	w.Append(&beatDown);
	w.Append(&beatUp);
	w.Append(&tempoDown);
	w.Append(&tempoUp);
	w.Append(&exitBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_1){
			bpm = 300;
			if (bpm >= maxbpm) bpm = maxbpm;

			if (bpm >= 201) strcpy (tempo_marking_char, "Prestissimo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			bpm_text.SetText(BpmChar);
		}
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_2){
			bpm = 30;
			if (bpm <= minbpm) bpm = minbpm;

			if (bpm <= 60) strcpy (tempo_marking_char, "Largo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			bpm_text.SetText(BpmChar);
		}	
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS){
			bpm += 10;
			if (bpm >= maxbpm) bpm = maxbpm;

			if (bpm <= 60) strcpy (tempo_marking_char, "Largo");
			if (bpm >= 61 && bpm <= 66) strcpy (tempo_marking_char, "Larghetto");
			if (bpm >= 67 && bpm <= 76) strcpy (tempo_marking_char, "Adagio");
			if (bpm >= 77 && bpm <= 108) strcpy (tempo_marking_char, "Andante");
			if (bpm >= 109 && bpm <= 120) strcpy (tempo_marking_char, "Moderato");
			if (bpm >= 121 && bpm <= 168) strcpy (tempo_marking_char, "Allegro");
			if (bpm >= 169 && bpm <= 200) strcpy (tempo_marking_char, "Presto");
			if (bpm >= 201) strcpy (tempo_marking_char, "Prestissimo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			tempoUp.ResetState();
			bpm_text.SetText(BpmChar);
		}
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_MINUS){
			bpm -= 10;
			if (bpm <= minbpm) bpm = minbpm;

			if (bpm <= 60) strcpy (tempo_marking_char, "Largo");
			if (bpm >= 61 && bpm <= 66) strcpy (tempo_marking_char, "Larghetto");
			if (bpm >= 67 && bpm <= 76) strcpy (tempo_marking_char, "Adagio");
			if (bpm >= 77 && bpm <= 108) strcpy (tempo_marking_char, "Andante");
			if (bpm >= 109 && bpm <= 120) strcpy (tempo_marking_char, "Moderato");
			if (bpm >= 121 && bpm <= 168) strcpy (tempo_marking_char, "Allegro");
			if (bpm >= 169 && bpm <= 200) strcpy (tempo_marking_char, "Presto");
			if (bpm >= 201) strcpy (tempo_marking_char, "Prestissimo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			tempoDown.ResetState();
			bpm_text.SetText(BpmChar);
		}
		if(TimerChar != ((char*)(sprintf(TimerChar, "%02d:%02d", min_elapsed, sec_elapsed))))
		{
			sprintf(TimerChar, "%02d:%02d", min_elapsed, sec_elapsed);
			timer_text.SetText(TimerChar);
			VIDEO_WaitVSync();
		}
		if(pause.GetState() == STATE_CLICKED)
		{
			LWP_SuspendThread(Timerthread);
			// wait for thread to finish
			while(!LWP_ThreadIsSuspended(Timerthread))
				usleep(THREAD_SLEEP);
			pause.ResetState();
			HaltGui();
			w.Remove(&pause);
			w.Append(&play);
			ResumeGui();
		}
		if(play.GetState() == STATE_CLICKED)
		{
			LWP_ResumeThread(Timerthread);
			play.ResetState();
			HaltGui();
			w.Remove(&play);
			w.Append(&pause);
			ResumeGui();
		}
		if(refresh.GetState() == STATE_CLICKED)
		{
			min_elapsed = 0;
			sec_elapsed = 0;
			refresh.ResetState();
			sprintf(TimerChar, "%02d:%02d", min_elapsed, sec_elapsed);
			timer_text.SetText(TimerChar);
		}
		if(tempoDown.GetState() == STATE_CLICKED)
		{
			bpm -= 1;
			if (bpm <= minbpm) bpm = minbpm;

			if (bpm <= 60) strcpy (tempo_marking_char, "Largo");
			if (bpm >= 61 && bpm <= 66) strcpy (tempo_marking_char, "Larghetto");
			if (bpm >= 67 && bpm <= 76) strcpy (tempo_marking_char, "Adagio");
			if (bpm >= 77 && bpm <= 108) strcpy (tempo_marking_char, "Andante");
			if (bpm >= 109 && bpm <= 120) strcpy (tempo_marking_char, "Moderato");
			if (bpm >= 121 && bpm <= 168) strcpy (tempo_marking_char, "Allegro");
			if (bpm >= 169 && bpm <= 200) strcpy (tempo_marking_char, "Presto");
			if (bpm >= 201) strcpy (tempo_marking_char, "Prestissimo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			tempoDown.ResetState();
			bpm_text.SetText(BpmChar);
		}
		else if(tempoUp.GetState() == STATE_CLICKED)
		{
			bpm += 1;
			if (bpm >= maxbpm) bpm = maxbpm;

			if (bpm <= 60) strcpy (tempo_marking_char, "Largo");
			if (bpm >= 61 && bpm <= 66) strcpy (tempo_marking_char, "Larghetto");
			if (bpm >= 67 && bpm <= 76) strcpy (tempo_marking_char, "Adagio");
			if (bpm >= 77 && bpm <= 108) strcpy (tempo_marking_char, "Andante");
			if (bpm >= 109 && bpm <= 120) strcpy (tempo_marking_char, "Moderato");
			if (bpm >= 121 && bpm <= 168) strcpy (tempo_marking_char, "Allegro");
			if (bpm >= 169 && bpm <= 200) strcpy (tempo_marking_char, "Presto");
			if (bpm >= 201) strcpy (tempo_marking_char, "Prestissimo");
			sprintf(MarkingChar, "%11s", tempo_marking_char);
			tempo_marking.SetText(MarkingChar);

			sprintf(BpmChar, "%3d", bpm);
			tempoUp.ResetState();
			bpm_text.SetText(BpmChar);
		}
		else if(beatDown.GetState() == STATE_CLICKED)
		{
			beat -= 1;
			if (beat <= 1)
				beat = 1;
			sprintf(BeatChar, "%d", beat);
			beatDown.ResetState();
			beat_text.SetText(BeatChar);
		}
		else if(beatUp.GetState() == STATE_CLICKED)
		{
			beat += 1;
			if (beat >= 9)
				beat = 9;
			sprintf(BeatChar, "%d", beat);
			beatUp.ResetState();
			beat_text.SetText(BeatChar);
		}
		else if(exitBtn.GetState() == STATE_CLICKED)
		{
			if(beepSound.IsPlaying()){
				beepSound.Stop();
			}
			if(clickSound.IsPlaying()){
				clickSound.Stop();
			}
			menu = MENU_EXIT;
		}
	}
	

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}


/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	#ifdef HW_RVL
	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);
	#endif

	mainWindow = new GuiWindow(screenwidth, screenheight);

	bgImg = new GuiImage(screenwidth, screenheight, (GXColor){21, 28, 43, 255});
	mainWindow->Append(bgImg);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	ResumeGui();

	while(currentMenu != MENU_EXIT)
	{
		switch (currentMenu)
		{
			case MENU_SETTINGS:
				currentMenu = MenuSettings();
				break;
			default: // unrecognized menu
				currentMenu = MenuSettings();
				break;
		}
	}

	ResumeGui();
	ExitRequested = 1;
	while(1) usleep(THREAD_SLEEP);

	HaltGui();

	delete bgImg;
	delete mainWindow;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	mainWindow = NULL;
}
