#define _POSIX_C_SOURCE 200809L

#include "visualizer.h"

#include <fcntl.h>
#include <limits.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static FILE *viz_pipe = NULL;

int viz_start(char *status_buf, size_t status_len) {
    if (viz_pipe != NULL) {
        return 0;
    }

    if (!command_exists("cava")) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "cava not found in PATH");
        }
        return -1;
    }

    FILE *cfg = fopen("/tmp/cava_mp.conf", "w");
    if (!cfg) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Failed to create cava config");
        }
        return -1;
    }
    fprintf(cfg, "[general]\n");
    fprintf(cfg, "bars = 64\n");
    fprintf(cfg, "[output]\n");
    fprintf(cfg, "method = raw\n");
    fprintf(cfg, "raw_target = /dev/stdout\n");
    fprintf(cfg, "channels = mono\n");
    fprintf(cfg, "[input]\n");
    fprintf(cfg, "method = pulseaudio\n");
    fclose(cfg);

    const char *cmd = "cava -p /tmp/cava_mp.conf";
    FILE *p = popen(cmd, "r");
    if (!p) {
        if (status_buf && status_len > 0) {
            snprintf(status_buf, status_len, "Failed to start cava");
        }
        return -1;
    }

    int fd = fileno(p);
    if (fd >= 0) {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    viz_pipe = p;
    if (status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "Visualizer started");
    }
    return 0;
}

int viz_stop(char *status_buf, size_t status_len) {
    if (viz_pipe == NULL) {
        return 0;
    }
    pclose(viz_pipe);
    viz_pipe = NULL;
    if (status_buf && status_len > 0) {
        snprintf(status_buf, status_len, "Visualizer stopped");
    }
    return 0;
}

int viz_read(Visualizer *out) {
    if (!out || viz_pipe == NULL) {
        return -1;
    }

    unsigned char buffer[128];
    int n = fread(buffer, 1, sizeof(buffer), viz_pipe);
    if (n <= 0) {
        if (ferror(viz_pipe)) {
            clearerr(viz_pipe);
        }
        return -1;
    }

    int new_bars[64];
    int new_count = 0;
    for (int i = 0; i < n && new_count < 64; i++) {
        new_bars[new_count++] = (buffer[i] / 25) % 10;
    }

    if (new_count > 0) {
        out->bar_count = new_count;
        memcpy(out->bars, new_bars, sizeof(int) * new_count);
        return 0;
    }
    return -1;
}

void viz_render(const Visualizer *viz, int start_y, int start_x, int width) {
    if (!viz || width < 20) return;

    int bars_to_show = (viz->bar_count < (width - 4) / 2) ? viz->bar_count : (width - 4) / 2;
    if (bars_to_show <= 0) return;

    int box_width = bars_to_show * 2 + 4;
    int box_x = start_x + (width - box_width) / 2;
    int box_height = 16;

    mvaddch(start_y, box_x, ACS_ULCORNER);
    for (int i = 1; i < box_width - 1; i++) {
        mvaddch(start_y, box_x + i, ACS_HLINE);
    }
    mvaddch(start_y, box_x + box_width - 1, ACS_URCORNER);

    for (int i = 1; i < box_height - 1; i++) {
        mvaddch(start_y + i, box_x, ACS_VLINE);
        mvaddch(start_y + i, box_x + box_width - 1, ACS_VLINE);
    }

    mvaddch(start_y + box_height - 1, box_x, ACS_LLCORNER);
    for (int i = 1; i < box_width - 1; i++) {
        mvaddch(start_y + box_height - 1, box_x + i, ACS_HLINE);
    }
    mvaddch(start_y + box_height - 1, box_x + box_width - 1, ACS_LRCORNER);

    int bar_start_y = start_y + box_height - 3;
    for (int i = 0; i < bars_to_show; i++) {
        int bar_height = viz->bars[i];
        int x = box_x + 2 + i * 2;
        for (int h = 0; h < bar_height && h < (box_height - 3); h++) {
            mvaddch(bar_start_y - h, x, ACS_BLOCK);
            mvaddch(bar_start_y - h, x + 1, ACS_BLOCK);
        }
    }
}
