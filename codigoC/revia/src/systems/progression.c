#include "systems/progression.h"
#include "core/config.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void progress_init(Progress *p) {
    memset(p, 0, sizeof *p);
    p->current_act = 1;
    p->unlocked_act = 1;
    p->trust = TRUST_START;
    p->difficulty = DIFF_NORMAL;
    p->volume = 7;
    p->ending = -1;
    for (int i = 0; i < NUM_CALLS; i++) p->call_choice[i] = -1;
}

void progress_add_trust(Progress *p, int delta) {
    int t = p->trust + delta;
    p->trust = t < 0 ? 0 : (t > 100 ? 100 : t);
}

static int popcount32(uint32_t v) {
    int n = 0;
    while (v) { n += (int)(v & 1u); v >>= 1; }
    return n;
}

int progress_calls_done_count(const Progress *p) { return popcount32(p->calls_done); }
int progress_collected_count(const Progress *p)  { return popcount32(p->collected); }

static bool calls_ok(const Progress *p, int first, int last) {
    for (int i = first; i <= last; i++)
        if (!((p->calls_done >> i) & 1u)) return false;
    return true;
}

bool progress_act_ready(const Progress *p, int act, char *msg, size_t msgsz) {
    bool quiz = (p->quiz_done >> (act - 1)) & 1u;
    bool ok = true;
    char tmp[128] = "";

    switch (act) {
    case 1:
        ok = quiz;
        if (!quiz) snprintf(tmp, sizeof tmp, "Conclua a avaliacao no terminal de perguntas.");
        break;
    case 2:
        ok = quiz && calls_ok(p, 0, 3);
        if (!ok) snprintf(tmp, sizeof tmp, "Faltam chamados (%d/4) ou a sessao de perguntas.",
                          progress_calls_done_count(p) > 4 ? 4 : progress_calls_done_count(p));
        break;
    case 3:
        ok = quiz && calls_ok(p, 4, 4) && p->bias_played && p->evidence == 7;
        if (!ok) snprintf(tmp, sizeof tmp, "Faltam: evidencias (%d/3), simulacao, chamado ou avaliacao.",
                          (p->evidence & 1) + ((p->evidence >> 1) & 1) + ((p->evidence >> 2) & 1));
        break;
    case 4:
        ok = quiz && calls_ok(p, 5, 6);
        if (!ok) snprintf(tmp, sizeof tmp, "Faltam chamados finais ou a sessao de perguntas.");
        break;
    default:
        break;
    }
    if (msg && msgsz) snprintf(msg, msgsz, "%s", ok ? "" : tmp);
    return ok;
}

void progress_complete_act(Progress *p, int act) {
    if (act < NUM_ACTS) {
        if (p->unlocked_act < act + 1) p->unlocked_act = act + 1;
        if (p->current_act < act + 1) p->current_act = act + 1;   /* nunca retrocede o "continuar" */
    }
    p->checkpoint = 0;                       /* o próximo ato começa do início */
}

const char *act_title(int act) {
    switch (act) {
    case 1: return "Ato 1 - Onboarding";
    case 2: return "Ato 2 - Rotina de Chamados";
    case 3: return "Ato 3 - O Escândalo";
    case 4: return "Ato 4 - Desfecho";
    default: return "?";
    }
}

const char *act_path(int act) {
    switch (act) {
    case 1: return "levels/act1/act1.txt";
    case 2: return "levels/act2/act2.txt";
    case 3: return "levels/act3/act3.txt";
    case 4: return "levels/act4/act4.txt";
    default: return "";
    }
}
