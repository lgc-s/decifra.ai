#include "core/config.h"

Tuning config_tuning(Difficulty d) {
    Tuning t = { 0.10f, 0.10f, 110.0f, 0.30f, false };
    switch (d) {
    case DIFF_ASSIST:
        t.coyote_time = 0.16f;
        t.jump_buffer_time = 0.16f;
        t.stamina_max = 170.0f;
        t.respawn_delay = 0.25f;
        t.infinite_dash = true;
        break;
    case DIFF_CHALLENGE:
        t.coyote_time = 0.06f;
        t.jump_buffer_time = 0.06f;
        t.stamina_max = 90.0f;
        t.respawn_delay = 0.35f;
        break;
    default:
        break;
    }
    return t;
}

const char *config_difficulty_name(Difficulty d) {
    switch (d) {
    case DIFF_ASSIST:    return "Assistido";
    case DIFF_CHALLENGE: return "Desafio";
    default:             return "Normal";
    }
}
