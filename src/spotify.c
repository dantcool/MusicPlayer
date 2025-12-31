#define _POSIX_C_SOURCE 200809L

#include "spotify.h"

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int command_exists(const char *command_name) {
    const char *path = getenv("PATH");
    if (!path || !command_name || command_name[0] == '\0') {
        return 0;
    }

    char path_copy[4096];
    if (strlen(path) >= sizeof(path_copy)) {
        return 0;
    }
    strcpy(path_copy, path);

    char candidate[PATH_MAX];
    char *dir = strtok(path_copy, ":");

    while (dir) {
        if (snprintf(candidate, sizeof(candidate), "%s/%s", dir, command_name) < (int)sizeof(candidate)) {
            if (access(candidate, X_OK) == 0) {
                return 1;
            }
        }
        dir = strtok(NULL, ":");
    }
    return 0;
}

static void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

int spotify_is_available(void) {
    return command_exists("playerctl");
}

static int run_playerctl(const char *args, char *out, size_t out_len, char *status_buf, size_t status_len) {
    if (!spotify_is_available()) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "playerctl not found in PATH");
        }
        return -1;
    }

    char command[512];
    if (snprintf(command, sizeof(command), "playerctl --player=spotify %s", args) >= (int)sizeof(command)) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Command too long");
        }
        return -1;
    }

    FILE *p = popen(command, out ? "r" : "w");
    if (!p) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Failed to run playerctl");
        }
        return -1;
    }

    if (out && out_len > 0) {
        if (fgets(out, (int)out_len, p) == NULL) {
            out[0] = '\0';
        }
    }

    int rc = pclose(p);
    if (rc != 0 && status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "playerctl returned %d", rc);
    }
    if (out) {
        trim_newline(out);
    }
    return rc == 0 ? 0 : -1;
}

int spotify_is_running(void) {
    char dummy[128];
    return run_playerctl("status", dummy, sizeof(dummy), NULL, 0) == 0;
}

int spotify_launch(char *status_buf, size_t status_len) {
    if (spotify_is_running()) {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Failed to fork for Spotify");
        }
        return -1;
    }

    if (pid == 0) {
        setsid();
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        if (command_exists("spotify")) {
            execlp("spotify", "spotify", "--no-zygote", (char *)NULL);
        } else if (command_exists("flatpak")) {
            execlp("flatpak", "flatpak", "run", "com.spotify.Client", (char *)NULL);
        } else if (command_exists("snap")) {
            execlp("snap", "snap", "run", "spotify", (char *)NULL);
        }
        _exit(1);
    }

    if (status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "Launching Spotify...");
    }
    return 0;
}

int spotify_play_pause(char *status_buf, size_t status_len) {
    return run_playerctl("play-pause", NULL, 0, status_buf, status_len);
}

int spotify_next(char *status_buf, size_t status_len) {
    return run_playerctl("next", NULL, 0, status_buf, status_len);
}

int spotify_previous(char *status_buf, size_t status_len) {
    return run_playerctl("previous", NULL, 0, status_buf, status_len);
}

int spotify_status(SpotifyStatus *out, char *status_buf, size_t status_len) {
    if (!out) {
        return -1;
    }
    out->title[0] = '\0';
    out->artist[0] = '\0';
    out->state[0] = '\0';
    out->art_url[0] = '\0';

    char buffer[256];
    int rc = run_playerctl("metadata --format '{{title}}|{{artist}}|{{status}}|{{mpris:artUrl}}'", buffer, sizeof(buffer), status_buf, status_len);
    if (rc != 0) {
        return -1;
    }

    char *first = strchr(buffer, '|');
    char *second = first ? strchr(first + 1, '|') : NULL;
    char *third = second ? strchr(second + 1, '|') : NULL;
    if (!first || !second || !third) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Unexpected metadata format");
        }
        return -1;
    }

    *first = '\0';
    *second = '\0';
    *third = '\0';
    strncpy(out->title, buffer, sizeof(out->title) - 1);
    out->title[sizeof(out->title) - 1] = '\0';
    strncpy(out->artist, first + 1, sizeof(out->artist) - 1);
    out->artist[sizeof(out->artist) - 1] = '\0';
    strncpy(out->state, second + 1, sizeof(out->state) - 1);
    out->state[sizeof(out->state) - 1] = '\0';
    strncpy(out->art_url, third + 1, sizeof(out->art_url) - 1);
    out->art_url[sizeof(out->art_url) - 1] = '\0';
    return 0;
}
