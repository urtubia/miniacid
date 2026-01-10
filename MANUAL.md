# MiniAcid User Manual

## Table of Contents
1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Hardware Requirements](#hardware-requirements)
4. [Installation](#installation)
5. [Interface Overview](#interface-overview)
6. [Transport & Playback](#transport--playback)
7. [Pages & Navigation](#pages--navigation)
8. [TB-303 Synthesizer](#tb-303-synthesizer)
9. [Pattern Editing](#pattern-editing)
10. [Drum Sequencer](#drum-sequencer)
11. [Song Mode](#song-mode)
12. [Project Management](#project-management)
13. [Recording Audio](#recording-audio)
14. [Keyboard Shortcuts Reference](#keyboard-shortcuts-reference)
15. [Mouse Controls](#mouse-controls)

---

## Introduction

**MiniAcid** is a portable acid groovebox for the M5Stack Cardputer and web. It combines two squelchy TB-303 style bass synthesizers with a punchy TR-808 inspired drum machine, giving you a complete rhythm production tool in your pocket.

### Key Features
- **Two TB-303 voices** with full filter, envelope, and effects controls
- **Eight-voice drum machine** with classic 808 sounds (kick, snare, hats, toms, rim, clap)
- **16-step sequencer** for each voice
- **Pattern banks** with 8 slots per instrument
- **Song mode** for arranging patterns into complete tracks
- **Live muting** for all voices
- **Audio recording** to WAV files
- **Scene management** with save/load functionality
- **Cross-platform**: runs on M5Stack Cardputer, and web browsers

---

## Getting Started

### Quick Start
1. Power on your M5Stack Cardputer/open MiniAcid in your web browser
2. Press **SPACE** to start playback
3. Use **`[`** and **`]`** to navigate between pages
4. Use **'ESC'** to open the help page for keyboard shortcuts on each page
5. Adjust parameters using the keyboard shortcuts
6. Press **SPACE** again to stop (auto-saves to SD card if available)

---

## Installation

### Web

Go to https://miniacid.mrbook.org

### Cardputer

1. Search for "MiniAcid" in Launcher or M5Burner
2. Install and run


---

## Interface Overview

### Screen Layout

The MiniAcid interface consists of several sections:

```
┌─────────────────────────────────────┐
│ TRANSPORT BAR (BPM, Play indicator) │
├─────────────────────────────────────┤
│                                     │
│          PAGE CONTENT               │
│       (varies by active page)       │
│                                     │
├─────────────────────────────────────┤
│    MUTE SECTION (Voice status)      │
└─────────────────────────────────────┘
```

#### Transport Bar
- Shows page title
- Shows current BPM
- Play/stop indicator (blue in Pattern mode, yellow in Song mode)

#### Page Content
- Main content area that changes based on the active page
- Displays parameters, patterns, sequencer grids, or waveforms

#### Mute Section
- Shows mute status for all voices
- Visual indicators: dimmed = muted, bright = active

### Pages

MiniAcid has several pages accessible via `[` and `]` keys:

1. **TB-303 Page (A)** - First bass synth parameters and waveform
2. **Pattern Edit Page (A)** - 303A sequencer editor
3. **TB-303 Page (B)** - Second bass synth parameters and waveform
4. **Pattern Edit Page (B)** - 303B sequencer editor
5. **Drum Sequencer** - Drum pattern editor
6. **Song Mode** - Pattern arrangement and song sequencing
7. **Project Page** - Scene management and settings
8. **Help Page** - Keyboard shortcuts and controls

---

## Transport & Playback

### Starting and Stopping

**Play/Stop**: Press **SPACE** 
- Starts playback from the current position
- Stopping automatically saves the scene 
  - Cardputer: It saves to SD card (if available)
  - Web: Saves to browser local storage

### Tempo Control

**Adjust BPM**:
- **`K`** - Decrease BPM by 5
- **`L`** - Increase BPM by 5
- Range: typically 60-200 BPM

### Playback Modes

MiniAcid has two playback modes:

1. **Pattern Mode** (default, blue indicator)
   - Plays the currently selected pattern for each instrument
   - Each voice loops its active pattern independently

2. **Song Mode** (yellow indicator)
   - Plays the song arrangement from the Song page
   - Patterns change automatically according to song positions

You can togle between modes using Ctrl/Cmd + `P` or by pressing `M` on the Song page.`

---

## TB-303 Synthesizer

### Overview

MiniAcid features two independent TB-303 style bass synthesizers (303A and 303B). Each has its own parameter page and pattern editor.

### Parameters

Each 303 voice has the following controls:

#### Filter Section
- **Cutoff** - Filter cutoff frequency (controls brightness)
  - **`A`** - Increase cutoff
  - **`Z`** - Decrease cutoff

- **Resonance** - Filter resonance (controls emphasis/squelch)
  - **`S`** - Increase resonance
  - **`X`** - Decrease resonance

#### Envelope Section
- **Env Amount** - Envelope modulation depth
  - **`D`** - Increase envelope amount
  - **`C`** - Decrease envelope amount

- **Decay** - Envelope decay time
  - **`F`** - Increase decay
  - **`V`** - Decrease decay

#### Oscillator
- **Waveform** - Oscillator waveform type
  - Options: Saw, Square, Pulse (various widths)
  - Adjust in parameter page

#### Effects
- **Delay** - Tempo-synced delay effect
  - **`M`** - Toggle delay on/off

- **Distortion** - Saturation/overdrive effect
  - **`N`** - Toggle distortion on/off

### Mouse Control (Desktop/Web)

On the 303 parameter pages, you can use the mouse to adjust knobs:
- **Click and drag** on a knob to adjust value
- **Scroll wheel** over a knob for fine adjustment

---

## Pattern Editing

### Overview

Each 303 voice has a dedicated pattern editor page with:
- Two rows of 8 steps (16 steps total)
- Note display for each step
- Slide and accent indicators
- Pattern bank selector (8 pattern slots)

### Navigation

**Move Between Steps**:
- **Arrow keys** or **Cardputer arrow cluster** (`; , . /`)
- Selected step highlighted with a border

**Move Between Rows**:
- **UP/DOWN arrows**

**Pattern Selection**:
- When cursor is on the patterns row, **`Q` through `I`** - Select pattern slots 1-8
- **ENTER** - Load the highlighted pattern (when on pattern row)

### Editing Steps

When a step is focused:

**Note Editing**:
- **`A`** - Increase note by one semitone
- **`Z`** - Decrease note by one semitone
- Range: C1 to B4 (4 octaves, including sharps)

**Octave Change**:
- **`S`** or **ALT+UP** - Increase octave
- **`X`** or **ALT+DOWN** - Decrease octave

**Modifiers**:
- **`Q`** - Toggle slide (glide to next note)
- **`W`** - Toggle accent (emphasis/louder)

**Clear Step**:
- **BACKSPACE** - Clear the step (no note plays)

### Visual Indicators

- **Note box** - Large square showing note value (e.g., "C#2")
- **Slide indicator** - Small colored box above note (green)
- **Accent indicator** - Small colored box above note (yellow)
- **Current step** - Border around the selected step

### Pattern Banks

Each 303 voice has 8 pattern slots:
- Switch between slots using **`Q`-`I`** keys
- Each slot stores a complete 16-step pattern
- Active pattern number shown at top of page
- Bank selection bar shows all 8 slots

### Pattern Operations

**Copy/Paste**:
- **CTRL+C** or **CMD+C** - Copy current pattern
- **CTRL+V** or **CMD+V** - Paste pattern to current slot
- **CTRL+X** or **CMD+X** - Cut pattern (copy and clear)

**Undo**:
- **CTRL+Z** or **CMD+Z** - Undo last edit

### Mouse Control (Desktop/Web)

- **Click on a step** to select it
- **Click on pattern slots** in the bank selector to switch patterns
- Drag across pattern selector for quick switching

---

## Drum Sequencer

### Overview

The drum sequencer features 8 drum voices arranged in a grid:
- 16 steps per voice
- 8 pattern slots
- Individual voice muting

### Drum Voices

1. **Kick** (3) - Bass drum
2. **Snare** (4) - Snare drum
3. **Closed Hat** (5) - Closed hi-hat
4. **Open Hat** (6) - Open hi-hat
5. **Mid Tom** (7) - Mid tom
6. **High Tom** (8) - High tom
7. **Rim** (9) - Rim shot
8. **Clap** (0) - Hand clap

*(Numbers in parentheses are mute keys)*

### Navigation

**Move in Grid**:
- **Arrow keys** or **Cardputer arrows**
- Highlight moves across steps and voices

**Pattern Selection**:
- **`Q` through `I`** - Select drum pattern slots 1-8
- Pattern row at top shows available patterns

### Editing

**Toggle Hit**:
- **ENTER** - Toggle drum hit on/off at current step
- Active hits shown as filled squares

**Load Pattern**:
- Navigate to pattern row (top of grid)
- Press **ENTER** to load selected pattern

### Mouse Control (Desktop/Web)

- **Click on grid cells** to toggle drum hits
- **Click pattern slots** to switch patterns
- Visual feedback on hover

---

## Song Mode

### Overview

Song Mode allows you to arrange patterns into complete songs by creating a sequence of song positions. Each position defines which patterns play for all three tracks (303A, 303B, Drums).

### Accessing Song Mode

- Navigate to the **Song Page** using `[` and `]` keys
- The page shows a grid of song positions, reminiscent of a tracker interface

### Song Structure

**Tracks** (columns):
- **303A** - First bass synthesizer
- **303B** - Second bass synthesizer  
- **Drums** - Drum machine

**Positions** (rows):
- Song positions 1-128
- Each position can have different pattern assignments
- Length: 1 to 128 positions

### Navigation

**Move Selection**:
- **Arrow keys** - Navigate between cells
- **UP/DOWN** - Move between positions (rows)
- **LEFT/RIGHT** - Move between tracks (columns) or focus mode toggle

**Vertical Scrolling**:
- Grid scrolls automatically during playback
- Selected cell is always visible

### Editing Pattern Assignments

**Assign Pattern to Cell**:
- Select a cell (track × position)
- Using **ALT + ARROW UP/ARROW DOWN**, change the assigned pattern to that cell
- Pattern bank+number appears in cell

**Clear Assignment**:
- **BACKSPACE** - Clear pattern from selected cell
- Track will be silent at that position

**Change Bank** (if supported):
- **ALT+UP** - Next bank
- **ALT+DOWN** - Previous bank

### Playback Control

**Mode Toggle**:
- **`M`** or **ENTER (on mode indicator)** - Toggle between Song Mode and Pattern Mode

**Pattern Mode** (blue indicator):
- Plays currently selected patterns continuously
- Pattern selection on 303/drum pages determines what plays

**Song Mode** (yellow indicator):
- Plays song arrangement sequentially
- Pattern changes according to song grid
- Playhead indicator shows current position

**Playhead Navigation** (during playback):
- **ALT+UP** - Jump to next position
- **ALT+DOWN** - Jump to previous position

### Song Length

- Default: 1 position
- Automatically extends when you edit positions beyond current length
- Maximum: 128 positions

### Integration with Other Pages

When **Song Mode is active**:
- Transport bar shows **yellow** indicator
- 303 and drum pages display the pattern currently playing at the current song position
- Pattern editors still work normally for editing individual patterns

---

## Project Management

### Scenes

A **Scene** contains your complete project state:
- All pattern banks (303A, 303B, Drums)
- All 303 parameter settings (filter, envelope, effects)
- Song arrangement
- Mute states

### Saving

**Auto-Save**:
- Scene automatically saves when you **stop playback** (press SPACE)
- Requires SD card on Cardputer

**Manual Save**:
- **CTRL+S** or **CMD+S** - Save scene immediately

### Loading

- On startup, MiniAcid loads the last saved scene from SD card
- If no scene exists, loads default patterns

### File Locations

**Cardputer**:
- Saves to SD card root. For example: `/miniacid_scene.json`

**Web Browser**:
- Uses browser local storage
- Persists between sessions

---

## Recording Audio

### Overview

MiniAcid can record your sessions to WAV audio files. Recording captures the master output including all voices, live changes and effects.

### Starting Recording

**Keyboard Shortcut**:
- **CTRL+R** or **CMD+R** - Start recording

**What Happens**:
- Recording indicator appears (red square) in the transport bar
- Audio begins writing to WAV file
- All generated audio is captured in real-time

### Stopping Recording

**Keyboard Shortcut**:
- **CTRL+R** or **CMD+R** again - Stop recording
- Or press **SPACE** to stop playback (also stops recording)

### Recording Format

- **Sample Rate**: 22050 Hz (22.05 kHz)
- **Bit Depth**: 16-bit PCM
- **Channels**: Mono
- **Format**: WAV (uncompressed)

### File Locations

**Cardputer**:
- Saves to SD card: `/miniacid_YYYYMMDD_HHMMSS.wav`
- Example: `miniacid_20260109_143045.wav`

**Web Browser**:
- Automatically downloads when recording stops
- Filename: `miniacid_recording.wav`

### Tips for Recording

- **Start playback first**, then start recording for a clean start
- **Let patterns loop** at least once for complete bars
- **Stop recording cleanly** using the keyboard shortcut
- **Check SD card space** on Cardputer before long recordings
- Typical file size: ~1.3 MB per minute of audio

---

## Keyboard Shortcuts Reference

### Global Controls

| Key | Action |
|-----|--------|
| **SPACE** | Play/Stop (auto-saves on stop) |
| **K** | Decrease BPM (-5) |
| **L** | Increase BPM (+5) |
| **`[`** | Previous page |
| **`]`** | Next page |
| **ENTER** | Dismiss splash / Confirm action |
| **ESC** | Open page help |
| **`-` `=`** | Decrease/Increase main volume |

### Pattern Randomization

| Key | Action |
|-----|--------|
| **I** | Randomize 303A pattern |
| **O** | Randomize 303B pattern |
| **P** | Randomize drum pattern |

### Mute Controls

| Key | Voice |
|-----|-------|
| **1** | 303A synthesizer |
| **2** | 303B synthesizer |
| **3** | Kick drum |
| **4** | Snare drum |
| **5** | Closed hat |
| **6** | Open hat |
| **7** | Mid tom |
| **8** | High tom |
| **9** | Rim shot |
| **0** | Clap |

### TB-303 Parameters (on 303 pages)

| Key | Parameter | Change |
|-----|-----------|--------|
| **A** | Cutoff | Increase |
| **Z** | Cutoff | Decrease |
| **S** | Resonance | Increase |
| **X** | Resonance | Decrease |
| **D** | Env Amount | Increase |
| **C** | Env Amount | Decrease |
| **F** | Decay | Increase |
| **V** | Decay | Decrease |
| **M** | Delay | Toggle on/off |
| **N** | Distortion | Toggle on/off |

### Pattern Editing (303 & Drum pages)

| Key | Action |
|-----|--------|
| **Arrow Keys** | Navigate steps/cells |
| **Q**-**I** | Select pattern slots 1-8 (when on patterns row)|
| **ENTER** | Load pattern / Toggle hit |
| **BACKSPACE** | Clear step |

### 303 Step Editing (when step focused)

| Key | Action |
|-----|--------|
| **Q** | Toggle slide |
| **W** | Toggle accent |
| **A** | Note +1 semitone |
| **Z** | Note -1 semitone |
| **S** | Octave up |
| **X** | Octave down |

### Song Mode (on Song page)

| Key | Action |
|-----|--------|
| **Arrow Keys** | Navigate song grid |
| **BACKSPACE** | Clear cell |
| **M** | Toggle Song/Pattern mode |
| **ENTER** | Toggle mode (on mode row) |
| **ALT+UP** | Increase bank / Next position (playing) |
| **ALT+DOWN** | Decrease bank / Prev position (playing) |

### Application Events (Ctrl/Cmd modifiers)

| Key | Action |
|-----|--------|
| **CTRL/CMD+C** | Copy pattern |
| **CTRL/CMD+V** | Paste pattern |
| **CTRL/CMD+X** | Cut pattern |
| **CTRL/CMD+Z** | Undo last edit |
| **CTRL/CMD+S** | Save scene |
| **CTRL/CMD+J** | Start/Stop recording |

---

## Mouse Controls

*Available in Desktop/Web versions*

### TB-303 Parameter Pages

**Knob Interaction**:
- **Click and drag** - Adjust parameter value
  - Drag up to increase, down to decrease
  - Smooth, continuous control
- **Mouse wheel** - Fine adjustment
  - Scroll up to increase, down to decrease
  - Precise value changes
- **Visual feedback** - Knob updates in real-time

### Pattern Editors (303 & Drums)

**Step Selection**:
- **Click on step** - Select that step for editing
- **Visual highlight** - Shows selected step

**Pattern Bank**:
- **Click on pattern slot** - Switch to that pattern
- **Active pattern** - Highlighted in bank selector

### Drum Sequencer

**Grid Editing**:
- **Click on cell** - Toggle drum hit on/off
- **Click and drag** - Paint multiple hits (if supported)
- **Hover effect** - Visual feedback before clicking

### Song Page

**Cell Selection**:
- **Click on cell** - Select position/track
- **Visual highlight** - Shows selected cell

**Pattern Assignment**:
- Use keyboard after clicking cell to assign patterns

### Components

**Bank Selection Bar**:
- **Click on bank** - Switch to that bank
- Visual indicator shows active bank

**Pattern Selection Bar**:
- **Click on pattern** - Load that pattern
- Active pattern highlighted

---

## Tips & Tricks

### Creating Acid Basslines

1. **Start simple** - Use 4-8 notes, not all 16 steps
2. **Use slides** - The "acid" sound comes from sliding between notes
3. **Add accents** - Emphasize certain beats (1 and 3, or syncopated)
4. **Cutoff and resonance** - High resonance + moving cutoff = squelch
5. **Envelope modulation** - Moderate env amount (200-600) sounds natural
6. **Randomize and edit** - Press `I` or `O` to generate ideas, then refine

### Drum Programming

1. **Four-on-floor** - Kick on every beat (steps 1, 5, 9, 13)
2. **Snare backbeat** - Snare on 2 and 4 (steps 5, 13)
3. **Hat patterns** - Closed hat every other step for energy
4. **Open hats** - Use sparingly for emphasis and release
5. **Randomize for variety** - Press `P` then edit to taste

### Performance Techniques

1. **Live muting** - Use number keys to drop voices in/out
2. **Parameter tweaking** - Adjust filter cutoff while playing
3. **Pattern switching** - Change patterns on-the-fly (switches at next loop)
4. **BPM changes** - Gradually increase tempo for energy
5. **Effects** - Toggle delay/distortion for texture changes

---

**Happy acid jamming!** 🎵🎛️🔊
