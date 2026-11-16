# Killing Part T7: trims the silence before each sound effect (the store files start 0.1-0.37 s late,
# which would put the snip behind the cut). Reads SourceArt/Audio/sfx/*.wav (left untouched), writes
# SourceArt/Audio/sfx_trimmed/*.wav: starts 5 ms before the first sample within 20 dB of the peak, with a
# 3 ms fade in. Plain Python (stdlib only): python Tools/trim_sfx.py
import os, sys, wave

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'SourceArt', 'Audio')
SRC = os.path.join(ROOT, 'sfx')
DST = os.path.join(ROOT, 'sfx_trimmed')
os.makedirs(DST, exist_ok=True)

def sample(raw, i, sw):
    b = raw[i:i + sw]
    if sw == 1:
        return b[0] - 128
    return int.from_bytes(b, 'little', signed=True)

def put(value, sw):
    if sw == 1:
        return bytes([value + 128])
    return int(value).to_bytes(sw, 'little', signed=True)

for name in sorted(os.listdir(SRC)):
    if not name.endswith('.wav'):
        continue
    w = wave.open(os.path.join(SRC, name), 'rb')
    params = w.getparams()
    ch, sw, rate, n = params.nchannels, params.sampwidth, params.framerate, params.nframes
    raw = w.readframes(n)
    w.close()
    frame = ch * sw
    peaks = [abs(sample(raw, f * frame, sw)) for f in range(n)]
    peak = max(peaks) or 1
    first = next((f for f, v in enumerate(peaks) if v >= peak * 0.1), 0)
    start = max(0, first - int(rate * 0.005))
    body = bytearray(raw[start * frame:])
    fade = int(rate * 0.003)
    for f in range(min(fade, n - start)):
        gain = f / float(fade)
        for c in range(ch):
            at = f * frame + c * sw
            body[at:at + sw] = put(int(sample(body, at, sw) * gain), sw)
    out = wave.open(os.path.join(DST, name), 'wb')
    out.setnchannels(ch)
    out.setsampwidth(sw)
    out.setframerate(rate)
    out.writeframes(bytes(body))
    out.close()
    print('%s: cut %.3f s, now %.2f s' % (name, start / float(rate), (n - start) / float(rate)))
