#!/usr/bin/env python3
"""Gera os mapas ASCII dos 4 atos em levels/actN/actN.txt.

Os mapas são construídos por segmentos (chão, degraus, poços, escaladas...) e o
gerador valida os limites de alcance do jogador (pulo <= 5 tiles de vão, dash <= 12,
escalada <= 10 tiles). Para editar uma fase, ajuste os segmentos abaixo e rode:

    python3 tools/gen_levels.py

Também é possível editar os .txt diretamente (formato descrito em src/world/level.h).
"""
import os

H = 36            # altura em tiles
GROUND = 28       # altura padrão do chão (linha da superfície)
MAX_W = 256


class Builder:
    def __init__(self, palette):
        self.g = GROUND
        self.x = 1
        self.cells = {}
        self.seg_x = 1
        self.seg_g = GROUND
        self.zones = []
        self.palette = palette
        self.stars = 0

    # --- primitivas ---
    def put(self, x, y, c):
        assert 0 <= y < H and 0 <= x < MAX_W, (x, y, c)
        self.cells[(x, y)] = c

    def _fill(self, x0, x1, top):
        for x in range(x0, x1):
            for y in range(top, H):
                self.put(x, y, '#')

    # --- segmentos ---
    def zone(self, pal):
        self.zones.append((self.x, pal))

    def flat(self, w):
        self.seg_x, self.seg_g = self.x, self.g
        self._fill(self.x, self.x + w, self.g)
        self.x += w

    def rise(self, h, w):
        assert 1 <= h <= 10, "escalada maxima: 10 tiles"
        self.g -= h
        self.flat(w)

    def drop(self, h, w):
        assert self.g + h <= H - 2
        self.g += h
        self.flat(w)

    def pit(self, w, dash=False):
        assert w <= (12 if dash else 5), f"vao de {w} tiles exige dash/cristais"
        self.seg_x, self.seg_g = self.x, self.g
        self.x += w

    def crumble(self, n, gap=3):
        """n plataformas frageis de 2 tiles, separadas por 'gap' tiles."""
        assert gap <= 5
        self.seg_x, self.seg_g = self.x, self.g
        x = self.x + gap
        for _ in range(n):
            for dx in range(2):
                self.put(x + dx, self.g, '~')
            x += 2 + gap
        self.x = x

    # --- decoração e entidades (relativas ao último segmento) ---
    def ent(self, ch, off, up=0):
        if ch == '*':
            self.stars += 1
        self.put(self.seg_x + off, self.seg_g - 1 - up, ch)

    def spikes(self, offs):
        for o in offs:
            self.put(self.seg_x + o, self.seg_g - 1, '^')

    def ghost(self, off, w, up):
        for dx in range(w):
            self.put(self.seg_x + off + dx, self.seg_g - up, 'g')

    # --- saída ---
    def render(self, name, act, ints):
        assert self.stars == 3, f"ato {act}: precisa de exatamente 3 colecionaveis (tem {self.stars})"
        width = self.x + 3
        assert width <= MAX_W, f"mapa largo demais: {width}"
        out = [f"@name {name}", f"@act {act}", f"@palette {self.palette}"]
        zs = self.zones or [(0, self.palette)]
        for i, (zx, zp) in enumerate(zs):
            out.append(f"@zone {0 if i == 0 else zx} {zp}")
        for d, (kind, i) in sorted(ints.items()):
            out.append(f"@int {d} {kind} {i}")
        out.append("@map")
        for y in range(H):
            out.append(''.join(self.cells.get((x, y), '.') for x in range(width)))
        return '\n'.join(out) + '\n'


def act1():
    b = Builder(0)
    b.flat(18); b.ent('P', 3); b.ent('1', 10); b.ent('2', 15)
    b.rise(1, 10); b.ent('3', 5); b.ent('*', 3, up=2)
    b.rise(1, 8); b.ent('4', 2); b.ent('C', 5)
    b.pit(5)
    b.flat(14); b.spikes([7, 8]); b.ent('5', 12)
    b.pit(8, dash=True); b.ent('*', 4, up=2)
    b.flat(10); b.ent('C', 2)
    b.flat(8); b.ent('6', 5)
    b.rise(10, 14); b.ent('*', 6, up=1)
    b.drop(10, 18); b.ent('8', 3); b.ent('7', 8); b.ent('E', 14)
    ints = {'1': ('dialogue', 0), '2': ('dialogue', 1), '3': ('dialogue', 2),
            '4': ('dialogue', 3), '5': ('dialogue', 4), '6': ('dialogue', 5),
            '7': ('quiz', 0), '8': ('dialogue', 6)}
    return b.render("Departamento de Alfabetização Algorítmica", 1, ints)


def act2():
    b = Builder(1)
    # Trânsito
    b.zone(1)
    b.flat(14); b.ent('P', 3); b.ent('1', 9)
    b.pit(5)
    b.flat(10); b.ent('C', 2); b.spikes([6, 7]); b.ent('*', 4, up=3)
    b.rise(2, 6); b.rise(2, 6); b.rise(2, 6)
    b.drop(6, 12); b.ent('2', 5)
    # Crédito
    b.zone(2)
    b.flat(8); b.ent('3', 2); b.ent('C', 6)
    b.crumble(5, 3)
    b.flat(10); b.ent('4', 4); b.ent('*', 7, up=2)
    b.rise(3, 6); b.rise(3, 6)
    b.pit(8, dash=True)
    b.flat(8); b.ent('C', 2)
    # Notícias
    b.zone(3)
    b.flat(6); b.ent('5', 2)
    b.pit(5); b.ghost(1, 3, up=2)
    b.flat(8); b.ent('C', 2)
    b.rise(10, 10); b.ent('6', 3); b.ent('*', 6, up=1)
    b.drop(10, 8)
    # Saúde
    b.zone(4)
    b.drop(6, 10); b.ent('C', 2); b.ent('7', 6)
    b.flat(16); b.spikes([4, 5, 10, 11])
    b.pit(12, dash=True); b.ent('o', 4, up=3); b.ent('o', 8, up=3)
    b.flat(14); b.ent('C', 1); b.ent('8', 4); b.ent('9', 8); b.ent('E', 12)
    ints = {'1': ('dialogue', 7), '2': ('call', 0), '3': ('dialogue', 8), '4': ('call', 1),
            '5': ('dialogue', 9), '6': ('call', 2), '7': ('dialogue', 10),
            '8': ('call', 3), '9': ('quiz', 1)}
    return b.render("Distrito NÓVA", 2, ints)


def act3():
    b = Builder(5)
    b.flat(10); b.ent('P', 3); b.ent('1', 7)
    b.rise(2, 6)
    b.crumble(4, 3)
    b.flat(8); b.ent('2', 3); b.ent('*', 6, up=2)
    b.pit(8, dash=True)
    b.flat(6); b.ent('C', 2)
    b.rise(10, 10); b.ent('3', 4); b.ent('*', 7, up=1)
    b.crumble(6, 3)
    b.drop(10, 8); b.ent('4', 3); b.ent('C', 5)
    b.flat(16); b.spikes([3, 4, 8, 9, 13, 14])
    b.pit(12, dash=True); b.ent('o', 4, up=3); b.ent('o', 8, up=3); b.ent('*', 6, up=3)
    b.flat(8); b.ent('C', 1); b.ent('5', 4)
    b.flat(10); b.ent('6', 2); b.ent('7', 5); b.ent('E', 8)
    ints = {'1': ('dialogue', 14), '2': ('dialogue', 11), '3': ('dialogue', 12),
            '4': ('dialogue', 13), '5': ('call', 4), '6': ('bias', 0), '7': ('quiz', 2)}
    return b.render("Arquivo Sul", 3, ints)


def act4():
    b = Builder(6)
    b.flat(10); b.ent('P', 3); b.ent('1', 7)
    b.rise(2, 6)
    b.crumble(4, 3)
    b.flat(8); b.ent('*', 4, up=2)
    b.pit(8, dash=True); b.ent('*', 4, up=2)
    b.flat(6); b.ent('C', 2)
    b.rise(10, 10); b.ent('2', 3); b.ent('*', 6, up=1)
    b.drop(10, 8); b.ent('C', 2); b.ent('3', 5)
    b.flat(16); b.spikes([3, 4, 9, 10])
    b.pit(12, dash=True); b.ent('o', 4, up=3); b.ent('o', 8, up=3)
    b.flat(14); b.ent('C', 1); b.ent('4', 4); b.ent('5', 8); b.ent('E', 12)
    ints = {'1': ('dialogue', 15), '2': ('call', 5), '3': ('call', 6),
            '4': ('quiz', 3), '5': ('dialogue', 16)}
    return b.render("Salão da Cidade", 4, ints)


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'levels')
    for n, fn in enumerate((act1, act2, act3, act4), start=1):
        d = os.path.join(root, f"act{n}")
        os.makedirs(d, exist_ok=True)
        path = os.path.join(d, f"act{n}.txt")
        with open(path, 'w', encoding='utf-8') as f:
            f.write(fn())
        print("gerado", os.path.relpath(path))


if __name__ == '__main__':
    main()
