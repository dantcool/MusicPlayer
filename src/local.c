/*
 * LOCAL PLAYBACK MODULE - Song scanning and mpg123 management
 * ===========================================================
 * 
 * This module demonstrates:
 * 1. FILESYSTEM I/O: opendir() / readdir() to scan directories
 * 2. PROCESS MANAGEMENT: fork() and exec() to spawn child processes
 * 3. POSIX SIGNALS: kill() to terminate processes
 * 4. STRING HANDLING: Fixed-size buffers, strncpy() for safety
 * 5. ENVIRONMENT VARIABLE: getenv("PATH") to locate commands
 * 
 * Pattern: Scanner (load_songs) + Executor (play_song_async/stop_playback)
 * All functions update caller-provided status message for UI feedback
 */

#define _POSIX_C_SOURCE 200809L  // Enable POSIX features: fork, exec, kill, etc.

#include "local.h"

#include <ctype.h>      // Character classification (isalpha, etc.)
#include <dirent.h>     // Directory entry scanning (opendir, readdir, closedir)
#include <limits.h>
#include <signal.h>     // Signals: SIGTERM for process termination
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>   // waitpid() to wait for child process completion
#include <unistd.h>     // POSIX API: fork, exec, access, getenv

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/*
 * HELPER: command_exists
 * Check if executable exists in PATH
 * 
 * How it works:
 *   1. Get PATH environment variable (e.g., "/usr/bin:/usr/local/bin:/bin")
 *   2. Split by ':' to get each directory
 *   3. For each directory, construct full path to command
 *   4. Use access(path, X_OK) to check if executable
 *   5. Return 1 if found, 0 if not
 * 
 * Why check? Defensive programming - verify mpg123 exists before trying to run it
 */
static int command_exists(const char *command_name) {
    const char *path = getenv("PATH");  // Get PATH environment variable
    if (!path || !command_name || command_name[0] == '\0') {
        return 0;
    }

    char path_copy[4096];
    if (strlen(path) >= sizeof(path_copy)) {
        return 0;
    }
    strcpy(path_copy, path);

    char candidate[PATH_MAX];
    char *dir = strtok(path_copy, ":");  // Split PATH by colon

    while (dir) {
        // Construct candidate: /path/to/dir + "/" + command_name
        if (snprintf(candidate, sizeof(candidate), "%s/%s", dir, command_name) < (int)sizeof(candidate)) {
            if (access(candidate, X_OK) == 0) {  // Check if file exists and is executable
                return 1;
            }
        }
        dir = strtok(NULL, ":");  // Get next PATH directory

    }
    return 0;
}

/*
 * HELPER: has_audio_extension
 * Check if filename ends with .mp3 or .wav (case-insensitive)
 * 
 * Demonstrates string manipulation and filename filtering
 */
static int has_audio_extension(const char *name) {
    const char *ext = strrchr(name, '.');  // Find last '.' in filename
    if (!ext || *(ext + 1) == '\0') {
        return 0;  // No extension found
    }

    char lower_ext[8];
    size_t len = 0;
    ext++;
    while (ext[len] && len < sizeof(lower_ext) - 1) {
        lower_ext[len] = (char)tolower((unsigned char)ext[len]);  // Convert to lowercase
        len++;
    }
    lower_ext[len] = '\0';

    // Check if extension matches our supported audio formats
    return (strcmp(lower_ext, "mp3") == 0 || strcmp(lower_ext, "wav") == 0);
}

/*
 * FUNCTION: load_songs
 * Scan music_dir and populate songs list with .mp3/.wav files
 * 
 * Demonstrates:
 *   - Directory iteration: opendir() → readdir() → closedir()
 *   - Dynamic list building (bounded by MAX_SONGS)
 *   - Status message for user feedback
 *   - Error handling (missing dir, no songs, etc.)
 * 
 * Algorithm:
 *   1. Open directory with opendir()
 *   2. Read each entry with readdir()
 *   3. Filter for .mp3/.wav files
 *   4. Add to songs->names[] array until MAX_SONGS reached
 *   5. Update songs->count
 *   6. Close directory
 * 
 * Note: Returns count of files found; caller should check status_buf for errors
 */
int load_songs(SongList *songs, const char *music_dir, char *status_buf, size_t status_len) {
    if (!songs) {
        return -1;
    }

    songs->count = 0;

    DIR *folder = opendir(music_dir);
    if (!folder) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Could not open %s", music_dir);
        }
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(folder)) != NULL) {
        if (!has_audio_extension(entry->d_name)) {
            continue;
        }
        if (songs->count >= MAX_SONGS) {
            if (status_buf && status_len > 0) {
                snprintf(status_buf, status_len, "Showing first %d songs", MAX_SONGS);
            }
            break;
        }
        // Copy filename to songs list (use strncpy for safety against buffer overflows)
        strncpy(songs->names[songs->count], entry->d_name, MAX_NAME_LEN - 1);
        songs->names[songs->count][MAX_NAME_LEN - 1] = '\0';  // Ensure null termination
        songs->count++;
    }
    closedir(folder);

    // Feedback to user if no songs found
    if (songs->count == 0 && status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "No .mp3 or .wav files in %s", music_dir);
    }

    return 0;
}

/*
 * FUNCTION: play_song_async
 * Spawn mpg123 process to play selected song in background
 * 
 * Demonstrates:
 *   - FORK/EXEC pattern: fork() to create child, exec*() to replace with mpg123
 *   - Process management: Return PID so caller can track/kill later
 *   - Path construction: Build full path to music file
 *   - Error handling: Check fork() and exec() return values
 * 
 * FORK/EXEC workflow:
 *   1. Check if mpg123 exists with command_exists()
 *   2. fork() to create child process (now 2 processes!)
 *   3. Child: Build full path, call execvp("mpg123", args)
 *   4. Parent: Return child PID (-1 on error)
 *   5. Parent can later kill(pid, SIGTERM) to stop playback
 * 
 * Why fork + exec? (not just system())
 *   - system() creates intermediate shell, wastes resources
 *   - fork+exec gives direct control over child process
 *   - Allows capturing PID for later termination
 * 
 * Why -q flag? 
 *   - Quiet mode: mpg123 runs with minimal output (no TUI)
 */
pid_t play_song_async(const SongList *songs, int index, const char *music_dir, char *status_buf, size_t status_len) {
    if (!songs || index < 0 || index >= songs->count) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Invalid song selection");
        }
        return -1;
    }

    if (!command_exists("mpg123")) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "mpg123 not found in PATH");
        }
        return -1;
    }

    char song_path[PATH_MAX];
    if (snprintf(song_path, sizeof(song_path), "%s/%s", music_dir, songs->names[index]) >= (int)sizeof(song_path)) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Song path too long");
        }
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Unable to start playback");
        }
        return -1;
    }

    if (pid == 0) {
        // CHILD PROCESS: Replace this process with mpg123
        // execlp(): Search PATH for mpg123, then replace current process
        // (Converts "./local" → "mpg123")
        execlp("mpg123", "mpg123", "-q", song_path, (char *)NULL);
        
        // If we reach here, exec() failed (shouldn't happen normally)
        perror("mpg123");
        _exit(1);  // _exit() because exec() didn't replace us
    }

    // PARENT PROCESS: Return child PID so caller can track/kill it later
    if (status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "Playing %s", songs->names[index]);
    }
    return pid;  // Return child PID to caller
}

/*
 * FUNCTION: stop_playback
 * Terminate the mpg123 child process
 * 
 * Demonstrates:
 *   - Signal sending: kill(pid, SIGTERM)
 *   - Process waiting: waitpid() to reap child resource
 *   - Cleanup: Set pid to -1 so caller knows it's stopped
 * 
 * SIGTERM flow:
 *   1. kill(*pid, SIGTERM): Send termination signal
 *   2. mpg123 receives signal, cleans up, exits
 *   3. waitpid(*pid, NULL, 0): Block until child exits
 *   4. Set *pid = -1 to indicate no active playback
 * 
 * Why waitpid()? Prevent zombie process
 *   - If child exits but parent doesn't wait(), zombie remains in process table
 *   - waitpid() reaps zombie and frees kernel resources
 */
int stop_playback(pid_t *pid, char *status_buf, size_t status_len) {
    if (!pid || *pid <= 0) {
        return 0;  // No active playback
    }

    kill(*pid, SIGTERM);              // Send termination signal to mpg123
    waitpid(*pid, NULL, 0);           // Wait for child to exit and reap it
    *pid = -1;                        // Mark as stopped

    if (status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "Stopped playback");
    }
    return 0;
}
