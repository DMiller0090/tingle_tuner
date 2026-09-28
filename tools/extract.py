"""Extract a Tingle Tuner client from a Wind Waker disc image and verify it.

    python tools/extract.py GZLJ01 path/to/TWW-JP.iso
    python tools/extract.py GZLE01 path/to/TWW-USA.iso     # also extracts GZLE01_ud
    python tools/extract.py GZLJ01_demo path/to/demo.iso     # the JP store demo
    python tools/extract.py GZLJ01 --file path/to/client.bin

Writes orig/<ver>/<client>.bin (gitignored) after checking its SHA-1, then unpacks it
into build/<ver>/{prog,tail}.bin for the disassembler.
"""
import argparse
import struct
import sys

from common import BUILD, ORIG, orig_path, sha1, split_client, versions


def read_disc_files(iso_path):
    """Map every file path in a GameCube disc's FST to its bytes (lazily)."""
    f = open(iso_path, 'rb')
    game_id = f.read(6).decode('ascii', 'replace')
    f.seek(0x424)
    fst_off, fst_size = struct.unpack('>II', f.read(8))
    f.seek(fst_off)
    fst = f.read(fst_size)
    count = struct.unpack_from('>I', fst, 8)[0]
    strtab = count * 12

    def name(o):
        end = fst.index(b'\0', strtab + o)
        return fst[strtab + o:end].decode('shift-jis', 'replace')

    entries = {}
    stack = [(count, '')]
    for i in range(1, count):
        while i >= stack[-1][0]:
            stack.pop()
        word, off, size = struct.unpack_from('>III', fst, i * 12)
        path = stack[-1][1] + '/' + name(word & 0xFFFFFF)
        if word >> 24:
            stack.append((size, path))
        else:
            entries[path] = (off, size)

    def read(path):
        off, size = entries[path]
        f.seek(off)
        return f.read(size)

    return game_id, entries, read


def unpack(ver, data):
    parts = split_client(data)
    cfg = versions()[ver]
    assert parts['prog_base'] == cfg['prog_base'], hex(parts['prog_base'])
    assert parts['tail_dest'] == cfg['tail_dest'], hex(parts['tail_dest'])
    out = BUILD / ver
    out.mkdir(parents=True, exist_ok=True)
    (out / 'prog.bin').write_bytes(parts['prog'])
    (out / 'tail.bin').write_bytes(parts['tail'])
    print(f'  prog {parts["prog_base"]:#010x} size {len(parts["prog"]):#x} '
          f'sha1 {sha1(parts["prog"])}')
    print(f'  tail {parts["tail_dest"]:#010x} size {len(parts["tail"]):#x}')


def install(ver, data):
    cfg = versions()[ver]
    if sha1(data) != cfg['sha1']:
        sys.exit(f'{ver}: SHA-1 mismatch ({sha1(data)}), expected {cfg["sha1"]} -- '
                 'use an unmodified retail disc')
    dest = orig_path(ver)
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_bytes(data)
    print(f'{ver}: {dest.relative_to(ORIG.parent)} OK')
    unpack(ver, data)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('iso', nargs='?', help='disc image (.iso/.gcm, uncompressed)')
    ap.add_argument('--file', help='use an already-extracted client .bin instead of a disc')
    args = ap.parse_args()

    if args.file:
        with open(args.file, 'rb') as f:
            install(args.version, f.read())
        return
    if not args.iso:
        # Re-unpack from orig/ only.
        unpack(args.version, orig_path(args.version).read_bytes())
        return

    game_id, entries, read = read_disc_files(args.iso)
    region = args.version.split('_')[0]
    if game_id != region:
        sys.exit(f'disc is {game_id}, expected {region}')
    # Several versions share a game ID (the JP retail disc and the JP demo): install the ones
    # whose file is on this disc with the right SHA-1.
    found = False
    for ver, cfg in versions().items():
        if ver.split('_')[0] == region and cfg['disc_path'] in entries:
            data = read(cfg['disc_path'])
            if sha1(data) == cfg['sha1']:
                install(ver, data)
                found = True
    if not found:
        sys.exit(f'no known client on this {game_id} disc -- use an unmodified disc')


if __name__ == '__main__':
    main()
