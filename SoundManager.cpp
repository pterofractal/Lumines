/* ***************************************************************
 *
 * File : SoundManager.cpp
 *
 * Author : Tiberiu Popa
 * 	    J. Alexander Clarke
 * Date   : June 18th, 2002
 *
 * Modified:
 *
 * Purpose: Implementation of the sound manager
 *
 * ****************************************************************/
#pragma implementation
#include "SoundManager.hpp"
#include <stdlib.h>


SoundManager SM;

SoundManager::SoundManager(){

    nSounds = 0;
    nMusics = 0;

    // initialize the sound library
    SDL_Init(SDL_INIT_AUDIO /*| SDL_INIT_VIDEO */);

    /* sound setup */
    int audio_rate = 44100;
    Uint16 audio_format = AUDIO_S16; 
    int audio_channels = 2;
    int audio_buffers = 4096;

    // open the audio stream 
    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
         printf("Unable to open audio!\n");
         exit(1);
       }
}

SoundManager::~SoundManager(){
}


/* *************** Manipulate Sound Chunks *******************/

int SoundManager::ResetSound(int snd){
    if(snd<0 || snd>=nSounds) return -1;

    Mix_HaltChannel(channel[snd]);
    channel[snd] = -1;

    return 0;
}

int SoundManager::LoadSound(char* file){
 
    chunks[nSounds] = Mix_LoadWAV(file);
    channel[nSounds] = -1;

    nSounds+=1;
    return nSounds - 1;

}

int SoundManager::PlaySound(int snd){
    if(snd<0 && snd>=nSounds) return -1;
    channel[snd] = Mix_PlayChannel(-1, chunks[snd], 0 /*-1*/);
    return 0;
}

int SoundManager::StopSound(int snd){
    if(snd<0 && snd>=nSounds) return -1;
    ResetSound(snd);
    return 0;
}

int SoundManager::PauseSound(int snd){
    if(snd<0 && snd>=nSounds) return -1;
    Mix_Pause(channel[snd]);
    return 0;
}

int SoundManager::ResumeSound(int snd){
    if(snd<0 && snd>=nSounds) return -1;
    Mix_Resume(channel[snd]);
    return 0;
}


/* *************** Manipulate Music *******************/

int SoundManager::ResetMusic(int snd){
    if(snd<0 || snd>=nMusics) return -1;

    Mix_HaltMusic();

    return 0;
}

int SoundManager::LoadMusic(char* file){
 
    music[nMusics] = Mix_LoadMUS(file);
	if(!music[nMusics]) 
	{
	    printf("Mix_LoadMUS(\"music.mp3\"): %s\n", Mix_GetError());
	    // this might be a critical error...
	}
    nMusics+=1;
    return nMusics - 1;

}

int SoundManager::PlayMusic(int snd){
    if(snd<0 && snd>=nMusics) return -1;
    //Mix_PlayMusic(music[snd], 1 /*-1*/);

	    if(Mix_PlayMusic(music[snd], -1)==-1) 
	{
	    printf("Mix_PlayMusic: %s\n", Mix_GetError());
	    // well, there's no music, but most games don't break without music...
	}
    return 0;
}

int SoundManager::StopMusic(int snd){
    if(snd<0 && snd>=nMusics) return -1;
    ResetMusic(snd);
    return 0;
}

int SoundManager::PauseMusic(int snd){
    if(snd<0 && snd>=nMusics) return -1;
    Mix_PauseMusic();
    return 0;
}

int SoundManager::ResumeMusic(int snd){
    if(snd<0 && snd>=nMusics) return -1;
    Mix_ResumeMusic();
    return 0;
}
