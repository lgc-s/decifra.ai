/* Cabeçalho MÍNIMO de teste: declara só as funções da Raylib 5.x usadas pelo jogo.
 * Serve para verificar sintaxe e fazer uma execução "headless" (tests/stub/raylib_stub.c)
 * em ambientes sem a Raylib. NÃO substitui a Raylib real: para jogar, use o raylib.h oficial. */
#ifndef RAYLIB_STUB_H
#define RAYLIB_STUB_H

#include <stdbool.h>
#include <stdarg.h>

typedef struct Vector2 { float x, y; } Vector2;
typedef struct Rectangle { float x, y, width, height; } Rectangle;
typedef struct Color { unsigned char r, g, b, a; } Color;
typedef struct Texture { unsigned int id; int width, height, mipmaps, format; } Texture;
typedef Texture Texture2D;
typedef struct RenderTexture { unsigned int id; Texture texture; Texture depth; } RenderTexture;
typedef RenderTexture RenderTexture2D;
typedef struct Wave { unsigned int frameCount, sampleRate, sampleSize, channels; void *data; } Wave;
typedef struct AudioStream { void *buffer; void *processor; unsigned int sampleRate, sampleSize, channels; } AudioStream;
typedef struct Sound { AudioStream stream; unsigned int frameCount; } Sound;

#define LIGHTGRAY (Color){200,200,200,255}
#define WHITE     (Color){255,255,255,255}
#define BLACK     (Color){0,0,0,255}
#define BLANK     (Color){0,0,0,0}

typedef enum {
    FLAG_VSYNC_HINT = 0x00000040, FLAG_WINDOW_RESIZABLE = 0x00000004
} ConfigFlags;
typedef enum { LOG_ALL = 0, LOG_WARNING = 4, LOG_NONE = 7 } TraceLogLevel;
typedef enum { TEXTURE_FILTER_POINT = 0 } TextureFilter;
typedef enum { BLEND_ALPHA = 0, BLEND_ADDITIVE = 1 } BlendMode;

typedef enum {
    KEY_NULL = 0, KEY_SPACE = 32, KEY_ZERO = 48,
    KEY_A = 65, KEY_C = 67, KEY_D = 68, KEY_E = 69, KEY_J = 74, KEY_K = 75, KEY_L = 76,
    KEY_P = 80, KEY_S = 83, KEY_W = 87, KEY_X = 88, KEY_Z = 90,
    KEY_ESCAPE = 256, KEY_ENTER = 257, KEY_BACKSPACE = 259,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265,
    KEY_F11 = 300, KEY_LEFT_SHIFT = 340, KEY_LEFT_CONTROL = 341
} KeyboardKey;

typedef enum {
    GAMEPAD_BUTTON_LEFT_FACE_UP = 1, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_DOWN,
    GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_TRIGGER_1,
    GAMEPAD_BUTTON_LEFT_TRIGGER_2, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_2,
    GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_MIDDLE, GAMEPAD_BUTTON_MIDDLE_RIGHT
} GamepadButton;
typedef enum { GAMEPAD_AXIS_LEFT_X = 0, GAMEPAD_AXIS_LEFT_Y = 1 } GamepadAxis;

/* janela e laço */
void InitWindow(int width, int height, const char *title);
void CloseWindow(void);
bool WindowShouldClose(void);
void SetConfigFlags(unsigned int flags);
void SetTargetFPS(int fps);
void SetExitKey(int key);
void SetWindowMinSize(int width, int height);
void SetTraceLogLevel(int logLevel);
int  GetScreenWidth(void);
int  GetScreenHeight(void);
void ToggleFullscreen(void);
bool IsWindowFullscreen(void);
float  GetFrameTime(void);
double GetTime(void);
bool DirectoryExists(const char *dirPath);
bool ChangeDirectory(const char *dir);
const char *GetApplicationDirectory(void);
const char *TextFormat(const char *text, ...);

/* desenho */
void BeginDrawing(void);
void EndDrawing(void);
void ClearBackground(Color color);
void BeginTextureMode(RenderTexture2D target);
void EndTextureMode(void);
void BeginBlendMode(int mode);
void EndBlendMode(void);
RenderTexture2D LoadRenderTexture(int width, int height);
void UnloadRenderTexture(RenderTexture2D target);
void SetTextureFilter(Texture2D texture, int filter);
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawRectangle(int posX, int posY, int width, int height, Color color);
void DrawRectangleLines(int posX, int posY, int width, int height, Color color);
void DrawRectangleGradientV(int posX, int posY, int width, int height, Color top, Color bottom);
void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void DrawCircle(int centerX, int centerY, float radius, Color color);
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color);
void DrawText(const char *text, int posX, int posY, int fontSize, Color color);
int  MeasureText(const char *text, int fontSize);
Color Fade(Color color, float alpha);

/* entrada */
bool IsKeyDown(int key);
bool IsKeyPressed(int key);
bool IsGamepadAvailable(int gamepad);
bool IsGamepadButtonDown(int gamepad, int button);
bool IsGamepadButtonPressed(int gamepad, int button);
float GetGamepadAxisMovement(int gamepad, int axis);

/* áudio */
void InitAudioDevice(void);
void CloseAudioDevice(void);
bool IsAudioDeviceReady(void);
void SetMasterVolume(float volume);
Sound LoadSoundFromWave(Wave wave);
void UnloadSound(Sound sound);
void PlaySound(Sound sound);
void StopSound(Sound sound);
bool IsSoundPlaying(Sound sound);
void SetSoundVolume(Sound sound, float volume);

#endif
