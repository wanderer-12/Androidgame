import math
import os
import wave

ROOT = os.path.join(os.path.dirname(__file__), '..', 'app', 'src', 'main', 'res', 'raw')

def write_wav(name, duration, fn):
    rate = 44100
    frames = bytearray()
    for i in range(int(rate * duration)):
        t = i / rate
        value = max(-1.0, min(1.0, fn(t, duration)))
        sample = int(value * 32767)
        frames += int(sample).to_bytes(2, 'little', signed=True)
    with wave.open(os.path.join(ROOT, name), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(frames)

def impact(t, d):
    click = math.sin(2 * math.pi * (1250 + 500 * math.exp(-t * 35)) * t) * math.exp(-t * 45)
    punch = math.sin(2 * math.pi * 105 * t) * math.exp(-t * 24)
    return 0.65 * click + 0.32 * punch

def phase(t, d):
    freq = 320 + 720 * min(1.0, t / d)
    tone = math.sin(2 * math.pi * freq * t) * math.exp(-t * 4.5)
    overtone = 0.3 * math.sin(2 * math.pi * freq * 2.0 * t) * math.exp(-t * 6.0)
    return 0.7 * tone + overtone

write_wav('impact.wav', 0.11, impact)
write_wav('boss_phase.wav', 0.32, phase)
