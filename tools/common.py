"""Shared paths and version/layout helpers."""
import hashlib
import struct
from pathlib import Path

import yaml

import lz77

ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / 'orig'
BUILD = ROOT / 'build'
CONFIG = ROOT / 'config'

EWRAM = 0x02000000
LZ_OFFSET = 0x1A4          # packer: LZ77 stream right after the stub's literal pool
STUB_POOL = 0x188          # packer stub literal pool (7 words)


def sha1(data):
    return hashlib.sha1(data).hexdigest()


def versions():
    with open(CONFIG / 'versions.yml') as f:
        return yaml.safe_load(f)


def version_defines(ver):
    """cpp flags for a version: VERSION_<ver>, VERSION_<base> for each `base` it builds on,
    the `defines` listed for it (and its bases), and PROG_BASE."""
    cfg = versions()
    defs = [f'-DPROG_BASE={cfg[ver]["prog_base"]:#010x}']
    while ver:
        defs.append(f'-DVERSION_{ver}')
        defs += [f'-D{d}' for d in cfg[ver].get('defines', [])]
        ver = cfg[ver].get('base')
    return ' '.join(defs)


def orig_path(ver):
    cfg = versions()[ver]
    return ORIG / ver / Path(cfg['disc_path']).name


def split_client(data):
    """Split a packed client into its parts, reading the layout from the stub's pool."""
    (io_base, _, tail_src, tail_dest, tail_end, lz_src, entry) = struct.unpack_from(
        '<7I', data, STUB_POOL)
    assert io_base == 0x04000130 and lz_src - EWRAM == LZ_OFFSET, 'unexpected packer stub'
    prog, lz_end = lz77.decompress(data, LZ_OFFSET)
    tail_off = tail_src - EWRAM
    assert tail_end - EWRAM == len(data)
    assert all(b == 0 for b in data[lz_end:tail_off]), 'nonzero padding after LZ stream'
    return {
        'stub': data[:LZ_OFFSET],
        'prog': prog,
        'prog_base': entry,
        'lz_len': lz_end - LZ_OFFSET,
        'tail': data[tail_off:],
        'tail_dest': tail_dest,
    }
