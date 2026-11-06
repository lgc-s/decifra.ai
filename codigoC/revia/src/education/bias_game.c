#include "education/bias_game.h"

#include <math.h>
#include <string.h>

#define GROUP_B_PENALTY 18     /* rigor extra do histórico contra o grupo B */
#define APPROVAL_CUTOFF 50
#define PROXY_STRENGTH  0.7f
#define WEIGHT_GAIN     1.5f

static uint32_t rng_next(uint32_t *s) {
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8;
}

static int rng_range(uint32_t *s, int lo, int hi) {   /* inclusivo */
    return lo + (int)(rng_next(s) % (uint32_t)(hi - lo + 1));
}

static bool correct_label(const BiasRecord *r) { return r->q + r->noise >= APPROVAL_CUTOFF; }

/* Qualificação bem distribuída (sequência de razão áurea): qualquer prefixo da lista
 * cobre toda a faixa 20..95, então a taxa de aprovação não depende do acaso. */
static int spread_q(int i, float offset) {
    float f = (float)i * 0.6180339f + offset;
    f -= floorf(f);
    return 20 + (int)(f * 75.0f);
}

void bias_init(BiasGame *g, uint32_t seed) {
    memset(g, 0, sizeof *g);
    uint32_t s = seed ? seed : 777u;
    float off_a = (float)(rng_next(&s) % 1000u) / 1000.0f;
    float off_b = (float)(rng_next(&s) % 1000u) / 1000.0f;
    float off_t = (float)(rng_next(&s) % 1000u) / 1000.0f;

    for (int i = 0; i < BIAS_POOL; i++) {
        BiasRecord a = { spread_q(i, off_a), rng_range(&s, -4, 4), false };
        BiasRecord b = { spread_q(i, off_b), rng_range(&s, -4, 4), false };
        a.hist_label = a.q + a.noise >= APPROVAL_CUTOFF;
        b.hist_label = b.q + b.noise - GROUP_B_PENALTY >= APPROVAL_CUTOFF;   /* viés histórico */
        g->pool_a[i] = a;
        g->pool_b[i] = b;
    }
    /* Teste: os dois grupos têm EXATAMENTE as mesmas qualificações, então qualquer
     * diferença de aprovação vem do modelo, não das pessoas. */
    for (int i = 0; i < BIAS_TEST; i++) {
        BiasRecord t = { spread_q(i, off_t), rng_range(&s, -4, 4), false };
        t.hist_label = correct_label(&t);
        g->test_a[i] = t;
        g->test_b[i] = t;
    }

    /* Configuração inicial: o problema aparece de cara (grupo B sub-representado). */
    g->n_a = 30;
    g->n_b = 6;
    g->use_group = true;
    g->use_proxy = false;
    g->fix_labels = false;
}

static bool train_label(const BiasGame *g, const BiasRecord *r) {
    return g->fix_labels ? correct_label(r) : r->hist_label;
}

bool bias_label(const BiasGame *g, bool group_b, int idx) {
    if (idx < 0 || idx >= BIAS_POOL) return false;
    return train_label(g, group_b ? &g->pool_b[idx] : &g->pool_a[idx]);
}

float bias_train_rate(const BiasGame *g, bool group_b) {
    int n = group_b ? g->n_b : g->n_a;
    const BiasRecord *pool = group_b ? g->pool_b : g->pool_a;
    if (n <= 0) return 0.0f;                     /* grupo ausente: o modelo nada sabe dele */
    int ok = 0;
    for (int i = 0; i < n; i++) if (train_label(g, &pool[i])) ok++;
    return 100.0f * (float)ok / (float)n;
}

void bias_run(BiasGame *g) {
    BiasResult *r = &g->result;
    memset(r, 0, sizeof *r);
    r->ran = true;

    r->train_rate_a = bias_train_rate(g, false);
    r->train_rate_b = bias_train_rate(g, true);
    int total = g->n_a + g->n_b;
    float overall = 50.0f;
    if (total > 0) {
        int ok = 0;
        for (int i = 0; i < g->n_a; i++) if (train_label(g, &g->pool_a[i])) ok++;
        for (int i = 0; i < g->n_b; i++) if (train_label(g, &g->pool_b[i])) ok++;
        overall = 100.0f * (float)ok / (float)total;
    }

    /* Peso que o modelo dá ao grupo: quanto mais a taxa do grupo foge da média, maior. */
    float strength = (g->use_group ? 1.0f : 0.0f) + (g->use_proxy ? PROXY_STRENGTH : 0.0f);
    if (strength > 1.0f) strength = 1.0f;
    float w_a = (r->train_rate_a - overall) * WEIGHT_GAIN * strength;
    float w_b = (r->train_rate_b - overall) * WEIGHT_GAIN * strength;

    int appr_a = 0, appr_b = 0, hits = 0;
    for (int i = 0; i < BIAS_TEST; i++) {
        const BiasRecord *ta = &g->test_a[i];
        const BiasRecord *tb = &g->test_b[i];
        bool pa = (float)ta->q + w_a >= (float)APPROVAL_CUTOFF;
        bool pb = (float)tb->q + w_b >= (float)APPROVAL_CUTOFF;
        if (pa) appr_a++;
        if (pb) appr_b++;
        if (pa == correct_label(ta)) hits++;
        if (pb == correct_label(tb)) hits++;
    }
    r->rate_a = 100.0f * (float)appr_a / BIAS_TEST;
    r->rate_b = 100.0f * (float)appr_b / BIAS_TEST;
    r->disparity = fabsf(r->rate_a - r->rate_b);
    r->accuracy = 100.0f * (float)hits / (2.0f * BIAS_TEST);
    r->solved = r->disparity <= BIAS_MAX_DISPARITY && r->accuracy >= BIAS_MIN_ACCURACY;
}
