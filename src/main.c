/*
 * MUSIC PLAYER TUI - Main Event Loop & UI Rendering
 * ================================================
 * 
 * This file demonstrates several CS concepts:
 * 1. MODULAR ARCHITECTURE: Separated concerns (local.c, spotify.c, visualizer.c)
 * 2. STATE MACHINE: MODE_LOCAL vs MODE_SPOTIFY enum for mode switching
 * 3. EVENT-DRIVEN PROGRAMMING: Poll user input, update display, repeat (render loop)
 * 4. NCURSES TUI: Terminal User Interface - low-level terminal control
 * 5. PROCESS MANAGEMENT: Track PID of child processes (mpg123, Spotify, cava)
 * 6. NON-BLOCKING I/O: getch() with ERR handling for 60fps smooth rendering
 * 7. POSIX SIGNALS: Handle SIGCHLD to detect when child processes exit
 * 8. STRUCT-BASED DATA PASSING: Pass by reference instead of global state
 */

#define _POSIX_C_SOURCE 200809L  // Enable POSIX features like popen(), kill(), etc.

#include <ncurses.h>  // Terminal UI library
#include <stdio.h>
#include <string.h>
#include <time.h>  // For current timestamp display
#include <unistd.h>  // POSIX API (sleep, fork, exec, etc.)

// Domain-specific headers: Each module handles one responsibility
#include "local.h"      // Local file playback (scan Music/, play with mpg123)
#include "spotify.h"    // Spotify control (playerctl commands, metadata)
#include "visualizer.h" // Audio visualization (cava integration)

#define MUSIC_DIR "Music"

/*
 * ENUM: Application Modes
 * Why use enum instead of #define?
 *   - Type-safe: Can't accidentally pass wrong value
 *   - Compiler can warn if switch() doesn't cover all cases
 *   - More readable than magic numbers
 */
typedef enum {
    MODE_LOCAL = 0,    // Play from local Music/ folder via mpg123
    MODE_SPOTIFY = 1   // Control Spotify via playerctl (system command)
} PlayerMode;

/*
 * FUNCTION: draw_ui
 * Purpose: Render the entire terminal UI (called every frame)
 * 
 * Key NCURSES concepts:
 *   - getmaxyx(): Query terminal dimensions (rows, cols)
 *   - mvprintw(): Move cursor to (row, col), then print formatted text
 *   - attron/attroff(A_REVERSE): Apply/remove text attributes (highlight selected)
 *   - mvhline(): Draw horizontal line
 * 
 * Layout:
 *   Row 0:    Title (centered)
 *   Row 1:    Current timestamp (centered)
 *   Row 3+:   Content (LOCAL: song list, SPOTIFY: metadata)
 *   Row -3:   Separator line
 *   Row -2:   Keybinding help (mode-specific)
 *   Row -1:   Status message + playback indicator
 * 
 * Note: We recalculate centering dynamically so UI adapts to window resize
 */
static void draw_ui(PlayerMode mode, const SongList *songs, int selected, const char *status, pid_t playing_pid, const SpotifyStatus *sp) {
    time_t now = time(NULL);  // Get current timestamp
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    int rows, cols;
    getmaxyx(stdscr, rows, cols);  // Query terminal size

    // Center the title dynamically
    int title_x = (cols - 22) / 2;
    if (title_x < 0) title_x = 0;
    mvprintw(0, title_x, "Music Player TUI (%s)", mode == MODE_LOCAL ? "Local" : "Spotify");
    mvprintw(1, (cols - (int)strlen(time_buf)) / 2, "%s", time_buf);

    if (mode == MODE_LOCAL) {
        mvprintw(3, 2, "Songs in %s:", MUSIC_DIR);
        if (songs->count == 0) {
            mvprintw(5, 4, "(No .mp3 or .wav files found)");
        } else {
            // Draw song list with highlight on selected item
            for (int i = 0; i < songs->count; i++) {
                if (i == selected) {
                    attron(A_REVERSE);  // Start reverse video (black on white)
                }
                int line_x = 4;
                mvprintw(5 + i, line_x, "%d. %s", i + 1, songs->names[i]);
                if (i == selected) {
                    attroff(A_REVERSE);  // End reverse video
                }
            }
        }
    } else {
        // Spotify mode: Show current track metadata (fetched by spotify_status())
        mvprintw(3, 2, "Spotify control (playerctl)");
        mvprintw(5, 4, "State: %s", sp ? sp->state : "?");
        mvprintw(6, 4, "Title: %s", sp ? sp->title : "");
        mvprintw(7, 4, "Artist: %s", sp ? sp->artist : "");
    }

    // Draw footer section
    mvhline(rows - 3, 0, '-', cols);
    if (mode == MODE_LOCAL) {
        mvprintw(rows - 2, 0, "Enter/Space: play  r: refresh  s: stop  m: mode  q: quit");
        mvprintw(rows - 1, 0, "Status: %s%s", status ? status : "", (playing_pid > 0 ? " (playing)" : ""));
    } else {
        mvprintw(rows - 2, 0, "Enter/Space: play/pause  n: next  b: prev  v: viz  m: mode  q: quit");
        mvprintw(rows - 1, 0, "Status: %s", status ? status : "");
    }
}

/*
 * FUNCTION: main
 * The entry point and event loop - demonstrates core CS patterns:
 * 
 * 1. INITIALIZATION: Set up ncurses, load initial data
 * 2. EVENT LOOP: Continuously:
 *    - Clear screen (redraw from scratch)
 *    - Read user input (non-blocking)
 *    - Update state based on input
 *    - Render UI
 *    - Sleep briefly (60fps target = 16.67ms per frame)
 * 3. CLEANUP: Restore terminal and exit
 * 
 * Pattern: Event-driven programming (desktop apps, game engines use similar loops)
 */
int main(void) {
    // STATE VARIABLES: All application state stored here (no globals except statics in modules)
    SongList songs = {0};       // Array of song names + count (see local.h)
    char status[256] = "";      // Status message (feedback to user)
    int selected = 0;           // Currently selected song index (LOCAL mode)
    int running = 1;            // Flag to control main loop
    pid_t playing_pid = -1;     // PID of currently playing mpg123 process (-1 = none)
    PlayerMode mode = MODE_LOCAL;
    SpotifyStatus sp = {0};     // Current Spotify metadata (state, title, artist)
    Visualizer viz = {0};       // Visualizer bar data (see visualizer.h)
    int viz_enabled = 0;        // Visualizer toggle flag

    load_songs(&songs, MUSIC_DIR, status, sizeof(status));

    /*
     * NCURSES INITIALIZATION
     * Why these setup calls?
     *   initscr():       Initialize ncurses (must be called first)
     *   cbreak():        Disable line buffering - getch() returns immediately
     *   noecho():        Don't echo typed characters to screen
     *   keypad(_, TRUE): Enable special keys (arrow keys, function keys, etc.)
     *   nodelay(_, TRUE): getch() returns ERR if no key pressed (non-blocking)
     *   curs_set(0):     Hide cursor
     */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);  // THIS IS KEY: Makes input non-blocking for 60fps rendering
    curs_set(0);

    /*
     * EVENT LOOP: The heart of interactive applications
     * Pattern: Poll input → Update state → Render → Repeat
     * 
     * This loop runs continuously until user presses 'q'
     * Each iteration: clears screen, updates, redraws (like animation frames)
     */
    while (running) {
        clear();  // Erase entire screen for fresh redraw
        
        // UPDATE STATE: Fetch latest data from system
        if (mode == MODE_SPOTIFY) {
            spotify_status(&sp, status, sizeof(status));  // Get current track metadata from Spotify
            if (viz_enabled) {
                viz_read(&viz);  // Read latest audio visualization data from cava
            }
        }
        
        // RENDER: Draw everything on screen buffer (not yet visible)
        draw_ui(mode, &songs, selected, status, playing_pid, &sp);

        // RENDER: Draw visualizer if enabled in Spotify mode
        if (mode == MODE_SPOTIFY && viz_enabled && viz.bar_count > 0) {
            int rows, cols;
            getmaxyx(stdscr, rows, cols);
            int viz_y = rows - 16;  // Position visualizer near bottom
            if (viz_y < 10) viz_y = 10;
            viz_render(&viz, viz_y, 2, cols - 4);  // Render boxed bars
        }

        // FLUSH: Update actual terminal display (all mvprintw() calls are buffered until refresh())
        refresh();

        /*
         * INPUT: Read single keystroke (non-blocking)
         * 
         * Why ERR check?
         *   nodelay(TRUE) makes getch() non-blocking:
         *   - Returns pressed key code if key available
         *   - Returns ERR (-1) if no key pressed
         *   - If ERR, sleep 16.67ms and continue loop (targets 60fps)
         */
        int ch = getch();
        if (ch == ERR) {
            // No key pressed: sleep ~16.67ms for 60fps (1 second / 60 frames)
            struct timespec ts = {0, 16670000};  // 16,670,000 nanoseconds
            nanosleep(&ts, NULL);
            continue;
        }
        
        /*
         * HANDLE INPUT: Dispatch to appropriate action based on keybinding
         * Pattern: Switch on input to update state (selected song, mode, flags)
         */
        switch (ch) {
            case KEY_UP:
                // Circular navigation: move up with wraparound
                if (songs.count > 0) {
                    selected = (selected - 1 + songs.count) % songs.count;  // Modulo for wraparound
                }
                break;
            case KEY_DOWN:
                // Circular navigation: move down with wraparound
                if (songs.count > 0) {
                    selected = (selected + 1) % songs.count;
                }
                break;
            case 'r':
            case 'R':
                // RELOAD: Scan Music/ directory again (for newly added songs)
                if (mode == MODE_LOCAL) {
                    load_songs(&songs, MUSIC_DIR, status, sizeof(status));
                    if (selected >= songs.count) {
                        selected = songs.count > 0 ? songs.count - 1 : 0;  // Adjust selection if out of bounds
                    }
                }
                break;
            case 's':
            case 'S':
                // STOP: Kill the currently playing mpg123 process
                if (mode == MODE_LOCAL) {
                    stop_playback(&playing_pid, status, sizeof(status));
                }
                break;
            case 'q':
            case 'Q':
                // QUIT: Exit main loop and cleanup
                running = 0;
                break;
            case '\n':  // Enter key
            case ' ':   // Space key
            case KEY_ENTER:
                // PLAY/PAUSE: Handle mode-specific playback toggle
                if (mode == MODE_LOCAL) {
                    stop_playback(&playing_pid, status, sizeof(status));
                    playing_pid = play_song_async(&songs, selected, MUSIC_DIR, status, sizeof(status));
                } else {
                    spotify_play_pause(status, sizeof(status));
                }
                break;
            case 'n':
            case 'N':
                // NEXT: Skip to next track (Spotify only)
                if (mode == MODE_SPOTIFY) {
                    spotify_next(status, sizeof(status));
                }
                break;
            case 'b':
            case 'B':
                // PREVIOUS: Go back one track (Spotify only)
                if (mode == MODE_SPOTIFY) {
                    spotify_previous(status, sizeof(status));
                }
                break;
            case 'm':
            case 'M':
                // MODE TOGGLE: Switch between LOCAL and SPOTIFY
                // Pattern: Cleanup old mode, initialize new mode
                if (mode == MODE_LOCAL) {
                    stop_playback(&playing_pid, status, sizeof(status));
                    spotify_launch(status, sizeof(status));  // Fork Spotify in background
                    mode = MODE_SPOTIFY;
                    viz_start(status, sizeof(status));      // Start cava visualizer
                    viz_enabled = 1;
                } else {
                    viz_stop(status, sizeof(status));       // Stop cava
                    viz_enabled = 0;
                    mode = MODE_LOCAL;
                }
                break;
            case 'v':
            case 'V':
                // VISUALIZER TOGGLE: Turn audio visualization on/off (Spotify mode only)
                if (mode == MODE_SPOTIFY) {
                    if (viz_enabled) {
                        viz_stop(status, sizeof(status));
                        viz_enabled = 0;
                    } else {
                        viz_start(status, sizeof(status));
                        viz_enabled = 1;
                    }
                }
                break;
            default:
                break;
        }
    }

    /*
     * CLEANUP: Restore terminal and free resources
     * Important: endscr() MUST be called to restore terminal to normal state
     * If omitted, terminal remains in weird ncurses state after program exits
     */
    stop_playback(&playing_pid, status, sizeof(status));
    viz_stop(status, sizeof(status));
    endwin();
    return 0;
}