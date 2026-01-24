# Music Player - TODO List

## Add Winamp Skin Support

### Research & Investigation
- [ ] Research Winamp skin file format (.wsz, .wal)
- [ ] Understand Winamp skin structure (XML, images, layouts)
- [ ] Study Winamp skin specifications and documentation
- [ ] Find Python libraries for parsing Winamp skins
- [ ] Research how to extract images from .wsz files (they're ZIP archives)

### Skin File Structure Understanding
- [ ] Learn about pledit.txt (playlist editor specs)
- [ ] Learn about main.bmp (main window graphics)
- [ ] Learn about region.txt (window shapes)
- [ ] Understand button positions and states
- [ ] Understand slider graphics and positions
- [ ] Learn color schemes from .txt files

### Code Implementation

#### 1. Skin Loading System
- [ ] Create `SkinManager` class to handle skin loading
- [ ] Add method to extract .wsz files (ZIP format)
- [ ] Add method to parse skin configuration files
- [ ] Add method to load skin images (BMPs, PNGs)
- [ ] Create default skin fallback system

#### 2. UI Redesign for Skinnable Interface
- [ ] Convert current fixed UI to dynamic skinnable layout
- [ ] Replace Tkinter widgets with Canvas-based custom widgets
- [ ] Create custom button class that uses skin images
- [ ] Create custom slider class with skin graphics
- [ ] Implement image-based window frame

#### 3. Skin File Parser
- [ ] Parse pledit.txt for playlist styling
- [ ] Parse main window dimensions and regions
- [ ] Parse button positions from skin files
- [ ] Parse color values for text and backgrounds
- [ ] Handle missing or corrupted skin files

#### 4. Graphics System
- [ ] Load and cache all skin bitmap images
- [ ] Implement sprite system for button states (normal, hover, pressed)
- [ ] Handle transparency in skin images
- [ ] Scale images if needed (or enforce original size)
- [ ] Implement double-buffering for smooth rendering

#### 5. Widget Implementations
- [ ] Custom skinned play button with hover/click states
- [ ] Custom skinned pause button
- [ ] Custom skinned stop button
- [ ] Custom skinned next/previous buttons
- [ ] Custom skinned volume slider
- [ ] Custom skinned seek bar (time scrubber)
- [ ] Custom skinned playlist area
- [ ] Custom skinned visualizer area
- [ ] Custom title bar with close/minimize buttons

#### 6. Skin Selection UI
- [ ] Add "Load Skin" button or menu option
- [ ] Create skin browser dialog
- [ ] Show skin preview thumbnails
- [ ] Allow user to select from multiple installed skins
- [ ] Save user's skin preference

#### 7. Configuration & Storage
- [ ] Create skins folder structure (e.g., ./skins/)
- [ ] Store extracted skins in organized directories
- [ ] Save current skin selection to config file
- [ ] Load saved skin on application startup

#### 8. Testing & Validation
- [ ] Test with classic Winamp skins
- [ ] Test with modern Winamp skins
- [ ] Test skin switching without restart
- [ ] Test with missing skin elements
- [ ] Test button hit detection accuracy
- [ ] Verify all controls still function with skins

#### 9. Error Handling
- [ ] Handle corrupted skin files
- [ ] Fall back to default if skin fails to load
- [ ] Display error messages for invalid skins
- [ ] Validate skin files before applying

#### 10. Documentation
- [ ] Document how to install Winamp skins
- [ ] Provide instructions for skin folder location
- [ ] Link to Winamp skin resources/downloads
- [ ] Document any limitations

### Technical Challenges
- **Challenge 1**: Tkinter has limited support for custom graphics
  - Solution: Use Canvas for everything or consider PyQt/Kivy
- **Challenge 2**: Complex region shapes (non-rectangular windows)
  - Solution: May need to use platform-specific window shaping
- **Challenge 3**: Performance with many image overlays
  - Solution: Cache rendered frames, use PIL optimization

### Alternative Approaches
1. **Full Rewrite**: Migrate to PyQt which has better skinning support
2. **Simplified Version**: Support basic color themes instead of full skins
3. **Hybrid**: Keep current UI but allow background/color customization

### Dependencies to Install
```bash
pip install pillow  # Already have
# May need additional libraries based on approach
```

### Files to Modify/Create
1. **NEW**: `skin_manager.py` - Skin loading and parsing
2. **NEW**: `skin_widgets.py` - Custom skinnable widgets
3. **MAJOR REWRITE**: `hello.py` - Convert to skinnable architecture
4. `README.md` - Add skin installation instructions

### Estimated Complexity
⚠️ **HIGH COMPLEXITY** - This is a major feature requiring significant refactoring

### Recommended Phases
1. **Phase 1**: Simple theme system (colors only)
2. **Phase 2**: Background images
3. **Phase 3**: Custom button graphics
4. **Phase 4**: Full Winamp skin compatibility

### Resources Needed
- Sample Winamp skins for testing
- Winamp skin specification documentation
- Time for major UI refactoring (20-40 hours estimated)

---

## Simpler Alternative: Theme Presets
If full Winamp skin support is too complex, consider:
- [ ] Create 5-10 preset color themes
- [ ] Add theme switcher dropdown
- [ ] Store themes as color configuration
- [ ] Examples: Windows 95, Winamp Classic, Dark Mode, Light Mode, etc.
