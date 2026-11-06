#include "core/audio.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SR 22050
#define PI_F 3.14159265f

typedef enum { W_SINE, W_SQUARE, W_NOISE } WaveKind;

static float note_freq(int midi) { return 440.0f * powf(2.0f, (float)(midi - 69) / 12.0f); }

/* Cria um Sound a partir de samples float (-1..1). Devolve um Sound zerado em caso de falha. */
static Sound sound_from_samples(const float *s, int n, float gain) {
    Sound out;
    memset(&out, 0, sizeof out);
    int16_t *pcm = malloc((size_t)n * sizeof *pcm);
    if (!pcm) return out;
    for (int i = 0; i < n; i++) {
        float v = s[i] * gain;
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        pcm[i] = (int16_t)(v * 32000.0f);
    }
    Wave w;
    w.frameCount = (unsigned)n;
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm;
    out = LoadSoundFromWave(w);       /* a Raylib copia os dados */
    free(pcm);
    return out;
}

/* Tom com glissando f0 -> f1 e envelope de ataque curto e decaimento. */
static Sound make_tone(float f0, float f1, float dur, WaveKind kind, float gain) {
    int n = (int)(dur * SR);
    float *buf = calloc((size_t)n, sizeof *buf);
    Sound zero;
    memset(&zero, 0, sizeof zero);
    if (!buf) return zero;
    float phase = 0.0f;
    uint32_t rng = 1234567u;
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)n;
        float f = f0 + (f1 - f0) * t;
        phase += 2.0f * PI_F * f / SR;
        float env = (t < 0.04f ? t / 0.04f : 1.0f) * powf(1.0f - t, 1.6f);
        float v;
        if (kind == W_SINE) v = sinf(phase);
        else if (kind == W_SQUARE) v = sinf(phase) > 0 ? 0.6f : -0.6f;
        else { rng = rng * 1664525u + 1013904223u; v = ((float)((rng >> 9) & 0x3FF) / 512.0f) - 1.0f; }
        buf[i] = v * env;
    }
    Sound s = sound_from_samples(buf, n, gain);
    free(buf);
    return s;
}

/* Sequência curta de notas (acordes/arpejos de feedback). */
static Sound make_notes(const int *midi, int count, float note_dur, WaveKind kind, float gain) {
    int per = (int)(note_dur * SR);
    int n = per * count;
    float *buf = calloc((size_t)n, sizeof *buf);
    Sound zero;
    memset(&zero, 0, sizeof zero);
    if (!buf) return zero;
    for (int k = 0; k < count; k++) {
        float f = note_freq(midi[k]), ph = 0.0f;
        for (int i = 0; i < per; i++) {
            float t = (float)i / (float)per;
            ph += 2.0f * PI_F * f / SR;
            float v = kind == W_SQUARE ? (sinf(ph) > 0 ? 0.5f : -0.5f) : sinf(ph);
            buf[k * per + i] = v * powf(1.0f - t, 1.4f) * (t < 0.03f ? t / 0.03f : 1.0f);
        }
    }
    Sound s = sound_from_samples(buf, n, gain);
    free(buf);
    return s;
}

/* Trilha ambiente: arpejo suave com baixo, ~16 s em laço. */
static Sound make_music(int root, const int *scale, int nscale, float bpm) {
    const int beats = 32;
    float beat = 60.0f / bpm;
    int total = (int)(beats * beat * SR);
    float *buf = calloc((size_t)total, sizeof *buf);
    Sound zero;
    memset(&zero, 0, sizeof zero);
    if (!buf) return zero;
    static const int pattern[8] = { 0, 2, 4, 2, 5, 4, 2, 1 };
    for (int b = 0; b < beats * 2; b++) {                    /* colcheias */
        int start = (int)((float)b * beat * 0.5f * SR);
        int len = (int)(beat * 0.9f * SR);
        int deg = pattern[(b + (b / 16)) % 8];
        float f = note_freq(root + 12 + scale[deg % nscale] + (deg >= nscale ? 12 : 0));
        float ph = 0.0f;
        for (int i = 0; i < len && start + i < total; i++) {
            float t = (float)i / (float)len;
            ph += 2.0f * PI_F * f / SR;
            buf[start + i] += sinf(ph) * powf(1.0f - t, 2.2f) * 0.22f;
        }
    }
    for (int b = 0; b < beats; b += 4) {                      /* baixo */
        int start = (int)((float)b * beat * SR);
        int len = (int)(beat * 3.6f * SR);
        float f = note_freq(root - 12 + ((b / 4) % 2 ? scale[2 % nscale] : 0)), ph = 0.0f;
        for (int i = 0; i < len && start + i < total; i++) {
            float t = (float)i / (float)len;
            ph += 2.0f * PI_F * f / SR;
            buf[start + i] += sinf(ph) * (t < 0.1f ? t / 0.1f : 1.0f) * (1.0f - t) * 0.20f;
        }
    }
    /* fade nas pontas para o laço não estalar */
    int fade = SR / 50;
    for (int i = 0; i < fade && i < total; i++) {
        float k = (float)i / (float)fade;
        buf[i] *= k;
        buf[total - 1 - i] *= k;
    }
    Sound s = sound_from_samples(buf, total, 0.9f);
    free(buf);
    return s;
}

void audio_init(Audio *a) {
    memset(a, 0, sizeof *a);
    a->track = -1;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    a->ready = true;

    a->sfx[SFX_JUMP]       = make_tone(300, 620, 0.12f, W_SQUARE, 0.20f);
    a->sfx[SFX_WALLJUMP]   = make_tone(380, 760, 0.14f, W_SQUARE, 0.20f);
    a->sfx[SFX_DASH]       = make_tone(900, 180, 0.18f, W_NOISE, 0.16f);
    a->sfx[SFX_LAND]       = make_tone(120, 60, 0.07f, W_NOISE, 0.14f);
    a->sfx[SFX_DEATH]      = make_tone(420, 50, 0.38f, W_SQUARE, 0.22f);
    static const int cp[]  = { 72, 76, 79 };
    static const int col[] = { 79, 84, 88, 91 };
    static const int cry[] = { 84, 91 };
    static const int cor[] = { 76, 83, 88 };
    static const int wrg[] = { 55, 51 };
    a->sfx[SFX_CHECKPOINT] = make_notes(cp, 3, 0.07f, W_SINE, 0.28f);
    a->sfx[SFX_COLLECT]    = make_notes(col, 4, 0.06f, W_SQUARE, 0.18f);
    a->sfx[SFX_CRYSTAL]    = make_notes(cry, 2, 0.06f, W_SINE, 0.28f);
    a->sfx[SFX_SELECT]     = make_tone(520, 560, 0.04f, W_SQUARE, 0.12f);
    a->sfx[SFX_CONFIRM]    = make_tone(520, 900, 0.09f, W_SQUARE, 0.16f);
    a->sfx[SFX_CORRECT]    = make_notes(cor, 3, 0.08f, W_SINE, 0.30f);
    a->sfx[SFX_WRONG]      = make_notes(wrg, 2, 0.14f, W_SQUARE, 0.20f);
    a->sfx[SFX_TALK]       = make_tone(440, 460, 0.03f, W_SQUARE, 0.08f);

    static const int s_pent[]  = { 0, 2, 4, 7, 9 };        /* pentatônica maior: Departamento */
    static const int s_dor[]   = { 0, 2, 3, 5, 7, 9 };     /* dórico: cidade */
    static const int s_min[]   = { 0, 2, 3, 5, 7, 8 };     /* menor natural: escândalo */
    static const int s_lyd[]   = { 0, 2, 4, 6, 7, 9 };     /* lídio: desfecho */
    a->music[0] = make_music(60, s_pent, 5, 84.0f);
    a->music[1] = make_music(62, s_dor, 6, 100.0f);
    a->music[2] = make_music(57, s_min, 6, 92.0f);
    a->music[3] = make_music(65, s_lyd, 6, 78.0f);
    for (int i = 0; i < MUSIC_TRACKS; i++) SetSoundVolume(a->music[i], 0.55f);
}

void audio_shutdown(Audio *a) {
    if (!a->ready) return;
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(a->sfx[i]);
    for (int i = 0; i < MUSIC_TRACKS; i++) UnloadSound(a->music[i]);
    CloseAudioDevice();
    a->ready = false;
}

void audio_play(Audio *a, Sfx s) {
    if (a->ready && s >= 0 && s < SFX_COUNT) PlaySound(a->sfx[s]);
}

void audio_set_volume(Audio *a, int v) {
    if (v < 0) v = 0;
    if (v > 10) v = 10;
    if (a->ready) SetMasterVolume((float)v / 10.0f);
}

void audio_music(Audio *a, int track) {
    if (!a->ready || track == a->track) return;
    if (a->track >= 0) StopSound(a->music[a->track]);
    a->track = (track >= 0 && track < MUSIC_TRACKS) ? track : -1;
    if (a->track >= 0) PlaySound(a->music[a->track]);
}

void audio_update(Audio *a) {
    if (a->ready && a->track >= 0 && !IsSoundPlaying(a->music[a->track]))
        PlaySound(a->music[a->track]);
}
