"""GBA BIOS LZ77 (type 0x10) codec, byte-matching Nintendo's encoder.

The original encoder (verified against every LZ stream in res/Gba/) is plain greedy:
at each position take the longest match (3..18 bytes) in the previous 4096 bytes,
preferring the nearest distance on ties. The client program uses distance 1 (fine for
LZ77UnCompWram); msg_LZ.bin was packed VRAM-safe (min_dist=2, as LZ77UnCompVram needs
because it writes 16 bits at a time), then zero-padded to 4 bytes.
"""
import struct

WINDOW = 0x1000
MIN_LEN = 3
MAX_LEN = 18


def decompress(src, offset=0):
    """Return (data, end_offset) for the LZ77 stream starting at `offset`."""
    header = struct.unpack_from('<I', src, offset)[0]
    if header & 0xFF != 0x10:
        raise ValueError(f'not an LZ77 stream (header {header:#x})')
    size = header >> 8
    o = offset + 4
    out = bytearray()
    while len(out) < size:
        flags = src[o]
        o += 1
        for _ in range(8):
            if len(out) >= size:
                break
            if flags & 0x80:
                x = src[o] << 8 | src[o + 1]
                o += 2
                length = (x >> 12) + MIN_LEN
                dist = (x & 0xFFF) + 1
                for _ in range(length):
                    out.append(out[-dist])
            else:
                out.append(src[o])
                o += 1
            flags <<= 1
    return bytes(out), o


def compress(data, min_dist=1):
    """Compress `data`; output is the raw stream (not padded). min_dist=2 is VRAM-safe."""
    n = len(data)
    out = bytearray(struct.pack('<I', 0x10 | n << 8))
    chains = {}  # 3-byte key -> list of positions (ascending)
    pos = 0
    while pos < n:
        flag_at = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if pos >= n:
                break
            best_len, best_dist = 0, 0
            if pos + MIN_LEN <= n:
                cands = chains.get(data[pos:pos + MIN_LEN], ())
                limit = min(MAX_LEN, n - pos)
                for cand in reversed(cands):  # nearest first
                    dist = pos - cand
                    if dist > WINDOW:
                        break
                    if dist < min_dist:
                        continue
                    length = MIN_LEN
                    while length < limit and data[pos + length] == data[pos + length - dist]:
                        length += 1
                    if length > best_len:
                        best_len, best_dist = length, dist
                        if length == limit:
                            break
            step = best_len if best_len >= MIN_LEN else 1
            if best_len >= MIN_LEN:
                flags |= 0x80 >> bit
                x = (best_len - MIN_LEN) << 12 | (best_dist - 1)
                out += bytes((x >> 8, x & 0xFF))
            else:
                out.append(data[pos])
            for p in range(pos, pos + step):
                if p + MIN_LEN <= n:
                    chains.setdefault(data[p:p + MIN_LEN], []).append(p)
            pos += step
        out[flag_at] = flags
    return bytes(out)
