# MusicPlayer TUI

Terminal music player with two modes:
- **Local**: browse and play `.mp3`/`.wav` from `Music/` using `mpg123`.
- **Spotify**: control an existing Spotify player via `playerctl` (e.g., `spotifyd`, `librespot`, or the official app).

## Requirements
- ncurses runtime (`libncurses`/`libtinfo`).
- `mpg123` for local playback.
- `playerctl` for Spotify control.
- `cava` for audio visualization (optional; toggle with `v` in Spotify mode).
- A running Spotify player that exposes the `spotify` MPRIS name (`spotifyd`, `librespot`, or Spotify desktop).

### Install examples
- Debian/Ubuntu: `sudo apt-get install libncurses6 libtinfo6 mpg123 playerctl cava`
- Fedora: `sudo dnf install ncurses-libs mpg123 playerctl cava`
- Arch: `sudo pacman -S ncurses mpg123 playerctl cava`

## Build and run
```bash
make clean && make
make run
```

## Usage
- Local mode (default):
  - Up/Down: move selection
  - Enter/Space: play selected song
  - r: refresh song list
  - s: stop playback
  - m: switch mode
  - q: quit
- Spotify mode:
  - Enter/Space: play/pause
  - n: next track
  - b: previous track
  - v: toggle visualizer
  - m: switch mode
  - q: quit

## Notes
- Place your audio files under `Music/` relative to the binary.
- Spotify mode only sends commands; it does not stream audio itself. Ensure a Spotify player is running and reachable by `playerctl --player=spotify status`.
