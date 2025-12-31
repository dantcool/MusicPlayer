#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <stddef.h>

typedef struct {
    int bars[64];
    int bar_count;
} Visualizer;

int viz_start(char *status_buf, size_t status_len);
int viz_stop(char *status_buf, size_t status_len);
int viz_read(Visualizer *out);
void viz_render(const Visualizer *viz, int start_y, int start_x, int width);

#endif // VISUALIZER_H
