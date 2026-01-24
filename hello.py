"""
Music Player Application
A feature-rich music player with playlist management, album art display,
audio visualization, and playback controls.
"""

# Standard library imports
import io
import os
import random
import re
from pathlib import Path

# Third-party imports
import tkinter as tk
from tkinter import filedialog, ttk
import numpy as np
import pygame
from PIL import Image, ImageTk
from mutagen import File as MutagenFile
from mutagen.mp3 import MP3
from mutagen.id3 import ID3, APIC


class MusicPlayer:
    """Main Music Player application class."""
    
    def __init__(self, root):
        """Initialize the music player application."""
        self.root = root
        self.root.title("Music Player")
        self.root.geometry("600x500")
        self.root.config(bg="#C0C0C0")
        
        # Initialize pygame mixer
        pygame.mixer.init()
        
        # Initialize application state
        self._init_variables()
        
        # Create UI
        self.create_widgets()
        self.debug("Music Player initialized")
    
    def _init_variables(self):
        """Initialize all instance variables."""
        # Playlist variables
        self.playlist = []
        self.current_track_index = -1
        self.sort_type = "Name"  # Default sort type
        self.track_metadata = {}  # Cache for track metadata
        
        # Playback state
        self.is_paused = False
        self.volume = 0.7
        pygame.mixer.music.set_volume(self.volume)
        self.shuffle_mode = False
        
        # Seeking state
        self.is_seeking = False
        self.seek_offset = 0
        
        # Visual elements
        self.current_album_art = None
        self.visualizer_bars = 32
        self.bar_heights = [0] * self.visualizer_bars
        
        # Debug flag
        self.debug_mode = True
    
    # ==================== UTILITY METHODS ====================
    
    def debug(self, message):
        """Print debug messages if debug mode is enabled."""
        if self.debug_mode:
            print(f"[DEBUG] {message}")
    
    def format_time(self, seconds):
        """Format seconds into MM:SS format."""
        minutes = int(seconds // 60)
        secs = int(seconds % 60)
        return f"{minutes}:{secs:02d}"
    
    def get_track_info(self, file_path):
        """Get track metadata for sorting."""
        if file_path in self.track_metadata:
            return self.track_metadata[file_path]
        
        try:
            audio = MutagenFile(file_path)
            title = Path(file_path).stem
            artist = "Unknown Artist"
            album = "Unknown Album"
            
            if audio is not None and hasattr(audio, 'tags') and audio.tags:
                # Try to get title
                if 'TIT2' in audio.tags:
                    title = str(audio.tags['TIT2'])
                elif 'title' in audio.tags:
                    title = str(audio.tags['title'][0])
                
                # Try to get artist
                if 'TPE1' in audio.tags:
                    artist = str(audio.tags['TPE1'])
                elif 'artist' in audio.tags:
                    artist = str(audio.tags['artist'][0])
                
                # Try to get album
                if 'TALB' in audio.tags:
                    album = str(audio.tags['TALB'])
                elif 'album' in audio.tags:
                    album = str(audio.tags['album'][0])
            
            # Cache the metadata
            self.track_metadata[file_path] = {'title': title, 'artist': artist, 'album': album}
            return self.track_metadata[file_path]
        except:
            info = {'title': Path(file_path).stem, 'artist': "Unknown Artist", 'album': "Unknown Album"}
            self.track_metadata[file_path] = info
            return info
    
    # ==================== ALBUM ART METHODS ====================
    
    def set_default_album_art(self):
        """Set a default placeholder album art."""
        img = Image.new('RGB', (120, 120), color='#1a1a1a')
        photo = ImageTk.PhotoImage(img)
        self.album_art_label.config(image=photo)
        self.album_art_label.image = photo
    
    def load_album_art(self, file_path):
        """Extract and display album art from audio file."""
        try:
            audio = MutagenFile(file_path)
            
            # Try to extract album art
            album_art_data = None
            
            if hasattr(audio, 'tags'):
                # For MP3 files
                if 'APIC:' in audio.tags:
                    album_art_data = audio.tags['APIC:'].data
                elif 'APIC::' in audio.tags:
                    album_art_data = audio.tags['APIC::'].data
                # Try other APIC tags
                else:
                    for tag in audio.tags:
                        if tag.startswith('APIC'):
                            album_art_data = audio.tags[tag].data
                            break
            
            if album_art_data:
                # Load image from bytes
                img = Image.open(io.BytesIO(album_art_data))
                img = img.resize((120, 120), Image.Resampling.LANCZOS)
                photo = ImageTk.PhotoImage(img)
                self.album_art_label.config(image=photo)
                self.album_art_label.image = photo
                self.debug("Album art loaded successfully")
            else:
                self.debug("No album art found in file")
                self.set_default_album_art()
        except Exception as e:
            self.debug(f"Error loading album art: {e}")
            self.set_default_album_art()
    
    def load_track_metadata(self, file_path):
        """Load and display track metadata"""
        try:
            audio = MutagenFile(file_path)
            title = Path(file_path).stem
            artist = "Unknown Artist"
            album = "Unknown Album"
            
            if audio is not None and hasattr(audio, 'tags') and audio.tags:
                # Try to get title
                if 'TIT2' in audio.tags:
                    title = str(audio.tags['TIT2'])
                elif 'title' in audio.tags:
                    title = str(audio.tags['title'][0])
                
                # Try to get artist
                if 'TPE1' in audio.tags:
                    artist = str(audio.tags['TPE1'])
                elif 'artist' in audio.tags:
                    artist = str(audio.tags['artist'][0])
                
                # Try to get album
                if 'TALB' in audio.tags:
                    album = str(audio.tags['TALB'])
                elif 'album' in audio.tags:
                    album = str(audio.tags['album'][0])
            
            self.np_title_label.config(text=title)
            self.np_artist_label.config(text=artist)
            self.np_album_label.config(text=album)
            self.debug(f"Loaded metadata: {title} - {artist}")
        except Exception as e:
            self.debug(f"Error loading metadata: {e}")
            self.np_title_label.config(text=Path(file_path).stem)
            self.np_artist_label.config(text="Unknown Artist")
            self.np_album_label.config(text="Unknown Album")
    
    # ==================== VISUALIZER METHODS ====================
    
    def animate_visualizer(self):
        """Animate the audio visualizer."""
        self.visualizer_canvas.delete("all")
        
        canvas_width = self.visualizer_canvas.winfo_width()
        if canvas_width <= 1:
            canvas_width = 550  # Default width
        
        canvas_height = 80
        bar_width = canvas_width / self.visualizer_bars
        
        # Update bar heights with smooth animation
        if pygame.mixer.music.get_busy() and not self.is_paused:
            # Create dynamic animation based on music playing
            for i in range(self.visualizer_bars):
                # Simulate audio response with random values and smoothing
                target = random.uniform(0.3, 1.0) * canvas_height * 0.8
                self.bar_heights[i] += (target - self.bar_heights[i]) * 0.3
        else:
            # Decay bars when not playing
            for i in range(self.visualizer_bars):
                self.bar_heights[i] *= 0.85
        
        # Draw bars
        for i in range(self.visualizer_bars):
            x = i * bar_width
            height = self.bar_heights[i]
            y = canvas_height - height
            
            # Color gradient based on height
            if height > canvas_height * 0.6:
                color = "#e74c3c"  # Red for high
            elif height > canvas_height * 0.3:
                color = "#3498db"  # Blue for medium
            else:
                color = "#2ecc71"  # Green for low
            
            self.visualizer_canvas.create_rectangle(
                x + 1, y, x + bar_width - 1, canvas_height,
                fill=color,
                outline=""
            )
        
        # Continue animation
        self.root.after(50, self.animate_visualizer)
    
    # ==================== UI CREATION METHODS ====================
        
    def create_widgets(self):
        # Title Label
        title_label = tk.Label(
            self.root, 
            text="Music Player", 
            font=("MS Sans Serif", 16, "bold"),
            bg="#000080",
            fg="white",
            relief=tk.RAISED,
            bd=2
        )
        title_label.pack(pady=5, padx=5, fill=tk.X)
        
        # Current Track Display
        self.track_label = tk.Label(
            self.root,
            text="No track loaded",
            font=("MS Sans Serif", 10),
            bg="white",
            fg="black",
            wraplength=550,
            relief=tk.SUNKEN,
            bd=2
        )
        self.track_label.pack(pady=5, padx=5, fill=tk.X)
        
        # Playlist Frame
        playlist_frame = tk.Frame(self.root, bg="#C0C0C0", relief=tk.SUNKEN, bd=2)
        playlist_frame.pack(pady=5, padx=5, fill=tk.BOTH, expand=True)
        
        # Playlist Listbox
        self.playlist_box = tk.Listbox(
            playlist_frame,
            bg="white",
            fg="black",
            selectbackground="#000080",
            selectforeground="white",
            font=("MS Sans Serif", 9)
        )
        self.playlist_box.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.playlist_box.bind('<Double-Button-1>', self.play_selected_track)
        
        # Audio Visualizer
        visualizer_frame = tk.Frame(self.root, bg="#C0C0C0")
        visualizer_frame.pack(pady=5, padx=5, fill=tk.X)
        
        tk.Label(
            visualizer_frame,
            text="Audio Visualizer",
            font=("MS Sans Serif", 8),
            bg="#C0C0C0",
            fg="black"
        ).pack()
        
        self.visualizer_canvas = tk.Canvas(
            visualizer_frame,
            bg="black",
            height=80,
            relief=tk.SUNKEN,
            bd=2
        )
        self.visualizer_canvas.pack(fill=tk.X, pady=5)
        self.animate_visualizer()
        
        # Time Scrubber Frame
        scrubber_frame = tk.Frame(self.root, bg="#C0C0C0")
        scrubber_frame.pack(pady=5, padx=5, fill=tk.X)
        
        # Current Time Label
        self.current_time_label = tk.Label(
            scrubber_frame,
            text="0:00",
            font=("MS Sans Serif", 8),
            bg="#C0C0C0",
            fg="black",
            width=5
        )
        self.current_time_label.pack(side=tk.LEFT, padx=5)
        
        # Time Scrubber
        self.time_scrubber = tk.Scale(
            scrubber_frame,
            from_=0,
            to=100,
            orient=tk.HORIZONTAL,
            bg="#C0C0C0",
            fg="black",
            showvalue=False,
            relief=tk.SUNKEN,
            bd=2,
            troughcolor="white"
        )
        self.time_scrubber.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.time_scrubber.bind('<ButtonPress-1>', self.start_seek)
        self.time_scrubber.bind('<ButtonRelease-1>', self.end_seek)
        
        # Total Time Label
        self.total_time_label = tk.Label(
            scrubber_frame,
            text="0:00",
            font=("MS Sans Serif", 8),
            bg="#C0C0C0",
            fg="black",
            width=5
        )
        self.total_time_label.pack(side=tk.LEFT, padx=5)
        
        # Control Buttons Frame
        control_frame = tk.Frame(self.root, bg="#C0C0C0")
        control_frame.pack(pady=5)
        
        # Buttons
        btn_style = {
            "font": ("MS Sans Serif", 9, "bold"),
            "bg": "#C0C0C0",
            "fg": "black",
            "width": 10,
            "height": 1,
            "relief": tk.RAISED,
            "bd": 2
        }
        
        self.play_btn = tk.Button(control_frame, text="Play", command=self.play_music, **btn_style)
        self.play_btn.grid(row=0, column=0, padx=5)
        
        self.pause_btn = tk.Button(control_frame, text="Pause", command=self.pause_music, **btn_style)
        self.pause_btn.grid(row=0, column=1, padx=5)
        
        self.stop_btn = tk.Button(control_frame, text="Stop", command=self.stop_music, **btn_style)
        self.stop_btn.grid(row=0, column=2, padx=5)
        
        self.prev_btn = tk.Button(control_frame, text="Previous", command=self.previous_track, **btn_style)
        self.prev_btn.grid(row=1, column=0, padx=5, pady=5)
        
        self.next_btn = tk.Button(control_frame, text="Next", command=self.next_track, **btn_style)
        self.next_btn.grid(row=1, column=1, padx=5, pady=5)
        
        self.shuffle_btn = tk.Button(control_frame, text="Shuffle: OFF", command=self.toggle_shuffle, **btn_style)
        self.shuffle_btn.grid(row=1, column=2, padx=5, pady=5)
        
        # File Buttons Frame
        file_frame = tk.Frame(self.root, bg="#C0C0C0")
        file_frame.pack(pady=5)
        
        add_btn = tk.Button(file_frame, text="Scan Directory", command=self.scan_directory, **btn_style)
        add_btn.grid(row=0, column=0, padx=5)
        
        clear_btn = tk.Button(file_frame, text="Clear Playlist", command=self.clear_playlist, **btn_style)
        clear_btn.grid(row=0, column=1, padx=5)
        
        # Volume Control
        volume_frame = tk.Frame(self.root, bg="#C0C0C0")
        volume_frame.pack(pady=5)
        
        volume_label = tk.Label(volume_frame, text="Volume:", font=("MS Sans Serif", 9), bg="#C0C0C0", fg="black")
        volume_label.pack(side=tk.LEFT, padx=5)
        
        self.volume_slider = tk.Scale(
            volume_frame,
            from_=0,
            to=100,
            orient=tk.HORIZONTAL,
            command=self.change_volume,
            bg="#C0C0C0",
            fg="black",
            length=200,
            relief=tk.SUNKEN,
            bd=2,
            troughcolor="white"
        )
        self.volume_slider.set(self.volume * 100)
        self.volume_slider.pack(side=tk.LEFT, padx=5)
        
        # Sort Control
        sort_frame = tk.Frame(self.root, bg="#C0C0C0")
        sort_frame.pack(pady=5)
        
        sort_label = tk.Label(sort_frame, text="Sort by:", font=("MS Sans Serif", 9), bg="#C0C0C0", fg="black")
        sort_label.pack(side=tk.LEFT, padx=5)
        
        self.sort_dropdown = ttk.Combobox(
            sort_frame,
            values=["Name", "Artist", "Album"],
            state="readonly",
            width=15
        )
        self.sort_dropdown.set("Name")
        self.sort_dropdown.bind("<<ComboboxSelected>>", self.sort_playlist)
        self.sort_dropdown.pack(side=tk.LEFT, padx=5)
        
        # Now Playing Section at Bottom
        now_playing_frame = tk.Frame(self.root, bg="#C0C0C0", relief=tk.SUNKEN, bd=2)
        now_playing_frame.pack(pady=5, padx=5, fill=tk.X)
        
        # Album Art
        album_art_frame = tk.Frame(now_playing_frame, bg="#C0C0C0")
        album_art_frame.pack(side=tk.LEFT, padx=10, pady=10)
        
        self.album_art_label = tk.Label(
            album_art_frame,
            bg="black",
            width=120,
            height=120,
            relief=tk.SUNKEN,
            bd=2
        )
        self.album_art_label.pack()
        self.set_default_album_art()
        
        # Now Playing Info
        info_frame = tk.Frame(now_playing_frame, bg="#C0C0C0")
        info_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        tk.Label(
            info_frame,
            text="Now Playing",
            font=("MS Sans Serif", 9, "bold"),
            bg="#C0C0C0",
            fg="#000080"
        ).pack(anchor=tk.W)
        
        self.np_title_label = tk.Label(
            info_frame,
            text="No track",
            font=("MS Sans Serif", 11, "bold"),
            bg="#C0C0C0",
            fg="black",
            wraplength=350,
            justify=tk.LEFT
        )
        self.np_title_label.pack(anchor=tk.W, pady=(5, 2))
        
        self.np_artist_label = tk.Label(
            info_frame,
            text="Unknown Artist",
            font=("MS Sans Serif", 9),
            bg="#C0C0C0",
            fg="#000080",
            wraplength=350,
            justify=tk.LEFT
        )
        self.np_artist_label.pack(anchor=tk.W)
        
        self.np_album_label = tk.Label(
            info_frame,
            text="Unknown Album",
            font=("MS Sans Serif", 8),
            bg="#C0C0C0",
            fg="#808080",
            wraplength=350,
            justify=tk.LEFT
        )
        self.np_album_label.pack(anchor=tk.W, pady=(2, 0))
    
    # ==================== PLAYLIST MANAGEMENT METHODS ====================
        
    def scan_directory(self):
        directory = filedialog.askdirectory(title="Select Music Directory")
        if not directory:
            self.debug("Directory selection cancelled")
            return
        
        self.debug(f"Scanning directory: {directory}")
        
        # Supported audio formats
        audio_extensions = {'.mp3', '.wav', '.ogg', '.flac', '.m4a', '.aac', '.wma'}
        
        # Scan directory recursively for audio files
        files_found = 0
        for root, dirs, files in os.walk(directory):
            for file in files:
                if Path(file).suffix.lower() in audio_extensions:
                    file_path = os.path.join(root, file)
                    if file_path not in self.playlist:
                        self.playlist.append(file_path)
                        files_found += 1
        
        self.debug(f"Found {files_found} audio files")
        
        # Sort the playlist after scanning
        if self.playlist:
            self.sort_playlist(None)
    
    def sort_playlist(self, event):
        """Sort the playlist based on selected criteria."""
        if not self.playlist:
            return
        
        sort_by = self.sort_dropdown.get()
        self.debug(f"Sorting playlist by {sort_by}")
        
        # Get currently playing track
        current_track = None
        if self.current_track_index >= 0 and self.current_track_index < len(self.playlist):
            current_track = self.playlist[self.current_track_index]
        
        # Sort based on selected criteria
        if sort_by == "Name":
            self.playlist.sort(key=lambda x: self.get_track_info(x)['title'].lower())
        elif sort_by == "Artist":
            self.playlist.sort(key=lambda x: (self.get_track_info(x)['artist'].lower(), self.get_track_info(x)['title'].lower()))
        elif sort_by == "Album":
            self.playlist.sort(key=lambda x: (self.get_track_info(x)['album'].lower(), self.get_track_info(x)['title'].lower()))
        
        # Update the current track index
        if current_track:
            try:
                self.current_track_index = self.playlist.index(current_track)
            except ValueError:
                self.current_track_index = -1
        
        # Update the display
        self.update_playlist_display()
    
    def update_playlist_display(self):
        """Refresh the playlist display."""
        self.playlist_box.delete(0, tk.END)
        for i, track in enumerate(self.playlist, 1):
            # Remove any existing numbering from the filename
            track_name = Path(track).stem
            # Strip leading numbers and dots/dashes (e.g., "07. " or "07 - ")
            track_name = re.sub(r'^\d+[\.\-\s]+', '', track_name)
            self.playlist_box.insert(tk.END, f"{i}. {track_name}")
        
        # Reselect the current track if playing
        if self.current_track_index >= 0 and self.current_track_index < len(self.playlist):
            self.playlist_box.select_set(self.current_track_index)
            self.playlist_box.see(self.current_track_index)
    
    def clear_playlist(self):
        """Clear all tracks from the playlist."""
        self.stop_music()
        self.playlist.clear()
        self.track_metadata.clear()
        self.playlist_box.delete(0, tk.END)
        self.current_track_index = -1
        self.track_label.config(text="No track loaded")
    
    # ==================== PLAYBACK CONTROL METHODS ====================
    
    def play_music(self):
        if not self.playlist:
            self.debug("Play called but playlist is empty")
            return
        
        if self.is_paused:
            pygame.mixer.music.unpause()
            self.is_paused = False
            self.debug("Music unpaused")
        else:
            if self.current_track_index == -1:
                self.current_track_index = 0
            
            track = self.playlist[self.current_track_index]
            self.debug(f"Playing track {self.current_track_index + 1}/{len(self.playlist)}: {Path(track).stem}")
            pygame.mixer.music.load(track)
            pygame.mixer.music.play()
            self.seek_offset = 0
            
            # Load album art and metadata
            self.load_album_art(track)
            self.load_track_metadata(track)
            
            # Get track length
            try:
                sound = pygame.mixer.Sound(track)
                self._track_length = sound.get_length()
            except:
                self._track_length = 100
            
            # Update display
            self.track_label.config(text=f"Now Playing: {Path(track).stem}")
            self.playlist_box.select_clear(0, tk.END)
            self.playlist_box.select_set(self.current_track_index)
            self.playlist_box.see(self.current_track_index)
        
        # Check if track has ended
        self.root.after(1000, self.check_track_end)
    
    def pause_music(self):
        if pygame.mixer.music.get_busy():
            pygame.mixer.music.pause()
            self.is_paused = True
            self.debug("Music paused")
    
    def stop_music(self):
        pygame.mixer.music.stop()
        self.is_paused = False
        self.bar_heights = [0] * self.visualizer_bars
        self.debug("Music stopped")
    
    def toggle_shuffle(self):
        """Toggle shuffle mode on/off."""
        self.shuffle_mode = not self.shuffle_mode
        if self.shuffle_mode:
            self.shuffle_btn.config(text="Shuffle: ON", relief=tk.SUNKEN)
            self.debug("Shuffle mode enabled")
        else:
            self.shuffle_btn.config(text="Shuffle: OFF", relief=tk.RAISED)
            self.debug("Shuffle mode disabled")
    
    # ==================== TRACK NAVIGATION METHODS ====================
    
    def next_track(self):
        """Play the next track in the playlist (or random if shuffle is on)."""
        if not self.playlist:
            return
        
        if self.shuffle_mode:
            # Pick a random track
            self.current_track_index = random.randint(0, len(self.playlist) - 1)
            self.debug(f"Shuffle: random track {self.current_track_index}")
        else:
            # Go to next track in order
            self.current_track_index = (self.current_track_index + 1) % len(self.playlist)
            self.debug(f"Next track: index {self.current_track_index}")
        
        self.stop_music()
        self.play_music()
    
    def previous_track(self):
        """Play the previous track in the playlist."""
        if not self.playlist:
            return
        
        self.current_track_index = (self.current_track_index - 1) % len(self.playlist)
        self.debug(f"Previous track: index {self.current_track_index}")
        self.stop_music()
        self.play_music()
    
    def play_selected_track(self, event):
        """Play a track selected from the playlist."""
        selection = self.playlist_box.curselection()
        if selection:
            self.current_track_index = selection[0]
            self.stop_music()
            self.play_music()
    
    # ==================== VOLUME & SEEKING METHODS ====================
    
    def change_volume(self, value):
        self.volume = float(value) / 100
        pygame.mixer.music.set_volume(self.volume)
        self.debug(f"Volume changed to {int(self.volume * 100)}%")
    
    def start_seek(self, event):
        """Called when user starts dragging the scrubber."""
        self.is_seeking = True
        self.debug("Started seeking")
    
    def end_seek(self, event):
        """Called when user releases the scrubber - actually performs the seek."""
        if not self.playlist or self.current_track_index == -1:
            self.is_seeking = False
            return
        
        seek_position = self.time_scrubber.get()
        self.seek_offset = seek_position
        self.debug(f"Seeking to {self.format_time(seek_position)}")
        
        # Stop current playback
        was_paused = self.is_paused
        pygame.mixer.music.stop()
        
        # Reload and play from new position
        track = self.playlist[self.current_track_index]
        pygame.mixer.music.load(track)
        pygame.mixer.music.play(start=seek_position)
        
        # If it was paused, pause again
        if was_paused:
            pygame.mixer.music.pause()
        
        self.is_seeking = False
    
    def check_track_end(self):
        """Monitor track playback and update UI elements."""
        if pygame.mixer.music.get_busy() or self.is_paused:
            # Update time scrubber (but not while user is seeking)
            if not self.is_seeking:
                try:
                    current_pos = (pygame.mixer.music.get_pos() / 1000.0) + self.seek_offset
                    if hasattr(self, '_track_length'):
                        self.time_scrubber.config(to=self._track_length)
                        self.time_scrubber.set(current_pos)
                        self.current_time_label.config(text=self.format_time(current_pos))
                        self.total_time_label.config(text=self.format_time(self._track_length))
                except:
                    pass
            # Continue checking
            self.root.after(1000, self.check_track_end)
        else:
            # Track has ended, play next
            if self.playlist and self.current_track_index != -1:
                self.next_track()
            else:
                # Reset scrubber
                self.time_scrubber.set(0)
                self.current_time_label.config(text="0:00")


# ==================== APPLICATION ENTRY POINT ====================

def main():
    """Initialize and run the music player application."""
    root = tk.Tk()
    app = MusicPlayer(root)
    root.mainloop()


if __name__ == "__main__":
    main()