# Music Player

A simple music player built with Python that plays your music collection with a nice visual interface.

## What It Does

- Play music from your computer
- Shows album artwork and song information
- Cool animated visualizer that moves with the music
- Control playback (play, pause, stop, next, previous)
- Shuffle your music randomly
- Adjust volume
- Jump to any part of a song with the time slider

## How to Use

### Installation

1. Install the required packages:
```bash
pip install pygame-ce Pillow mutagen numpy
```

2. Run the program:
```bash
python hello.py
```

### Playing Music

1. Click **"Scan Directory"** and choose a folder with your music
2. All songs in that folder (and subfolders) will be added to the playlist
3. Double-click any song to play it, or click **"Play"**
4. Use the buttons to control playback:
   - **Play** - Start playing
   - **Pause** - Pause the music
   - **Stop** - Stop completely
   - **Next/Previous** - Skip between songs
   - **Shuffle** - Play songs in random order

### Other Features

- Drag the time slider to jump to different parts of the song
- Use the volume slider to make it louder or quieter
- The visualizer at the top shows animated bars that react to the music
- Album art and song info appear at the bottom when playing

## Supported Audio Formats

MP3, WAV, OGG, FLAC, M4A, AAC, WMA

## What You Need

- Python 3.7 or newer
- pygame-ce 2.5+
- Pillow
- mutagen
- numpy

## Tips

- Turn on debug mode to see what's happening in the console
- The visualizer bars change color based on intensity (green → blue → red)
- You can clear the playlist anytime with **"Clear Playlist"**
