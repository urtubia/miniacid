# fork: ajrAcid

This is a personal fork of MiniAcid, a tiny acid groovebox for the M5Stack Cardputer. I've forked it to make some changes and make this awesome little app more to my liking. A huge THANK YOU to [urtubia](https://github.com/urtubia/) for developing this and sharing it with the world! The UI is excellent, it's well thought out.

DONE:
- Simple bus compressor for the drums to increase punch
- Updated all drum voices - BD, SN, CH, OH, RS, and CP all have been changed. Current clap is a burst of static, but I'm working on that.
- Changed banner to "ajrAcid" in blue so I can tell whether I'm running my fork or the original
- Lowered the resonance floor - there was always some squelch with MiniAcid 0.0.5. Especially with two voices, it's important to me to have a squelchless 303.

TODO - Short Term:
- Better clap - currently it's a burst of noise, working on that. Aiming for something like the 808 clap.
- Improving the hi-hats - I wanted a crispier and more metalic hi-hats, 606/808ish

TODO - Medium term:
- Drum parameter screen: Modify drum BusCompAmount, drum voice decay time(s)
- Mixer screen: to change levels of each synth and drum voice

# MiniAcid

MiniAcid is a tiny acid groovebox for the M5Stack Cardputer. It runs two squelchy TB-303 style voices plus a punchy TR-808 inspired drum section on the Cardputer's built-in keyboard and screen, so you can noodle basslines and beats anywhere.

## What it does
- Two independent 303 voices with filter/env controls and optional tempo-synced delay
- 16-step sequencers for both acid lines and drums, with quick randomize actions
- Live mutes for every part (two synths + eight drum lanes)
- On-device UI with waveform view, help pages, and page hints

## Using it
1) Flash `miniacid.ino` to your M5Stack Cardputer ADV (Arduino IDE/PlatformIO).  
2) Use the keyboard shortcuts below to play, navigate pages, and tweak sounds.  
3) Stopping play (SPACE) triggers saving to the Cardputer's SD Card automatically, if there's an SD card inserted.
4) Jam, tweak synths, sequence, randomize, and mute on the fly

### Keyboard & button cheatsheet
- **Transport:** `SPACE` or Cardputer `BtnA` toggles play/stop. `K` / `L` BPM down/up.
- **Navigation:** `[` / `]` previous/next page. (Press `ENTER` to dismiss the splash/help.)
- **Sequencer randomize:** `I` random 303A pattern, `O` random 303B pattern, `P` random drum pattern.
- **Sound shaping (on the active 303 page):**
  - `A` / `Z` cutoff up/down
  - `S` / `X` resonance up/down
  - `D` / `C` env amount up/down
  - `F` / `V` decay up/down
  - `M` toggle delay for the active 303 voice
- **303 pattern edit pages (A/B):** Use the Cardputer arrow cluster (`; , . /`) or host arrow keys to move between steps and pattern slots; `ENTER` loads the highlighted pattern. `Q..I` choose pattern slots 1-8. When a step is focused: `Q` slide, `W` accent, `A` / `Z` note +1 / -1, `S` / `X` octave up/down, `BACKSPACE` clears the step.
- **Drum sequencer page:** Use the arrow cluster (`; , . /`) or host arrows to move. `ENTER` toggles a hit (or loads the highlighted drum pattern when the pattern row is focused). `Q..I` pick drum pattern slots 1-8.
- **Mutes:** `1` 303A, `2` 303B, `3` kick, `4` snare, `5` closed hat, `6` open hat, `7` mid tom, `8` high tom, `9` rim, `0` clap.

Tip: Each 303 page controls one voice (A on the first knob page, B on the second), and the page hint in the top-right reminds you where you are.
