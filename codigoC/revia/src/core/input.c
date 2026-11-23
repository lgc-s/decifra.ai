#include "core/input.h"
#include "raylib.h"

#include <math.h>
#include <string.h>

static bool pad_down(int b)    { return IsGamepadAvailable(0) && IsGamepadButtonDown(0, b); }
static bool pad_pressed(int b) { return IsGamepadAvailable(0) && IsGamepadButtonPressed(0, b); }

void input_poll(GameInput *in) {
    memset(in, 0, sizeof *in);
    float ax = 0.0f, ay = 0.0f;
    if (IsGamepadAvailable(0)) {
        ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        ay = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    }
    const float dz = 0.35f;

    bool left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A) || ax < -dz || pad_down(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) || ax > dz  || pad_down(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    bool up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W) || ay < -dz || pad_down(GAMEPAD_BUTTON_LEFT_FACE_UP);
    bool down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S) || ay > dz  || pad_down(GAMEPAD_BUTTON_LEFT_FACE_DOWN);

    in->pi.left = left && !right;
    in->pi.right = right && !left;
    in->pi.up = up && !down;
    in->pi.down = down && !up;

    in->pi.jump_pressed = IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_K) ||
                          pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    in->pi.jump_held = IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_K) ||
                       pad_down(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    in->pi.dash_pressed = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_J) ||
                          pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_LEFT) || pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    in->pi.grab_held = IsKeyDown(KEY_C) || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_L) ||
                       pad_down(GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || pad_down(GAMEPAD_BUTTON_RIGHT_TRIGGER_2);

    /* navegação de menus: setas, WASD ou direcional (só na borda) */
    in->up_p    = IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_UP);
    in->down_p  = IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    in->left_p  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    in->right_p = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

    in->confirm  = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE) ||
                   pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    in->back     = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X) || pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    in->interact = IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_UP);
    in->pause    = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || pad_pressed(GAMEPAD_BUTTON_MIDDLE_RIGHT);
}
