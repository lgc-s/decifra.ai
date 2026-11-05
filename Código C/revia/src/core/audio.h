/* Áudio: efeitos e trilhas gerados por síntese, sem arquivos externos.
 * Se não houver dispositivo de áudio, tudo vira no-op e o jogo segue normalmente. */
#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

typedef enum {
    SFX_JUMP, SFX_WALLJUMP, SFX_DASH, SFX_LAND, SFX_DEATH, SFX_CHECKPOINT, SFX_COLLECT,
    SFX_CRYSTAL, SFX_SELECT, SFX_CONFIRM, SFX_CORRECT, SFX_WRONG, SFX_TALK, SFX_COUNT
} Sfx;

#define MUSIC_TRACKS 4

typedef struct {
    bool ready;
    Sound sfx[SFX_COUNT];
    Sound music[MUSIC_TRACKS];
    int track;               /* trilha tocando, -1 nenhuma */
} Audio;

void audio_init(Audio *a);
void audio_shutdown(Audio *a);
void audio_play(Audio *a, Sfx s);
void audio_set_volume(Audio *a, int volume_0_10);
/* track: 0..3 ou -1 para parar. Troca só se for diferente da atual. */
void audio_music(Audio *a, int track);
/* Reinicia a trilha quando ela termina (laço). */
void audio_update(Audio *a);

#endif
