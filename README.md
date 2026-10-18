# ASAS Homework 3: Shepard Keyboard

A JUCE VST3 / standalone instrument for Activity 2. It recreates the steady organ-like Shepard keyboard in the supplied demonstration using additive sine synthesis. The editor contains only a keyboard and a volume slider. Nothing plays until a key or MIDI note is received.

## Play

- Load **ASAS Shepard Keyboard** as an instrument on a MIDI track, or open its standalone executable.
- Click the on-screen keyboard, connect a MIDI keyboard, or click a key to focus the editor and use **A W S E D F T G Y H U J K** for a chromatic octave.
- Play adjacent notes in an ascending or descending sequence. Brief gaps help the staircase illusion.
- Every MIDI octave repeats the same twelve sounds: C3, C4, C5, etc. have identical spectra. The on-screen keyboard spans MIDI 48–96.
- MIDI velocity controls level; sustain pedal is supported. Pitch bend is intentionally ignored.
- Volume defaults to **-24 dB** and is saved with the host session. Gain changes ramp over 20 ms. A soft output ceiling at **0.20 (-14 dBFS)** prevents dense chords reaching full scale.
- There are 32 voices. Excess simultaneous notes are ignored until a voice becomes free.

## How the illusion works

An ordinary organ repeats its harmonic pattern at progressively higher absolute frequencies as you move up the keys. Here only pitch class changes: all keys share one fixed amplitude envelope over **absolute log frequency**. Octave layers near the middle stay strong while the lowest and highest layers fade. Once the pitch class wraps after twelve semitones, the same bank returns, allowing the pitch steps to appear to continue rising or falling.

For MIDI note m, let p = m mod 12 and

```text
f0(p) = 16.3515978313 * 2^(p/12) Hz
fk(p) = f0(p) * 2^k,              k = 0,...,9
w(f)  = exp(-0.5 * [log2(f/440)/1.25]^2)
ak    = velocity * w(fk) * nyquistFade(fk)
        / sum_j [w(fj) * nyquistFade(fj)]

x(t)  = masterGain * noteEnvelope(t) * sum_k ak * sin(2*pi*fk*t)
```

The fixed Gaussian has a center of 440 Hz and sigma of 1.25 octaves. The note envelope has an 8 ms attack, constant sustain and 100 ms release. The highest ranks fade between 0.40 and 0.45 times the sample rate, before Nyquist. Double-precision quadrature oscillators render the sine bank without per-sample trigonometry or audio-thread allocation in the voice.

The supplied OGG is **audio only**, despite its filename. Analysis supports octave-only sine ranks, a broad fixed envelope in the stable section, and short attack/release. The first 29–35 seconds changes register before reaching the repeating band. Room/instrument coloration prevents identifying the exact original synthesis patch. This recreation targets the stable illusion rather than that introductory register change.

Measured frequencies, levels, timestamps and fit uncertainty are in [the analysis report](analysis/reference_analysis.md). The fixed-envelope mechanism is also explained in [UBC's Shepard tone demonstration](https://www.cs.ubc.ca/labs/imager/contributions/flinn/Illusions/ST/st.html), based on [Shepard's original paper](https://www.brainmusic.org/MBB91%20Webpage/Pitch_I_Shepard.pdf).

## Building

Use the MSVC toolchain on Windows.

1. Clone the repository with submodules, or use your existing checkout.
2. If you cloned without `--recursive`, fetch JUCE:

   ```powershell
   git submodule update --init --recursive
   ```

3. Configure and build as **two separate commands**:

   ```powershell
   cmake -S . -B build
   cmake --build build --config Release
   ```

For Visual Studio 2026 with a CMake version that does not offer its generator, run the included helper from PowerShell. It finds the installed MSVC toolchain and Ninja, configures Release in `build`, and builds locally:

```powershell
.\build.ps1
```

Or from a developer shell with Ninja available:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
```

Build outputs:

- `build/ASAS_HW3_artefacts/Release/Standalone/ASAS Shepard Keyboard.exe`
- `build/ASAS_HW3_artefacts/Release/VST3/ASAS Shepard Keyboard.vst3`

The VST3 is a **folder bundle**: copy the entire folder when installing it in your host's VST3 location. The build does not install it automatically. AU is additionally built on macOS.

## Offline demos

`ShepardRender` renders the actual plugin processor without opening an audio device. It checks octave identity, note release and the output ceiling, then writes ascending and descending WAVs to `output/audio`:

```powershell
& '.\build\ShepardRender_artefacts\Release\ShepardRender.exe'
```

Or build and render with `.\build.ps1 -RenderDemo`. Disable the renderer target with `-DASAS_BUILD_RENDERER=OFF` if you only want the instrument. This repository does not add a unit-test suite.

Verified locally with MSVC 19.51 and JUCE 8.0.6: Release VST3 and standalone builds succeeded. The actual processor produced identical C sounds twelve semitones apart, reached silence after release/all-notes-off, and stayed finite at 22.05, 44.1, 48 and 96 kHz. A 32-note chord at maximum volume peaked at 0.20 (-13.98 dBFS); both demo WAVs peaked at -30.63 dBFS. The renderer also saves `output/audio/keyboard.png` as an editor preview. No audio device or speaker playback is used by the renderer.

The assignment also asks for a demo video and a separate report PDF. The plugin and offline WAVs are materials for preparing those submissions; they do not constitute a recorded keyboard performance video.
