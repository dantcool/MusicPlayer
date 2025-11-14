#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int numOfSongs = 0;
char songNames[100][256];

void listSongs(){
    numOfSongs = 0;

    DIR *folder;
    folder = opendir("Music");

        if (folder == NULL) {
            printf("Could not open Music directory.\n");
        } else { 
            printf("Available music files:\n");

    struct dirent *entry;

        while ((entry = readdir(folder)) != NULL) {

    char *ext = strrchr(entry->d_name, '.');

        if (ext != NULL && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0)) {
            strcpy(songNames[numOfSongs], entry->d_name);
            printf("%d. %s\n", numOfSongs + 1, songNames[numOfSongs]);
                numOfSongs++;
                
        }
    }
    closedir(folder);
}
}

void playSong(int songIndex){

    if (songIndex < 1 || songIndex > numOfSongs) {
        printf("Invalid song index.\n");
        return;
    }

    const char *songPath = songNames[songIndex - 1];
    char command[512];

    snprintf(command, sizeof(command), "mpg123 -o pulse \"Music/%s\"", songPath);
    
    printf("Executing command: %s\n", command);
    printf("Now playing: %s\n", songPath);
    system(command);
}