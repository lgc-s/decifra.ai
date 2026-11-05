/* Implementação "headless" das funções da Raylib usadas pelo jogo.
 * Não abre janela nem toca áudio: apenas permite compilar, ligar e executar o jogo
 * inteiro com entradas simuladas (fuzz) para achar travamentos e erros de memória.
 *
 * Variáveis de ambiente:
 *   STUB_FRAMES  número de quadros a executar (padrão 6000)
 *   STUB_SEED    semente das entradas aleatórias (padrão 1)
 *   STUB_MODE    "fuzz" (padrão) ou "run" (anda para a direita pulando e usando dash) */
#include "raylib.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long g_frame = 0, g_max_frames = 6000;
static uint32_t g_rng = 1;
static bool g_run_mode = false, g_none_mode = false;
static bool g_down[512], g_pressed[512];
static bool g_audio = false;
static long g_draw_calls = 0, g_samples_read = 0;
static FILE *g_dump = NULL;                 /* comandos de desenho do quadro atual (se pedido) */
static bool g_in_tex = false;

static bool dump_wanted(void) {
    const char *at = getenv("STUB_DUMP_AT");
    if (!at) return false;
    char buf[256]; snprintf(buf, sizeof buf, ",%s,", at);
    char key[32]; snprintf(key, sizeof key, ",%ld,", g_frame);
    return strstr(buf, key) != NULL;
}
#define LOGC(...) do { if (g_dump) fprintf(g_dump, __VA_ARGS__); } while (0)

static uint32_t rnd(void) { g_rng = g_rng * 1664525u + 1013904223u; return g_rng >> 8; }

static void init_env(void) {
    const char *f = getenv("STUB_FRAMES"), *s = getenv("STUB_SEED"), *m = getenv("STUB_MODE");
    if (f) g_max_frames = atol(f);
    if (s) g_rng = (uint32_t)atol(s) * 2654435761u + 1u;
    g_run_mode = m && strcmp(m, "run") == 0;
    g_none_mode = m && strcmp(m, "none") == 0;
}

static const int KEYS[] = { KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_Z, KEY_SPACE, KEY_X, KEY_C,
                            KEY_E, KEY_ENTER, KEY_ESCAPE, KEY_P, KEY_A, KEY_D, KEY_W, KEY_S };

/* Decide as teclas deste quadro. */
static void script_input(void) {
    memset(g_pressed, 0, sizeof g_pressed);
    if (g_none_mode) return;
    if (g_run_mode) {
        g_down[KEY_RIGHT] = true;
        g_pressed[KEY_SPACE] = (g_frame % 45) == 0;
        g_down[KEY_SPACE] = (g_frame % 45) < 20;
        g_pressed[KEY_X] = (g_frame % 90) == 40;
        g_pressed[KEY_E] = (g_frame % 30) == 0;
        g_pressed[KEY_ENTER] = (g_frame % 30) == 1;
        return;
    }
    /* fuzz: segura algumas teclas por um tempo e aperta outras ao acaso */
    for (size_t i = 0; i < sizeof KEYS / sizeof *KEYS; i++) {
        int k = KEYS[i];
        if (rnd() % 24 == 0) g_down[k] = !g_down[k];
        if (rnd() % 14 == 0) g_pressed[k] = true;
        if (k == KEY_ESCAPE || k == KEY_P) { g_down[k] = false; if (rnd() % 60 != 0) g_pressed[k] = false; }
    }
    if (g_frame < 12) g_pressed[KEY_ENTER] = (g_frame == 6);       /* entra no jogo */
}

void InitWindow(int w, int h, const char *t) { (void)w; (void)h; (void)t; init_env(); }
void CloseWindow(void) {
    fprintf(stderr, "[stub] %ld quadros, %ld chamadas de desenho, %ld amostras de audio lidas\n", g_frame, g_draw_calls, g_samples_read);
}
bool WindowShouldClose(void) { return g_frame >= g_max_frames; }
void SetConfigFlags(unsigned int f) { (void)f; }
void SetTargetFPS(int f) { (void)f; }
void SetExitKey(int k) { (void)k; }
void SetWindowMinSize(int w, int h) { (void)w; (void)h; }
void SetTraceLogLevel(int l) { (void)l; }
int GetScreenWidth(void) { return 1280; }
int GetScreenHeight(void) { return 720; }
void ToggleFullscreen(void) {}
bool IsWindowFullscreen(void) { return false; }
float GetFrameTime(void) { return (rnd() % 7 == 0) ? 1.0f / 25.0f : 1.0f / 60.0f; }   /* alterna quadros longos */
double GetTime(void) { return (double)g_frame / 60.0; }
bool DirectoryExists(const char *p) { (void)p; return true; }
bool ChangeDirectory(const char *d) { (void)d; return true; }
const char *GetApplicationDirectory(void) { return "./"; }

const char *TextFormat(const char *text, ...) {
    static char ring[8][512];
    static int idx = 0;
    char *b = ring[idx++ & 7];
    va_list ap;
    va_start(ap, text);
    vsnprintf(b, 512, text, ap);
    va_end(ap);
    return b;
}

void BeginDrawing(void) {}
void EndDrawing(void) { g_frame++; script_input(); }
void ClearBackground(Color c) { (void)c; }
void BeginTextureMode(RenderTexture2D t) {
    (void)t; g_in_tex = true;
    if (dump_wanted()) {
        const char *pre = getenv("STUB_DUMP_PREFIX");
        char path[512]; snprintf(path, sizeof path, "%s%ld.txt", pre ? pre : "dump_", g_frame);
        g_dump = fopen(path, "w");
    }
}
void EndTextureMode(void) { g_in_tex = false; if (g_dump) { fclose(g_dump); g_dump = NULL; } }
void BeginBlendMode(int m) { LOGC("BM %d\n", m); }
void EndBlendMode(void) { LOGC("BM 0\n"); }
RenderTexture2D LoadRenderTexture(int w, int h) { RenderTexture2D r; memset(&r, 0, sizeof r); r.id = 1; r.texture.id = 1; r.texture.width = w; r.texture.height = h; return r; }
void UnloadRenderTexture(RenderTexture2D t) { (void)t; }
void SetTextureFilter(Texture2D t, int f) { (void)t; (void)f; }
void DrawTexturePro(Texture2D t, Rectangle s, Rectangle d, Vector2 o, float r, Color c) { (void)t; (void)s; (void)d; (void)o; (void)r; (void)c; g_draw_calls++; }
void DrawRectangle(int x, int y, int w, int h, Color c) { LOGC("R %d %d %d %d %d %d %d %d\n", x, y, w, h, c.r, c.g, c.b, c.a); if (w < 0 || h < 0) fprintf(stderr, "[stub] retangulo com tamanho negativo %d x %d\n", w, h); g_draw_calls++; }
void DrawRectangleLines(int x, int y, int w, int h, Color c) { LOGC("RL %d %d %d %d %d %d %d %d\n", x, y, w, h, c.r, c.g, c.b, c.a); g_draw_calls++; }
void DrawRectangleGradientV(int x, int y, int w, int h, Color a, Color b) { LOGC("GV %d %d %d %d %d %d %d %d %d %d %d %d\n", x, y, w, h, a.r, a.g, a.b, a.a, b.r, b.g, b.b, b.a); g_draw_calls++; }
void DrawLine(int a, int b, int c, int d, Color col) { (void)a; (void)b; (void)c; (void)d; (void)col; g_draw_calls++; }
void DrawCircle(int x, int y, float r, Color c) { LOGC("C %d %d %f %d %d %d %d\n", x, y, (double)r, c.r, c.g, c.b, c.a); g_draw_calls++; }
void DrawTriangle(Vector2 a, Vector2 b, Vector2 c, Color col) { LOGC("T %f %f %f %f %f %f %d %d %d %d\n", (double)a.x, (double)a.y, (double)b.x, (double)b.y, (double)c.x, (double)c.y, col.r, col.g, col.b, col.a); g_draw_calls++; }
void DrawPoly(Vector2 c, int s, float r, float rot, Color col) { LOGC("P %f %f %d %f %f %d %d %d %d\n", (double)c.x, (double)c.y, s, (double)r, (double)rot, col.r, col.g, col.b, col.a); g_draw_calls++; }
void DrawText(const char *t, int x, int y, int s, Color c) {
    LOGC("TX %d %d %d %d %d %d %d|%s\n", x, y, s, c.r, c.g, c.b, c.a, t ? t : "");
    if (!t) { fprintf(stderr, "[stub] DrawText com texto nulo\n"); abort(); }
    volatile size_t n = strlen(t); (void)n;     /* força a leitura (detecta string inválida) */
    if (g_in_tex && t[0]) {
        int w = MeasureText(t, s), sz = s < 10 ? 10 : s;
        if (x < 0 || x + w > 384 || y + sz > 212) {
            static char seen[64][96]; static int nseen = 0;
            bool dup = false;
            for (int i = 0; i < nseen; i++) if (strncmp(seen[i], t, 95) == 0) dup = true;
            if (!dup && nseen < 64) { snprintf(seen[nseen++], 96, "%s", t); fprintf(stderr, "[stub] texto fora da tela (x=%d y=%d w=%d): %s\n", x, y, w, t); }
        }
    }
    g_draw_calls++;
}
/* Mesma medida da fonte padrão da Raylib 5.0 (tabela charsWidth de rtext.c):
 * tamanhos abaixo de 10 viram 10; espaçamento = tamanho/10. */
static const int CHARS_WIDTH[224] = {
    3,1,4,6,5,7,6,2,3,3,5,5,2,4,1,7,5,2,5,5,5,5,5,5,5,5,1,1,3,4,3,6,
    7,6,6,6,6,6,6,6,6,3,5,6,5,7,6,6,6,6,6,6,7,6,7,7,6,6,6,2,7,2,3,5,
    2,5,5,5,5,5,4,5,5,1,2,5,2,5,5,5,5,5,5,5,4,5,5,5,5,5,5,3,1,3,4,4,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,5,5,5,7,1,5,3,7,3,5,4,1,7,4,3,5,3,3,2,5,6,1,2,2,3,5,6,6,6,6,
    6,6,6,6,6,6,7,6,6,6,6,6,3,3,3,3,7,6,6,6,6,6,6,5,6,6,6,6,6,6,4,6,
    5,5,5,5,5,5,9,5,5,5,5,5,2,2,3,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,5 };

int MeasureText(const char *t, int s) {
    if (s < 10) s = 10;
    int spacing = s / 10, n = 0, sum = 0;
    for (const unsigned char *p = (const unsigned char *)t; *p; ) {
        unsigned cp = *p; int len = 1;
        if (cp >= 0xC0 && cp < 0xE0 && p[1]) { cp = ((cp & 0x1F) << 6) | (p[1] & 0x3F); len = 2; }
        else if (cp >= 0xE0) { cp = 63; len = 3; }                  /* fora do Latin-1: vira '?' */
        int idx = (int)cp - 32;
        if (idx < 0 || idx >= 224) idx = 63 - 32;
        sum += CHARS_WIDTH[idx];
        n++; p += len;
    }
    if (n == 0) return 0;
    return (int)((float)sum * ((float)s / 10.0f) + (float)((n - 1) * spacing));
}
Color Fade(Color c, float a) { if (a < 0) a = 0; if (a > 1) a = 1; c.a = (unsigned char)(255.0f * a); return c; }

bool IsKeyDown(int k) { return k >= 0 && k < 512 && g_down[k]; }
bool IsKeyPressed(int k) { return k >= 0 && k < 512 && g_pressed[k]; }
bool IsGamepadAvailable(int p) { (void)p; return false; }
bool IsGamepadButtonDown(int p, int b) { (void)p; (void)b; return false; }
bool IsGamepadButtonPressed(int p, int b) { (void)p; (void)b; return false; }
float GetGamepadAxisMovement(int p, int a) { (void)p; (void)a; return 0.0f; }

void InitAudioDevice(void) { g_audio = true; }
void CloseAudioDevice(void) { g_audio = false; }
bool IsAudioDeviceReady(void) { return g_audio; }
void SetMasterVolume(float v) { (void)v; }
Sound LoadSoundFromWave(Wave w) {
    Sound s; memset(&s, 0, sizeof s);
    const int16_t *d = w.data;
    if (!d || w.sampleSize != 16 || w.channels != 1) { fprintf(stderr, "[stub] Wave invalida\n"); abort(); }
    long acc = 0;
    for (unsigned i = 0; i < w.frameCount; i++) acc += d[i];       /* lê tudo: ASan acusa estouro */
    g_samples_read += (long)w.frameCount;
    s.frameCount = w.frameCount;
    (void)acc;
    return s;
}
void UnloadSound(Sound s) { (void)s; }
void PlaySound(Sound s) { (void)s; }
void StopSound(Sound s) { (void)s; }
bool IsSoundPlaying(Sound s) { (void)s; return false; }
void SetSoundVolume(Sound s, float v) { (void)s; (void)v; }
