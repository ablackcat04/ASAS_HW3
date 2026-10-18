# Reference audio analysis

Source: `shepard's_tone_phenomenon (480p).ogg`. Decoded with FFmpeg and examined numerically, without speaker playback.

## Direct measurements

- The file has **audio only**: stereo Vorbis, 44.1 kHz, duration **266.303832 seconds**. The filename does not imply a video stream.
- There is narration/background noise around the demonstrations. Clear tonal passages include about **29–65 s**, **117–122 s**, **135–142 s**, **171–176 s**, **181–185 s**, and **227–253 s**.
- The synthesis lines are octave powers, not a conventional sawtooth or complete harmonic series. At **43.95 s**, the B-class tone has lines at about **247, 494, 988, 1976 Hz**. Measured amplitudes are **−12.38, −27.70, −23.59, −39.74 dBFS**. The 3rd and 5th harmonics (741 and 1235 Hz) measure **−66.41 and −68.04 dBFS**, more than 54 dB below the strongest line. Pure sine oscillators at octave intervals are sufficient for the organ-like character.
- The stable later passage repeatedly returns to the same absolute frequency bands while descending pitch classes. Most useful partials occupy roughly **80–2500 Hz**, with most energy around **175–1100 Hz**; low and high octave layers fade away.
- Examples of median octave levels from isolated frames during **36–65 s**, normalized within a note: C frequencies **130.8 / 261.6 / 523.3 / 1046.5 / 2093 Hz** have levels **−12.4 / 0 / −3.1 / −5.5 / −15.8 dB**. B frequencies **123.5 / 246.9 / 493.9 / 987.8 / 1975.5 Hz** have levels **−17.2 / 0 / −14.6 / −11.2 / −26.7 dB**.
- Frequencies are close to equal temperament at A=440, with some later notes a few cents sharp (e.g. C around 262 Hz). This does not establish deliberate detuning.
- The first portion, about **29–35 s**, starts with strong high-register energy (C 1046/2093/4186/8373 Hz around 29.3 s), then descends into the stable repeated spectral band. Its octave amplitudes are different from later repetitions of the same pitch class, so the entire clip cannot be described as one unchanged octave-invariant spectrum from the outset.

## Envelope and articulation estimates

A pooled fit of amplitude in dB to a quadratic in log2 frequency, allowing each pitch class its own normalization, gives a Gaussian amplitude envelope with **center 437.6 Hz** and **sigma 1.241 octaves**. Fit error is about **6.1 dB RMS**, which is too large to claim this is the exact original formula. Strong non-smooth spectral coloration (for example the subdued 494 Hz component beside strong 247 and 988 Hz components) may come from instrument filtering, speakers, microphone/room response, or the original synthesis. Those causes cannot be separated from this recording.

The first isolated C onset at **29.10 s** rises from 10% to 90% of steady amplitude in approximately **12 ms** after a 25 ms total analysis smoothing window. The actual synthesis attack is likely shorter. A B partial around **36.16–36.24 s** falls from 90% to 10% in about **70 ms**, with room/acoustic decay contributing to that result. The waveform holds during note sustain instead of decaying like a struck piano.

## Recommended recreation

Use a polyphonic organ-style sine stack at frequencies `f0 * 2^k` with a fixed **absolute log-frequency** envelope. Start with `w(f) = exp(-0.5 * (log2(f / 440) / 1.25)^2)`, include enough octave layers below and above the audible band, and normalize note gain. Build the oscillator bank from **MIDI pitch class** so MIDI notes separated by twelve semitones reproduce the same spectrum; a descending/ascending chromatic sequence then cycles continuously without announcing an octave boundary. A roughly **5–10 ms attack**, constant sustain, and **70–100 ms release** are reasonable click-free defaults. Use conservative output gain and polyphony limiting.

The recording supports octave spacing, sustained sine-based timbre, and fading edges strongly. The precise source envelope, its early high-register behavior, and actual physical MIDI key/octave assignments remain uncertain because the local file contains no video and has recording coloration. Avoid claiming sample-perfect matching.

## Reproducible artifacts

- `analyze_reference.py`: decoding-independent STFT peak extraction.
- `envelope_reference.py`: stable-note octave levels and log-frequency fit.
- `articulation_reference.py`: narrow-carrier amplitude traces.
- `spectrogram_28_65.png`: demonstration frequency trace.
- `peaks.json` and `octave_relative_levels.json`: measured numerical data.

Intermediate mono PCM and full spectra are local analysis caches and are not required by the plugin. They were removed after measurement to avoid adding roughly 181 MB to the project. To rerun, decode the source with `ffmpeg -i "../shepard's_tone_phenomenon (480p).ogg" -ac 1 -c:a pcm_s16le reference_mono.wav`, run `analyze_reference.py`, then run the other scripts from this folder. Cache files and numerical JSON are ignored by Git.
