#ifndef LOCAL_H
#define LOCAL_H

#include <sys/types.h>
#include <stddef.h>

#define MAX_SONGS 100
#define MAX_NAME_LEN 256

typedef struct {
    int count;
    char names[MAX_SONGS][MAX_NAME_LEN];
} SongList;

int load_songs(SongList *songs, const char *music_dir, char *status_buf, size_t status_len);
pid_t play_song_async(const SongList *songs, int index, const char *music_dir, char *status_buf, size_t status_len);
int stop_playback(pid_t *pid, char *status_buf, size_t status_len);

#endif // LOCAL_H
