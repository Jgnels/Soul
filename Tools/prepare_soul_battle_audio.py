"""Prepare bounded, spatial mono battle clips without launching Unreal.
Original recordings remain untouched. Generated WAVs are local licensed derivatives.
"""
import argparse
import array
import hashlib
import json
from pathlib import Path
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = Path(r"D:\SFX\Free Fantasy SFX Pack By TomMusic\Free Fantasy SFX Pack By TomMusic\WAV Files\SFX")
DEFAULT_OUTPUT = ROOT / "Data/BattleAudioLocal"
CLIPS = {
    "SwordSwing1": "Attacks/Sword Attacks Hits and Blocks/Sword Attack 1.wav",
    "SwordSwing2": "Attacks/Sword Attacks Hits and Blocks/Sword Attack 2.wav",
    "SwordHit1": "Attacks/Sword Attacks Hits and Blocks/Sword Impact Hit 1.wav",
    "SwordHit2": "Attacks/Sword Attacks Hits and Blocks/Sword Impact Hit 2.wav",
    "BowRelease1": "Attacks/Bow Attacks Hits and Blocks/Bow Attack 1.wav",
    "BowRelease2": "Attacks/Bow Attacks Hits and Blocks/Bow Attack 2.wav",
    "ArrowHit": "Attacks/Bow Attacks Hits and Blocks/Bow Impact Hit 1.wav",
    "Firebolt": "Spells/Fireball 1.wav",
    "Blizzard": "Spells/Ice Barrage 1.wav",
    "MagicImpact": "Spells/Spell Impact 1.wav",
    "TidalWard": "Spells/Wave Attack 1.wav",
}
def read_mono(source):
    with wave.open(str(source), "rb") as wav:
        channels, width, rate, frames = wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()
        if width != 2 or channels not in (1, 2) or rate not in (22050, 44100, 48000):
            raise ValueError("Expected mono/stereo 16-bit PCM at 22.05/44.1/48 kHz")
        if not 0 < frames <= rate * 8:
            raise ValueError("Battle one-shots must last more than zero and at most eight seconds")
        raw = wav.readframes(frames)
        if len(raw) != frames * channels * width:
            raise ValueError("Truncated PCM data")
    samples = array.array("h")
    samples.frombytes(raw)
    if sys.byteorder != "little":
        samples.byteswap()
    mono = list(samples) if channels == 1 else [round((samples[i] + samples[i+1]) / 2) for i in range(0, len(samples), 2)]
    peak = max(abs(v) for v in mono)
    if peak < 16:
        raise ValueError("Silent or phase-cancelled mono clip")
    # Attenuate hot clips only; do not amplify noise or normalize every sound equally.
    gain = min(1.0, 0.9 * 32767 / peak)
    result = array.array("h", [round(v * gain) for v in mono])
    if sys.byteorder != "little":
        result.byteswap()
    return result.tobytes(), rate, {"duration": frames / rate, "source_channels": channels, "peak_before": peak, "gain": gain}

def prepare(source_root=DEFAULT_SOURCE, output=DEFAULT_OUTPUT):
    source_root, output = Path(source_root).resolve(), Path(output).resolve()
    if output == source_root or source_root in output.parents or output in source_root.parents:
        raise ValueError("Output and licensed source trees must be separate")
    # Validate every input before writing any output.
    pending = []
    for name, relative in CLIPS.items():
        source = (source_root / relative).resolve()
        if source_root not in source.parents:
            raise ValueError("Source escapes licensed source root")
        pcm, rate, info = read_mono(source)
        pending.append((name, source, pcm, rate, info))
    output.mkdir(parents=True, exist_ok=True)
    receipt = []
    for name, source, pcm, rate, info in pending:
        target = output / (name + ".wav")
        with wave.open(str(target), "wb") as wav:
            wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(rate); wav.writeframes(pcm)
        receipt.append({"name": name, "source": str(source), "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                        "file": str(target), "sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
                        "package": "/Game/Soul/Audio/Battle/" + name, **info})
    (output / "manifest.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    clips = prepare(args.source, args.output)
    print(f"Prepared {len(clips)} mono clips; originals unchanged; Unreal import and listening qualification pending.")
