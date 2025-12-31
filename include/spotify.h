#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <stddef.h>

typedef struct {
    char title[128];
    char artist[128];
    char state[32];
    char art_url[256];
} SpotifyStatus;

int spotify_is_available(void);
int spotify_is_running(void);
int spotify_launch(char *status_buf, size_t status_len);
int spotify_play_pause(char *status_buf, size_t status_len);
int spotify_next(char *status_buf, size_t status_len);
int spotify_previous(char *status_buf, size_t status_len);
int spotify_status(SpotifyStatus *out, char *status_buf, size_t status_len);

#endif // SPOTIFY_H
