"""Dump the client's tile graphics as PNG sheets, one per 16-colour palette.

    python tools/gfx.py GZLJ01            # -> build/GZLJ01/gfx/{obj,bg}_pal<N>.png
    python tools/gfx.py GZLJ01 --scale 3
    python tools/gfx.py GZLJ01 --sprites  # also build/GZLJ01/gfx/sprites/<name>_<frame>.png

Reads build/<ver>/prog.bin (run build.py first). OBJ tiles are gMode1ObjTiles (VRAM
0x06010000, palette gMode1ObjPalette at 0x05000200), BG tiles gMode1BgTiles (0x06008000,
gMode1BgPalette at 0x05000000); both LZ77, 4bpp, 32 tiles per row here. Tile N in the
sheet is OAM/BG tile number N. Colour 0 is drawn as transparent.

--sprites assembles each sprite (a struct SpriteFrame list: the data symbols after
gMode1ObjPalette, and the entries of gSpriteTable/gMapIconSprites) frame by frame from its struct SpritePart OAM
entries, with each part's own palette. OBJ mapping is 2D (DISPCNT 0x1100), so a part is a
rectangle of the 32-tile-wide sheet.
"""
import argparse
import re

from PIL import Image

import lz77
from common import BUILD, CONFIG, versions

SHEETS = [('obj', 'gMode1ObjTiles', 'gMode1ObjPalette'),
          ('bg', 'gMode1BgTiles', 'gMode1BgPalette')]
PER_ROW = 32


def symbol(ver, name):
    m = re.search(rf'^{name} = (0x[0-9A-Fa-f]+);', (CONFIG / ver / 'symbols.txt').read_text(), re.M)
    return int(m.group(1), 16)


def palettes(prog, off):
    out = []
    for p in range(16):
        pal = []
        for i in range(16):
            c = int.from_bytes(prog[off + (p * 16 + i) * 2:][:2], 'little')
            r, g, b = (c & 31) << 3, (c >> 5 & 31) << 3, (c >> 10 & 31) << 3
            pal.append((r, g, b, 0 if i == 0 else 255))
        out.append(pal)
    return out


def render(tiles, pal, scale):
    n = len(tiles) // 32
    rows = (n + PER_ROW - 1) // PER_ROW
    img = Image.new('RGBA', (PER_ROW * 8, rows * 8))
    px = img.load()
    for t in range(n):
        tx, ty = t % PER_ROW * 8, t // PER_ROW * 8
        for i in range(64):
            byte = tiles[t * 32 + i // 2]
            px[tx + i % 8, ty + i // 8] = pal[byte >> 4 if i & 1 else byte & 15]
    return img.resize((img.width * scale, img.height * scale), Image.NEAREST)


# OAM shape x size -> (width, height) in pixels.
OBJ_SIZES = [[(8, 8), (16, 16), (32, 32), (64, 64)],
             [(16, 8), (32, 8), (32, 16), (64, 32)],
             [(8, 16), (8, 32), (16, 32), (32, 64)]]


def data_symbols(ver):
    return {m.group(1): int(m.group(2), 16) for m in re.finditer(
        r'^(\w+) = (0x[0-9A-Fa-f]+); // data', (CONFIG / ver / 'symbols.txt').read_text(), re.M)}


def sprite_list(ver, prog, base):
    """{address: name} of every sprite: the data symbols between the OBJ palette and the
    sound bank (the sprite block), plus the targets of the sprite tables."""
    syms = data_symbols(ver)
    starts = sorted(set(syms.values()))
    word = lambda a: int.from_bytes(prog[a - base:][:4], 'little')
    lo, hi = syms['gMode1ObjPalette'] + 0x200, syms['gSoundBankHeader']
    sprites = {a: n for n, a in syms.items() if lo <= a < hi}
    by_addr = {a: n for n, a in syms.items()}
    for table in ('gSpriteTable', 'gMapIconSprites'):
        a = syms[table]
        end = next(s for s in starts if s > a)
        for i, p in enumerate(range(a, end, 4)):
            w = word(p)
            if base <= w < base + len(prog):
                sprites.setdefault(w, by_addr.get(w, f'{table}_{i}'))
    return sprites, starts


def parts_at(prog, base, addr):
    """The struct SpritePart list at addr, or None if it doesn't look like one."""
    parts = []
    o = addr - base
    while 0 <= o and o + 8 <= len(prog):
        y, a1, x, a3 = prog[o:o + 4]
        a4 = int.from_bytes(prog[o + 4:o + 6], 'little')
        if int.from_bytes(prog[o + 6:o + 8], 'little') == 0xFFFF:
            return parts
        shape, size = a1 >> 6, a3 >> 6
        if shape == 3 or len(parts) > 64:
            return None
        x = x | (a3 & 1) << 8
        parts.append({'x': x - 512 if x >= 256 else x, 'y': y - 256 if y >= 128 else y,
                      'w': OBJ_SIZES[shape][size][0], 'h': OBJ_SIZES[shape][size][1],
                      'hflip': a3 >> 4 & 1, 'vflip': a3 >> 5 & 1,
                      'tile': a4 & 0x3FF, 'pal': a4 >> 12})
        o += 8
    return None


def sprite_frames(prog, base, addr, stop):
    frames = []
    while addr + 8 <= stop:
        ptr = int.from_bytes(prog[addr - base:][:4], 'little')
        if not base <= ptr < base + len(prog):
            break
        parts = parts_at(prog, base, ptr)
        if parts is None:
            break
        frames.append(parts)
        addr += 8
    return frames


def render_frame(tiles, pals, parts, scale):
    if not parts:
        return None
    x0 = min(p['x'] for p in parts)
    y0 = min(p['y'] for p in parts)
    w = max(p['x'] + p['w'] for p in parts) - x0
    h = max(p['y'] + p['h'] for p in parts) - y0
    img = Image.new('RGBA', (w, h))
    px = img.load()
    # Earlier OAM entries are drawn on top: paint in reverse.
    for p in reversed(parts):
        pal = pals[p['pal']]
        for yy in range(p['h']):
            for xx in range(p['w']):
                sx = p['w'] - 1 - xx if p['hflip'] else xx
                sy = p['h'] - 1 - yy if p['vflip'] else yy
                t = p['tile'] + sy // 8 * PER_ROW + sx // 8
                off = t * 32 + (sy % 8) * 4 + (sx % 8) // 2
                if off >= len(tiles):
                    continue
                c = tiles[off] >> 4 if sx & 1 else tiles[off] & 15
                if c:
                    px[p['x'] - x0 + xx, p['y'] - y0 + yy] = pal[c]
    return img.resize((w * scale, h * scale), Image.NEAREST)


def dump_sprites(ver, prog, base, out, scale):
    tiles, _ = lz77.decompress(prog, symbol(ver, 'gMode1ObjTiles') - base)
    pals = palettes(prog, symbol(ver, 'gMode1ObjPalette') - base)
    sprites, starts = sprite_list(ver, prog, base)
    sdir = out / 'sprites'
    sdir.mkdir(exist_ok=True)
    for old in sdir.glob('*.png'):
        old.unlink()
    count = 0
    for addr, name in sorted(sprites.items()):
        stop = next((s for s in starts if s > addr), base + len(prog))
        for f, parts in enumerate(sprite_frames(prog, base, addr, stop)):
            img = render_frame(tiles, pals, parts, scale)
            if img:
                img.save(sdir / f'{name}_{f}.png')
                count += 1
    print(f'sprites: {len(sprites)} sprites, {count} frames -> {sdir}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('--scale', type=int, default=2)
    ap.add_argument('--sprites', action='store_true')
    args = ap.parse_args()
    ver = args.version
    prog = (BUILD / ver / 'prog.bin').read_bytes()
    base = versions()[ver]['prog_base']
    out = BUILD / ver / 'gfx'
    out.mkdir(exist_ok=True)
    for kind, tiles_sym, pal_sym in SHEETS:
        tiles, _ = lz77.decompress(prog, symbol(ver, tiles_sym) - base)
        for p, pal in enumerate(palettes(prog, symbol(ver, pal_sym) - base)):
            render(tiles, pal, args.scale).save(out / f'{kind}_pal{p}.png')
        print(f'{kind}: {len(tiles) // 32} tiles -> {out}/{kind}_pal0..15.png')
    if args.sprites:
        dump_sprites(ver, prog, base, out, args.scale)


if __name__ == '__main__':
    main()
