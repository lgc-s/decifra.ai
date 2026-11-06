#include "render/gfx.h"

#include <string.h>

unsigned gfx_hash(int a, int b) {
    unsigned h = (unsigned)a * 374761393u + (unsigned)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void gfx_panel(int x, int y, int w, int h, Color fill, Color border) {
    DrawRectangle(x + 1, y, w - 2, h, fill);
    DrawRectangle(x, y + 1, w, h - 2, fill);
    DrawRectangle(x + 1, y, w - 2, 1, border);
    DrawRectangle(x + 1, y + h - 1, w - 2, 1, border);
    DrawRectangle(x, y + 1, 1, h - 2, border);
    DrawRectangle(x + w - 1, y + 1, 1, h - 2, border);
}

void gfx_text_shadow(const char *t, int x, int y, int size, Color c) {
    DrawText(t, x + 1, y + 1, size, (Color){ 0, 0, 0, 170 });
    DrawText(t, x, y, size, c);
}

void gfx_text_center(const char *t, int cx, int y, int size, Color c) {
    gfx_text_shadow(t, cx - MeasureText(t, size) / 2, y, size, c);
}

void gfx_text_right(const char *t, int rx, int y, int size, Color c) {
    gfx_text_shadow(t, rx - MeasureText(t, size), y, size, c);
}

/* Evita cortar um caractere UTF-8 no meio. */
static int utf8_safe_len(const char *s, int n) {
    int len = (int)strlen(s);
    if (n >= len) return len;
    while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) n--;
    return n;
}

static int wrap_impl(const char *text, int x, int y, int maxw, int size, Color c, int max_bytes, bool draw) {
    enum { LINE_MAX = 255 };
    char line[LINE_MAX + 1] = "";
    int llen = 0, consumed = 0;
    const int lh = size + 3;
    const char *p = text;

#define FLUSH_LINE() do {                                                        \
        if (draw) {                                                              \
            int n = llen;                                                        \
            if (max_bytes >= 0) {                                                \
                int rest = max_bytes - consumed;                                 \
                if (rest < n) n = rest < 0 ? 0 : rest;                           \
            }                                                                    \
            n = utf8_safe_len(line, n);                                          \
            if (n > 0) {                                                         \
                char tmp[LINE_MAX + 1];                                          \
                memcpy(tmp, line, (size_t)n); tmp[n] = '\0';                     \
                gfx_text_shadow(tmp, x, y, size, c);                             \
            }                                                                    \
        }                                                                        \
        consumed += llen + 1;                                                    \
        y += lh;                                                                 \
    } while (0)

    while (*p) {
        const char *w = p;
        while (*p && *p != ' ') p++;
        int wl = (int)(p - w);
        if (wl > LINE_MAX) wl = LINE_MAX;
        char cand[LINE_MAX + 1];
        int clen = 0;
        if (llen > 0) {
            if (llen + 1 + wl > LINE_MAX) clen = -1;     /* não cabe no buffer: quebra */
            else { memcpy(cand, line, (size_t)llen); cand[llen] = ' '; memcpy(cand + llen + 1, w, (size_t)wl); clen = llen + 1 + wl; }
        } else {
            memcpy(cand, w, (size_t)wl); clen = wl;
        }
        if (clen >= 0) cand[clen] = '\0';
        if (llen > 0 && (clen < 0 || MeasureText(cand, size) > maxw)) {
            FLUSH_LINE();
            memcpy(line, w, (size_t)wl); llen = wl; line[llen] = '\0';
        } else {
            memcpy(line, cand, (size_t)clen); llen = clen; line[llen] = '\0';
        }
        while (*p == ' ') p++;
    }
    if (llen > 0) FLUSH_LINE();
#undef FLUSH_LINE
    return y;
}

int gfx_text_wrap(const char *text, int x, int y, int maxw, int size, Color c, int max_bytes) {
    return wrap_impl(text, x, y, maxw, size, c, max_bytes, true);
}

int gfx_text_wrap_height(const char *text, int maxw, int size) {
    return wrap_impl(text, 0, 0, maxw, size, WHITE, -1, false);
}

void gfx_bar(int x, int y, int w, int h, float frac, Color fill, Color back) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    DrawRectangle(x, y, w, h, back);
    DrawRectangle(x, y, (int)((float)w * frac), h, fill);
}
