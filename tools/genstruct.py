"""Regenerate a struct body in a header from a compact field list.

    python tools/genstruct.py include/game.h

A header opts in with a block like:

    // genstruct GameState 0x240
    // 0x000 struct GbaComFlags flags  // comment
    // 0x03A u16 switchMsg
    // 0x1E5 bits u8 a:1 b:1 c:6       // several bitfields in one byte
    // end genstruct
    struct GameState
    {
        ...
    };

and the struct that follows the block is rewritten with filler arrays between fields.
Sizes: u8/s8 1, u16/s16 2, u32/s32/pointers 4, `name[N]` arrays, `struct X` needs `size=N`
after the name (e.g. `0x000 struct GbaComFlags flags size=12`).

A version's extra fields go in a block that inserts `size` bytes at its first field's offset;
the block's fields use that version's offsets, everything outside it the base layout's (so
fields after the block sit `size` bytes higher in that version):

    // #ifdef VERSION_PAL size=8
    // 0x138 s16 pendingMsg
    // #endif
"""
import re
import sys

SIZES = {'u8': 1, 's8': 1, 'bool8': 1, 'u16': 2, 's16': 2, 'u32': 4, 's32': 4, 'bool32': 4}


def parse(lines):
    fields = []
    block = None
    for line in lines:
        if line.startswith('#if'):
            cond, _, size = line.rpartition(' size=')
            block = (cond, int(size, 0), [])
            continue
        if line.startswith('#endif'):
            cond, size, spec = block
            inner = parse(spec)
            start = min(f[0] for f in inner)
            fields.append((start, 0, ('block', cond, size, inner), ''))
            block = None
            continue
        if block is not None:
            block[2].append(line)
            continue
        body, _, comment = line.partition('//')
        comment = comment.strip()
        parts = body.split()
        off = int(parts[0], 16)
        if parts[1] == 'bits':
            typ = parts[2]
            bits = parts[3:]
            fields.append((off, SIZES[typ], [f'{typ} {b};' for b in bits], comment))
            continue
        size = None
        rest = parts[1:]
        if rest[-1].startswith('size='):
            size = int(rest[-1][5:], 0)
            rest = rest[:-1]
        typ = ' '.join(rest[:-1])
        name = rest[-1]
        m = re.match(r'(\w+)((?:\[[^\]]+\])*)$', name)
        count = 1
        for dim in re.findall(r'\[([^\]]+)\]', m.group(2)):
            count *= int(dim, 0)
        if size is None:
            size = (4 if typ.endswith('*') else SIZES[typ]) * count
        fields.append((off, size, [f'{typ} {name};'], comment))
    return fields


def render(fields, total, start=0):
    out = []
    pos = start
    # a block sorts before the base field at its offset
    for off, size, decls, comment in sorted(fields, key=lambda f: (f[0], f[1] != 0)):
        if off < pos:
            sys.exit(f'field at {off:#x} overlaps the previous one (ends {pos:#x})')
        if off > pos:
            out.append(f'    /*0x{pos:03X}*/ u8 filler{pos:03X}[0x{off:03X} - 0x{pos:03X}];')
            pos = off
        if isinstance(decls, tuple):
            _, cond, bsize, inner = decls
            out.append(cond)
            out += render(inner, off + bsize, off)
            out.append('#endif')
            continue
        for i, d in enumerate(decls):
            prefix = f'/*0x{off:03X}*/ ' if i == 0 else ' ' * 10
            line = f'    {prefix}{d}'
            if comment and i == 0:
                line = f'{line:<46}// {comment}'
            out.append(line.rstrip())
        pos = off + size
    if pos < total:
        out.append(f'    /*0x{pos:03X}*/ u8 filler{pos:03X}[0x{total:03X} - 0x{pos:03X}];')
    return out


def main(path):
    text = open(path, encoding='utf-8').read()
    lines = text.split('\n')
    i = 0
    result = []
    while i < len(lines):
        m = re.match(r'// genstruct (\w+) (0x[0-9A-Fa-f]+)$', lines[i])
        if not m:
            result.append(lines[i])
            i += 1
            continue
        name, total = m.group(1), int(m.group(2), 16)
        spec = []
        result.append(lines[i])
        i += 1
        while lines[i] != '// end genstruct':
            result.append(lines[i])
            spec.append(lines[i][3:])
            i += 1
        result.append(lines[i])
        i += 1
        if lines[i] != f'struct {name}' or lines[i + 1] != '{':
            sys.exit(f'expected "struct {name}" after the genstruct block')
        result += [lines[i], lines[i + 1]]
        i += 2
        while lines[i] != '};':
            i += 1
        result += render(parse(spec), total)
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(result))


if __name__ == '__main__':
    main(sys.argv[1])
